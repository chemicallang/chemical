// `web::Shell` — the middle delivery mode (lang/docs/web-stack-plan.md §3.12).
//
// A route's HTML is produced once, the request-varying regions are recorded as
// byte ranges of that frozen buffer, and a request splices fills into those ranges
// instead of walking the component tree again:
//
//   public func Dashboard(ctx : &mut web::RequestContext, page : &mut HtmlPage) {
//       #html {
//           <header>@{page.slot("account")}</header>
//           <main><h1>Dashboard</h1></main>
//       }
//   }
//
//   public func DashboardFills(req : &http::Request) : web::SlotFills {
//       var f = web::SlotFills()
//       f.set("account", account_bar(req))       // plain HTML, no component calls
//       return f
//   }
//
//   app.shell("/dashboard", Dashboard, DashboardFills)
//
// What a shell is not: a second renderer. `HtmlPage` still produces the bytes; a
// shell is a frozen buffer plus the ranges to patch. Structure that varies per
// request (signed in vs out, admin vs user) is a separate shell, not a branch
// inside a frozen one, and a fill is never a component call: a universal
// component's hydration ids are baked into the shell and cannot be allocated per
// request.
public namespace web {

using std::string;
using std::string_view;
using std::vector;

// One piece of a frozen document: either a literal byte range `[start, end)` of
// `Shell.frozen`, or (when `slot >= 0`) a fill position (with an empty range).
@direct_init
public struct ShellSegment {
    public var start : ubigint
    public var end : ubigint
    public var slot : int
}

/// A route's HTML frozen once, with a byte range per request-varying region.
///
/// `compile` produces it from a finished document, so a shell is byte-identical to
/// a full SSR of the same route by construction; `emit` reconstructs the document
/// from the literals plus the fills. Cost per request: one pass over the segments
/// and one `append` per segment — no component work, no CSS hashing, no hydration
/// id allocation.
@direct_init
public struct Shell {

    private var frozen : string
    private var segments : vector<ShellSegment>
    private var names : vector<string>
    private var failure : string

    @constructor
    public func constructor() {
        return Shell {
            frozen : string(),
            segments : vector<ShellSegment>(),
            names : vector<string>(),
            failure : string()
        }
    }

    /// Freeze `html` (one finished page) into a shell. Every `slot` marker becomes a
    /// fill position; the marker bytes themselves are dropped. The document is
    /// copied, so the render that produced it may die immediately.
    ///
    /// A malformed marker (no closing `-->`, or a name `is_valid_slot_name` rejects)
    /// is a **build error**, reported by `is_valid`/`error_message`: a shell with a
    /// wrong slot would serve one route's value into another's position.
    @make
    public func compile(html : string_view) : Shell {
        var shell = Shell()
        if(html.empty()) {
            shell.fail_with("the document is empty")
            return shell
        }
        shell.frozen.append_view(&html)

        var prefix = slot_marker_prefix()
        var suffix = slot_marker_suffix()
        var pos : ubigint = 0
        var literal_start : ubigint = 0
        while(true) {
            var open = find_bytes(&html, &prefix, pos)
            if(open < 0) { break }
            var name_start = open as ubigint + prefix.size()
            var close = find_bytes(&html, &suffix, name_start)
            if(close < 0) {
                shell.fail_with("a slot marker has no closing '-->'")
                return shell
            }
            var name = unsafe(string_view(html.data() + name_start, close as ubigint - name_start))
            if(!is_valid_slot_name(&name)) {
                shell.fail_with("a slot marker has a name that is not [A-Za-z0-9_-]{1,64}")
                return shell
            }

            var marker_at = open as ubigint
            if(marker_at > literal_start) {
                shell.segments.push_back(ShellSegment { start : literal_start, end : marker_at, slot : -1 })
            }
            var slot = shell.slot_index(&name)
            if(slot < 0) {
                var copy = string()
                copy.append_view(&name)
                shell.names.push_back(copy)
                slot = (shell.names.size() - 1) as int
            }
            var marker_end = close as ubigint + suffix.size()
            shell.segments.push_back(ShellSegment { start : marker_end, end : marker_end, slot : slot })
            literal_start = marker_end
            pos = marker_end
        }
        if(literal_start < html.size()) {
            shell.segments.push_back(ShellSegment { start : literal_start, end : html.size(), slot : -1 })
        }
        return shell
    }

    public func is_valid(&self) : bool {
        return failure.empty()
    }

    /// Why `compile` refused this document (empty on success).
    public func error_message(&self) : string_view {
        return failure.to_view()
    }

    public func slot_count(&self) : ubigint {
        return names.size()
    }

    public func slot_name(&self, i : ubigint) : string_view {
        if(i >= names.size()) { return string_view("") }
        var name = unsafe(&*names.get_ptr(i))
        return name.to_view()
    }

    /// Index of `name`, or -1 when the shell has no such slot. A caller uses this to
    /// reject a fill for a slot the route never declared instead of silently
    /// dropping it.
    public func slot_index(&self, name : &std::string_view) : int {
        var i : ubigint = 0
        while(i < names.size()) {
            var existing = unsafe(&*names.get_ptr(i))
            var existing_view = existing.to_view()
            if(existing_view.equals(name)) { return i as int }
            i = i + 1
        }
        return -1
    }

    /// Bytes of the frozen document (its slots not counted): the shell's build cost,
    /// stable across requests.
    public func frozen_bytes(&self) : ubigint {
        return frozen.size()
    }

    public func segment_count(&self) : ubigint {
        return segments.size()
    }

    /// Reconstruct the document: literals from the frozen buffer, one fill per slot,
    /// in slot order. `values` must carry exactly `slot_count()` entries; a wrong
    /// count returns an empty string (the caller refused to fill, so it must not
    /// serve a shell missing a region).
    public func emit(&self, values : &std::vector<std::string_view>) : std::string {
        var out = std::string()
        if(!is_valid() || values.size() != names.size()) { return out }
        var frozen_view = frozen.to_view()
        var i : ubigint = 0
        while(i < segments.size()) {
            var seg = unsafe(&*segments.get_ptr(i))
            if(seg.slot >= 0) {
                var fill = values.at(seg.slot as ubigint)
                out.append_view(&fill)
            } else {
                var len = seg.end - seg.start
                if(len > 0) {
                    var literal = unsafe(string_view(frozen_view.data() + seg.start, len))
                    out.append_view(&literal)
                }
            }
            i = i + 1
        }
        return out
    }

    func fail_with(&mut self, message : string_view) {
        if(!failure.empty()) {
            failure.append_view("; ")
        }
        failure.append_view(&message)
    }
}

/// The per-request half of a shell route: the values for its slots.
///
/// A value is plain HTML/text, never a component call (see the file comment). A
/// slot the provider does not set emits nothing — that is a legitimate
/// "no banner for this user" — while a value for a slot the shell does not have is
/// ignored by `App` (the shell decides the positions, not the provider).
@direct_init
public struct SlotFills {

    private var names : vector<string>
    private var values : vector<string>

    @constructor
    public func constructor() {
        return SlotFills {
            names : vector<string>(),
            values : vector<string>()
        }
    }

    /// Set a slot to HTML that is **already final**: it is spliced in verbatim, so
    /// the caller owns escaping. Use `set_text` for anything user-provided.
    public func set(&mut self, name : std::string_view, html : std::string_view) {
        set_value(name, html)
    }

    /// Set a slot to text: `&`, `<`, `>`, `"` and `'` are escaped, which is correct
    /// for a text position and for a quoted attribute value.
    public func set_text(&mut self, name : std::string_view, text : std::string_view) {
        var escaped = escape_html_view(text)
        var escaped_view = escaped.to_view()
        set_value(name, escaped_view)
    }

    /// The value set for `name` (empty when unset).
    public func get(&self, name : std::string_view) : string_view {
        var i : ubigint = 0
        while(i < names.size()) {
            var existing = unsafe(&*names.get_ptr(i))
            var existing_view = existing.to_view()
            if(existing_view.equals(&name)) {
                var value = unsafe(&*values.get_ptr(i))
                return value.to_view()
            }
            i = i + 1
        }
        return string_view("")
    }

    public func count(&self) : ubigint {
        return names.size()
    }

    // Last write wins: a provider that sets a slot twice means the later value.
    func set_value(&mut self, name : std::string_view, value : std::string_view) {
        var i : ubigint = 0
        while(i < names.size()) {
            var existing = unsafe(&*names.get_ptr(i))
            var existing_view = existing.to_view()
            if(existing_view.equals(&name)) {
                var slot = values.get_ptr(i)
                unsafe {
                    slot.clear()
                    slot.append_view(&value)
                }
                return
            }
            i = i + 1
        }
        var key = string()
        key.append_view(&name)
        var stored = string()
        stored.append_view(&value)
        names.push_back(key)
        values.push_back(stored)
    }
}

/// The per-request half of a shell route: given the request, produce its fills.
/// This is the only code that runs per request for a shell route.
public type SlotValues = (req : &http::Request) => SlotFills

/// Picks between the variants of one shell route (`App.shell_variant`). Each
/// variant is a separately frozen document, so choosing one per request costs one
/// predicate call — the chrome it serves is still byte-range splices.
public type ShellPredicate = (req : &http::Request) => bool

/// The predicate a plain `shell` route carries: never consulted (`has_when` is
/// false), so it only has to exist to keep the route struct uniform.
public func default_shell_predicate(req : &http::Request) : bool {
    var _ = req
    return true
}

// Index of `needle` in `hay` at or after `from`, or -1. A byte scan rather than a
// `string` search so a shell's markup is never copied to look for a marker.
func find_bytes(hay : &std::string_view, needle : &std::string_view, from : ubigint) : int {
    var h = hay.size()
    var n = needle.size()
    if(n == 0 || n > h) { return -1 }
    var i = from
    while(i + n <= h) {
        var j : ubigint = 0
        while(j < n && hay.at(i + j) == needle.at(j)) { j = j + 1 }
        if(j == n) { return i as int }
        i = i + 1
    }
    return -1
}

} // namespace web
