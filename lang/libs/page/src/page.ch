
using std::Result;

// HTML escaping, and the sinks #html uses for it.
//
// `#html` escapes `{value}` by default: it becomes a text node (or an attribute
// value) and is escaped exactly once by the `append_text_*` sinks below. `{!value}`
// is the explicit raw splice, for a value that already is markup. A
// `<script>`/`<style>` body is raw text (the browser never entity-decodes it), so
// values there are written verbatim. These two helpers stay public because literal
// head/attribute markup assembled by the libraries still needs them.
public func escape_html(value : &std::str) : std::string {
    return escape_html_view(value.to_view())
}

public func escape_html_view(value : std::string_view) : std::string {
    var out = std::string()
    var i : size_t = 0
    while(i < value.size()) {
        var c = value.at(i)
        if(c == '<') { out.append_view(std::string_view("&lt;")) }
        else if(c == '>') { out.append_view(std::string_view("&gt;")) }
        else if(c == '&') { out.append_view(std::string_view("&amp;")) }
        else if(c == '"') { out.append_view(std::string_view("&quot;")) }
        else if(c == '\'') { out.append_view(std::string_view("&#39;")) }
        else { out.append(c) }
        i = i + 1
    }
    return out
}

// ── Slot markers (frozen shells, lang/docs/web-stack-plan.md §3.12) ──────
// `HtmlPage.slot` writes one of these; `web::Shell.compile` parses it into a byte
// range. The constants live here, not in `web`, so the writer and the parser
// cannot drift apart (the dependency runs `web → page`).
public func slot_marker_prefix() : std::string_view {
    return std::string_view("<!--chx-slot:")
}

public func slot_marker_suffix() : std::string_view {
    return std::string_view("-->")
}

// A slot name is written verbatim between the marker delimiters, so it is limited
// to characters that cannot end the comment or be mistaken for markup: 1..64 of
// `[A-Za-z0-9_-]`. Anything else marks nothing, and `Shell.compile` never sees it.
public func is_valid_slot_name(name : &std::string_view) : bool {
    var n = name.size()
    if(n == 0 || n > 64) { return false }
    var i : size_t = 0
    while(i < n) {
        var c = name.at(i)
        var ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-'
        if(!ok) { return false }
        i = i + 1
    }
    return true
}

// ── Router parameter store (lang/docs/universal-router-design.md §3, §14.5)
//
// `PageParameter` lives in `page` (not in the `router` library) because the
// dependency runs `router → page`: a type defined in `router` could not be a
// field of `HtmlPage`. The router library contributes only extension functions
// over this type.
public variant PageParameter {
    Text(view : std::string_view)
    Object(ptr : *mut void)
}

// One declared route pattern collected during render for the static-export
// `routes.json` manifest (§13.2.3). Page-local, so `page` need not know about
// `router::RoutePattern`.
@direct_init
// Typed head metadata (§3.17 #5, §3.12): one declaration for the document head
// instead of ad-hoc `<title>` strings appended from page bodies. Every field is
// optional and empty/false means "emit nothing", so `Head{title: "Docs"}` is the
// common case and the rest are opt-in.
public struct PageHead {
    var title : std::string_view = ""
    var description : std::string_view = ""
    var canonical : std::string_view = ""
    var noindex : bool = false
}

public struct RouteManifestEntry {
    var router : std::string_view
    var pattern : std::string_view    // "/projects/{id}", or "" for a fallback
    var id : std::string_view
    var is_fallback : bool
}

func route_json_escape(v : std::string_view, out : &mut std::string) {
    for(var i : size_t = 0; i < v.size(); i++) {
        const c = v.at(i)
        if(c == '"') { out.append_view(std::string_view("\\\"")) }
        else if(c == '\\') { out.append_view(std::string_view("\\\\")) }
        else if(c == '\n') { out.append_view(std::string_view("\\n")) }
        else if(c == '\r') { out.append_view(std::string_view("\\r")) }
        else if(c == '\t') { out.append_view(std::string_view("\\t")) }
        else { out.append(c) }
    }
}

// Escapes `v` for embedding inside a double-quoted JS string literal.
func append_router_js_quoted(v : std::string_view, out : &mut std::string) {
    for(var i : size_t = 0; i < v.size(); i++) {
        const c = v.at(i)
        if(c == '"') { out.append_view(std::string_view("\\\"")) }
        else if(c == '\\') { out.append_view(std::string_view("\\\\")) }
        else if(c == '\n') { out.append_view(std::string_view("\\n")) }
        else if(c == '\r') { out.append_view(std::string_view("\\r")) }
        else if(c == '\t') { out.append_view(std::string_view("\\t")) }
        else { out.append(c) }
    }
}

// ── Static-route SSR snapshot cache (Phase 7, §7.5) ────────────────────────
//
// A route whose body is a purely-static native subtree (no components, no
// expressions, no hooks, no params) renders identically on every request. The
// generated router renders such a route once, stores the bytes in this
// process-global cache, and appends them on later requests.
//
// The map is node-based, so stored byte ranges stay valid across later inserts.
// Reads/writes are guarded by one mutex; rendering itself happens per-page
// (outside the lock), so a concurrent miss simply renders+stores identical bytes.
// A snapshot owns its bytes in a `std::string`, so `unordered_map.clear()` (and
// an overwrite via `insert`) frees them and `reset_route_snapshots` cannot leak.
// The map itself is `@never_destructed`: entries live for the process lifetime
// (bounded by the number of static routes at compile time), which is the
// intended cache behaviour.
struct RouteSnapshot {
    var data : std::string
}

@never_destructed
var route_snapshots : std::unordered_map<std::string, RouteSnapshot>
// A zero-initialized `std::mutex` is a valid default lock on POSIX (a zeroed
// pthread mutex) and Windows (SRWLOCK_INIT). It is only ever used after
// `ensure_route_snapshots`, which constructs the map under it.
@never_destructed
var route_snapshots_mutex : std::mutex
var route_snapshots_ready : bool = false
var route_snapshots_enabled : bool = true
var route_snapshot_hits : ubigint = 0
var route_snapshot_misses : ubigint = 0

// Lazily constructs the snapshot map (top-level globals are not auto-constructed).
func ensure_route_snapshots() {
        unsafe {
        var lk = std::lock_guard(&mut route_snapshots_mutex)
        if(!route_snapshots_ready) {
            route_snapshots = std::unordered_map<std::string, RouteSnapshot>()
            route_snapshots_ready = true
        }

        }
    }

func route_snapshot_key(key : std::string_view) : std::string {
    return unsafe(std::string(key.data(), key.size()))
}


public struct HtmlPage {

    var pageHead : std::string

    var pageHtml : std::string

    var pageCss : std::string

    var pageJs : std::string

    var pageHeadJs : std::string
    // Start of the typed-head block emitted by `set_head`, so a later call can
    // replace it instead of appending a duplicate `<title>`.
    var head_mark_start : size_t = 0
    var head_mark_valid : bool = false

    var pageJsEnd : std::string
    var js_hoist_pos : ubigint = 0

    // Non-owning sink borrowed for the render. Private so the pointer cannot be
    // forged or replaced without `attach_shared`; the owning handle is
    // `SharedAssetsHandle` (see shared_assets.ch).
    private var shared : *mut SharedAssets = null

    // Which route this page is, for the shared sink's unit graph (see
    // `SharedAssets`): 0 = not part of a site, 1..N = the N-th pre-rendered route,
    // `unit_owner_dynamic()` = a per-request (live/shell) render. A `StaticSite`
    // sets it before the page renders; every component use reports the token, so
    // the build learns each route's real component closure instead of guessing it
    // from the emitted text.
    var unit_owner : ubigint = 0

    private var local_js_depth : ubigint = 0
    private var local_css_depth : ubigint = 0

    // When true, a generated universal component server function emits only its
    // client JS (its `require_component` block + hoisting) and skips SSR markup.
    // The converter sets this around the child server-function call it makes
    // during the client-JS pass, so a nested component's subtree is server
    // rendered exactly once (in the later HTML pass) instead of twice. Without
    // it, SSR work grows exponentially with component-tree depth.
    var render_js_only : bool = false

    // we track which classes are done through this unordered map
    // TODO using ubigint, instead need to use size_t
    private var doneClasses : std::unordered_map<ubigint, bool>

    // track random CSS classes (for dynamic values) to prevent duplicates
    private var doneRandomClasses : std::unordered_map<ubigint, bool>

    private var doneComponents : std::unordered_map<ubigint, bool>

    // Set by the first `require_component` on this page (i.e. by the generated
    // guard of a universal component's server function). `used_universal()` is how
    // a caller decides whether the page needs the hydration runtime at all: a
    // component-free page that skips `defaultUniversalSetup` writes no JS, so
    // `StaticSite` gives it no chunk and no bundle `<script>`.
    private var universal_used : bool = false

    // Global JS blocks emitted via `#globaljs`, keyed by source location.
    private var doneJs : std::unordered_map<ubigint, bool>

    // ── Router parameter store (§3.2) ────────────────────────────────────────
    // Node-based map, so `get_ptr(key)` references stay valid across later
    // inserts for the lifetime of the render (D-5.1).
    private var parameters : std::unordered_map<std::string_view, PageParameter>
    // Node-based storage for decoded URL params whose views are stored in
    // `parameters` (§6.1 / D-5.3): decoding produces new bytes that must outlive
    // the match, and the node addresses are stable across later inserts.
    // Private: `add_parameter_owned`/`has_owned_parameter` are the API.
    private var route_param_owned : std::unordered_map<std::string, std::string>
    // Set when a request URL matched no route and no fallback existed; read by
    // the handler after render to choose the status code (§6.1). Server-only.
    private var route_missing_flag : bool = false
    // Patterns declared by generated router functions during render, serialized
    // by writeToDirectory for static export (§13.2.3).
    private var route_manifest : std::vector<RouteManifestEntry>
    // Emitted-once latch for the client router runtime (INV-18: zero bytes on
    // pages that never call ensure_router_runtime).
    private var router_runtime_emitted : bool = false

    // ── Per-instance hydration ids ───────────────────────────────────────────
    // Hydration ids used to be the encoded source location of the JSX tag, a
    // compile-time constant, so two instances of one component call site shared
    // an id and the second dispatch re-hydrated the first element. They are now
    // allocated from this counter at render time, which makes them unique per
    // *instance*; a frozen shell keeps the same ids across requests, which is
    // what lets its prebuilt bytes stay valid.
    private var hydration_seq : ubigint = 0

    /// Allocate the next per-instance hydration id. The caller keeps it in a
    /// local and passes it to the marker and the client dispatch, so both name
    /// the same element even when nested components allocate in between.
    func next_hydration_id(&mut self) : ubigint {
        hydration_seq = hydration_seq + 1
        return hydration_seq
    }

    /// Open an inline hydration boundary: `<span id="u<id>" data-chx-i>`.
    func begin_hydration_marker(&mut self, id : ubigint) {
        pageHtml.append_view("<span id=\"u")
        pageHtml.append_uinteger(id)
        pageHtml.append_view("\" data-chx-i>")
    }

    /// Open a comment hydration boundary (`<!--u<id>-->`), valid inside table
    /// structure where a wrapper element would be foster-parented away.
    func begin_hydration_comment(&mut self, id : ubigint) {
        pageHtml.append_view("<!--u")
        pageHtml.append_uinteger(id)
        pageHtml.append_view("-->")
    }

    func getHead(&self) : std::string_view {
        return pageHead.to_view()
    }

    func getHtml(&self) : std::string_view {
        return pageHtml.to_view();
    }

    func getCss(&self) : std::string_view {
        return pageCss.to_view()
    }

    func getJs(&self) : std::string_view {
        return pageJs.to_view()
    }

    func getHeadJs(&self) : std::string_view {
        return pageHeadJs.to_view()
    }

    // ── Production minification ──────────────────────────────────────────────
    // When true, serialization writes this page's own JS and CSS through
    // `page.minify_js` / `page.minify_css`: comments and insignificant
    // whitespace go, identifiers stay, so a stack trace still names the
    // functions that ran. The render is untouched and the buffers are never
    // rewritten — the same page still dumps and reads as source while a
    // component is being written, and only the artifact shrinks.
    //
    // Off by default: emitted bytes are the most useful thing to read during
    // development, and only bytes that go over a network are worth shrinking.
    // `StaticSite`/`web::App` turn it on from `set_minify`, which is where a
    // production site decides it.
    private var minify_output : bool = false

    /// Minify this page's own JS and CSS when it is serialized: `toString`,
    /// `toStringExternalJs`, `toStringWithAssets`, and the `{name}.js` /
    /// `{name}_head.js` / `{name}.css` files `writeToDirectory` writes. Off by default;
    /// call before serializing.
    ///
    /// The work happens at serialization, which is a build for an export and a
    /// response for a server that inlines its JS. The `getJs`/`toStringJsOnly`
    /// accessors keep returning the rendered bytes, so an app that serves its
    /// client JS as a cached asset can minify it once and hand it out as-is.
    public func set_minify(&mut self, on : bool) {
        minify_output = on
    }

    public func is_minify_enabled(&self) : bool {
        return minify_output
    }

    // Serialization helpers for the two buffers that are not `pageJs` (which
    // `getFinalizedPageJs` already minifies). Appending straight through when the
    // switch is off is why enabling it costs nothing on a development build; a
    // minifier that cannot prove a rewrite is safe returns its input unchanged, so
    // a page never ships half-minified code.
    func append_output_head_js(&self, out : &mut std::string) {
        if(!minify_output) {
            out.append_string(&pageHeadJs)
            return
        }
        var js = minify_js(pageHeadJs.to_view())
        out.append_string(&js)
    }

    func append_output_css(&self, out : &mut std::string) {
        if(!minify_output) {
            out.append_string(&pageCss)
            return
        }
        var css = minify_css(pageCss.to_view())
        out.append_string(&css)
    }

    // ── Shared JS/CSS sink ────────────────────────────────────────────────
    // Raw pointer attach (legacy). The page does not take ownership; the sink
    // must outlive every attached page. Prefer `attach_shared_ref` /
    // `SharedAssetsHandle`.
    public unsafe func attach_shared(&mut self, s : *mut SharedAssets) {
        shared = s
        if(s != null) { unsafe { js_hoist_pos = s.hoist_end() } }
    }

    // Borrow a shared sink for this render. The caller keeps ownership (via a
    // `SharedAssetsHandle`) and must drop it only after the pages are done.
    public func attach_shared_ref(&mut self, s : &mut SharedAssets) {
        shared = s as *mut SharedAssets
        unsafe { js_hoist_pos = s.hoist_end() }
        // The definition hoist writes at `js_hoist_pos` in the *sink's* buffer, and
        // `js_hoist_pos` is per page. A page therefore starts hoisting where the
        // sink's already-hoisted prefix ends; starting at 0 would insert its
        // definitions *before* another page's and shift ranges a previous page
        // already recorded.
    }

    // Stop routing page-independent output into the sink.
    public func detach_shared(&mut self) {
        shared = null
    }

    public func has_shared(&self) : bool {
        return self.shared != null
    }

    public func begin_local_js(&mut self) {
        local_js_depth = local_js_depth + 1
    }

    public func end_local_js(&mut self) {
        if(local_js_depth > 0) { local_js_depth = local_js_depth - 1 }
    }

    public func begin_local_css(&mut self) {
        local_css_depth = local_css_depth + 1
    }

    public func end_local_css(&mut self) {
        if(local_css_depth > 0) { local_css_depth = local_css_depth - 1 }
    }

    public func local_js(&self) : std::string {
        var out = std::string()
        out.reserve(pageJs.size() + pageJsEnd.size())
        out.append_view(pageJs.to_view())
        out.append_view(pageJsEnd.to_view())
        return out
    }

    public func local_css(&self) : std::string {
        return pageCss.copy()
    }

    func runtime_css(&mut self, v : &std::string_view) {
        unsafe {
            if(shared != null) { shared.css_data.append_view(v) } else { pageCss.append_view(v) }

        }
    }

    func runtime_js(&mut self, v : &std::string_view) {
        unsafe {
            if(shared != null) { shared.js_data.append_view(v) } else { pageJs.append_view(v) }

        }
    }

    func runtime_head_js(&mut self, v : &std::string_view) {
        unsafe {
            if(shared != null) { shared.js_data.append_view(v) } else { pageHeadJs.append_view(v) }

        }
    }

    // Safe append: a length-tracked string view. The raw `*char` + length form
    // lives in the module's `raw` namespace (`raw::append_html`) and in
    // `append_html_raw` for CBI.
    public func append_html(&mut self, value : &std::str) {
        pageHtml.append_view(value.to_view())
    }

    // Raw CBI entry point: the html plugin emits `page.append_html_raw(ptr, len)`.
    func append_html_raw(&mut self, value : *char, len : size_t) {
        pageHtml.append_with_len(value, len);
    }

    func append_html_char_ptr(&mut self, value : *char) {
        pageHtml.append_char_ptr(value);
    }

    func append_html_char(&mut self, value : char) {
        pageHtml.append(value)
    }

    // ── Text sinks: what `{expr}` means (`#html` escapes by default) ──────────
    // A value interpolated into `#html` is **text**, so `&`, `<`, `>`, `"` and `'`
    // are escaped on the way in. `append_text_*` is that sink; the `append_html_*`
    // sinks above stay raw for a value the author built as markup (`page.append_html`,
    // a type's `writeToPageHtml`), and `@(expr)` in a template is the raw form.
    //
    // Escaping here rather than in the macro keeps one definition of "safe to put in
    // a document" and makes the guarantee hold for every backend (C and LLVM both
    // call these functions).
    func append_text_char_ptr(&mut self, value : *char) {
        var view = unsafe(std::string_view(value, strlen(value)))
        appendHtmlEscaped(&mut pageHtml, &view)
    }

    func append_text_char(&mut self, value : char) {
        switch(value) {
            '&' => pageHtml.append_view("&amp;")
            '<' => pageHtml.append_view("&lt;")
            '>' => pageHtml.append_view("&gt;")
            '"' => pageHtml.append_view("&quot;")
            '\'' => pageHtml.append_view("&#39;")
            default => pageHtml.append(value)
        }
    }

    /// Append `value` as escaped text into the page body (the sink `{expr}` uses).
    /// A value that is already markup goes through `append_html` instead.
    public func append_text(&mut self, value : &std::string_view) {
        appendHtmlEscaped(&mut pageHtml, value)
    }

    func append_html_integer(&mut self, value : bigint) {
        pageHtml.append_integer(value)
    }

    func append_html_uinteger(&mut self, value : ubigint) {
        pageHtml.append_uinteger(value)
    }

    func append_html_float(&mut self, value : float) {
        pageHtml.append_float(value, 3)
    }

    func append_html_double(&mut self, value : double) {
        pageHtml.append_double(value, 3)
    }    // ── Slot markers (frozen shells, §3.12) ───────────────────────────────
    // Marks a request-varying region of this page's markup. `web::Shell.compile`
    // replaces the marker with a byte range of the frozen buffer, and `shell.emit`
    // splices the per-request fill into that range — so the chrome around it is
    // rendered once and never again.
    //
    // The region holds plain text/HTML, **never a component call**: a universal
    // component's hydration ids are baked into the frozen bytes and cannot be
    // allocated per request. Structural variation (signed in vs out) is a separate
    // shell, not a branch inside one.
    //
    // The marker is an HTML comment (`<!--chx-slot:name-->`), so a shell that was
    // never compiled still produces valid, invisible markup instead of corrupt
    // bytes. A name is validated by `is_valid_slot_name`; an invalid name marks
    // nothing (a slot that silently disappears is what `Shell.compile` reports).
    public func slot(&mut self, name : std::string_view) {
        if(!is_valid_slot_name(&name)) { return }
        pageHtml.append_view("<!--chx-slot:")
        pageHtml.append_view(&name)
        pageHtml.append_view("-->")
    }

    func get_html_size(&self) : ubigint {
        return pageHtml.size();
    }

    func truncate_html(&mut self, size : ubigint) {
        pageHtml.resize(size);
    }

    public func append_head(&mut self, value : &std::str) {
        pageHead.append_view(value.to_view())
    }

    func append_head_raw(&mut self, value : *char, len : size_t) {
        pageHead.append_with_len(value, len);
    }

    func append_head_view(&mut self, value : &std::string_view) {
        pageHead.append_view(value);
    }

    func append_head_char_ptr(&mut self, value : *char) {
        pageHead.append_char_ptr(value);
    }

    func append_head_char(&mut self, value : char) {
        pageHead.append(value)
    }

    func append_head_text_char_ptr(&mut self, value : *char) {
        var view = unsafe(std::string_view(value, strlen(value)))
        appendHtmlEscaped(&mut pageHead, &view)
    }

    func append_head_text_char(&mut self, value : char) {
        switch(value) {
            '&' => pageHead.append_view("&amp;")
            '<' => pageHead.append_view("&lt;")
            '>' => pageHead.append_view("&gt;")
            '"' => pageHead.append_view("&quot;")
            '\'' => pageHead.append_view("&#39;")
            default => pageHead.append(value)
        }
    }

    func append_head_integer(&mut self, value : bigint) {
        pageHead.append_integer(value)
    }

    func append_head_uinteger(&mut self, value : ubigint) {
        pageHead.append_uinteger(value)
    }

    func append_head_float(&mut self, value : float) {
        pageHead.append_float(value, 3)
    }

    func append_head_double(&mut self, value : double) {
        pageHead.append_double(value, 3)
    }

    public func append_css(&mut self, value : &std::str) {
        var view = value.to_view()
        append_css_view(&view)
    }

    func append_css_raw(&mut self, value : *char, len : size_t) {
        unsafe {
            if(shared != null && local_css_depth == 0) { shared.css_data.append_with_len(value, len); return }
            pageCss.append_with_len(value, len);

        }
    }

    func append_css_view(&mut self, value : &std::string_view) {
        unsafe {
            if(shared != null && local_css_depth == 0) { shared.css_data.append_with_len(value.data(), value.size()); return }
            pageCss.append_with_len(value.data(), value.size())

        }
    }

    func require_css_hash(&self, hash : size_t) : bool {
        unsafe {
            if(shared != null) { return !shared.done_classes.contains(&hash) }
            return !doneClasses.contains(&hash)

        }
    }

    func set_css_hash(&mut self, hash : size_t) {
        unsafe {
            if(shared != null) { shared.done_classes.insert(hash, true); return }
            doneClasses.insert(hash, true)

        }
    }

    /// Whether anything on this page asked for a universal component, and so for
    /// the hydration runtime. False for a plain `#html` page.
    public func used_universal(&self) : bool {
        return universal_used
    }

    func require_component(&mut self, hash : size_t) : bool {
        unsafe {
            universal_used = true
            if(shared != null) {
                // Record the use even when the sink already holds the definition:
                // this call runs once per *instance*, so it is exactly how the
                // build learns which routes need this component.
                shared.note_usage(hash as ubigint, unit_owner)
                return !shared.done_components.contains(&hash)
            }
            return !doneComponents.contains(&hash)

        }
    }

    // The component emitter calls this once per definition, with the byte range
    // the definition occupies in the JS buffer after hoisting. With no shared
    // sink there is no graph to build (a page on its own has one output file).
    func record_js_unit(&mut self, hash : ubigint, name : std::string_view, start : ubigint, length : ubigint) {
        unsafe {
            if(shared != null) { shared.record_unit(hash, name, start, length) }

        }
    }

    func set_component_hash(&mut self, hash : size_t) {
        unsafe {
            if(shared != null) { shared.done_components.insert(hash, true); return }
            doneComponents.insert(hash, true)

        }
    }

    // `#globaljs` de-duplication: keyed by the macro's source location. With a
    // shared sink attached the map lives in the sink, so N pages emit the block
    // once; otherwise it lives on the page.
    func require_js_hash(&self, hash : size_t) : bool {
        unsafe {
            if(shared != null) { return !shared.done_js.contains(&hash) }
            return !doneJs.contains(&hash)

        }
    }

    func set_js_hash(&mut self, hash : size_t) {
        unsafe {
            if(shared != null) { shared.done_js.insert(hash, true); return }
            doneJs.insert(hash, true)

        }
    }

    // ── Router parameter store (§3.2) ────────────────────────────────────────
    // Keys beginning with `__route_`/`__query_` are reserved for router
    // internals (D-5.2); passing one here is a programming error.
    // Typed parameter setter: `page.add_parameter<int>("count", &count)`. The
    // value is formatted and copied into page-owned storage. Because Chemical
    // cannot infer a generic method's type parameter from the argument, the
    // type argument must be written explicitly (or use the free function
    // `page::set_parameter`, which infers).
    public func <T : PageParam> add_parameter(&mut self, key : &std::str, value : &T) {
        var text = value.to_page_param()
        add_parameter_owned(key.to_view(), text.to_view())
    }

    // Borrowed-view setter: both key and value are copied into page-owned
    // storage, so the caller's buffers may die immediately after the call.
    public func add_parameter_view(&mut self, key : std::string_view, value : std::string_view) {
        add_parameter_owned(key, value)
    }

    // Stores a parameter whose value is not backed by page-lifetime memory
    // (e.g. a freshly percent-decoded URL segment). The bytes are copied into
    // `route_param_owned`, whose node-based storage keeps the stored view valid
    // for the rest of the render.
    public func add_parameter_owned(&mut self, key : std::string_view, value : std::string_view) {
        unsafe {
            var owned_key = unsafe(std::string(key.data(), key.size()))
            var owned_value = unsafe(std::string(value.data(), value.size()))
            route_param_owned.insert(owned_key, owned_value)
            var lookup = unsafe(std::string(key.data(), key.size()))
            const kp = route_param_owned.get_key_ptr(&lookup)
            const p = route_param_owned.get_ptr(&lookup)
            if(kp == null || p == null) { return }
            parameters.insert(kp.to_view(), PageParameter.Text(p.to_view()))

        }
    }

    // Stores a parameter whose KEY is also dynamic (e.g. a percent-decoded query
    // key). `add_parameter_owned` keeps the caller's key view (the `parameters`
    // map is keyed by `string_view`), which dangles for a computed key; this
    // stores both key and value in page-owned storage and is read back with
    // `get_owned_parameter`.
    public func add_owned_parameter(&mut self, key : std::string_view, value : std::string_view) {
        route_param_owned.insert(unsafe(std::string(key.data(), key.size())), unsafe(std::string(value.data(), value.size())))
    }

    public func get_owned_parameter(&self, key : std::string_view) : std::string_view {
        unsafe {
            var lookup = unsafe(std::string(key.data(), key.size()))
            const p = route_param_owned.get_ptr(&lookup)
            if(p == null) { return std::string_view() }
            return p.to_view()

        }
    }

    // Presence check for the owned store (keys written by
    // `add_parameter_owned`/`add_owned_parameter`).
    public func has_owned_parameter(&self, key : std::string_view) : bool {
        unsafe {
            var lookup = unsafe(std::string(key.data(), key.size()))
            return route_param_owned.contains(&lookup)
        }
    }

    // Raw object-parameter write. Kept as a public raw method for
    // `router::raw`; `raw::add_parameter_object` forwards to this.
    public unsafe func add_parameter_object_raw(&mut self, key : std::string_view, ptr : *mut void) {
        parameters.insert(key, PageParameter.Object(ptr))
    }

    // Raw object-parameter lookup (internal to `page`/`router`). Returns the
    // stored pointer without a cast; callers must know the concrete type.
    public func get_parameter_object_ptr(&self, key : std::string_view) : *mut void {
        unsafe {
            const p = parameters.get_ptr(&key)
            if(p == null) { return null }
            if(p is PageParameter.Object) {
                var Object(ptr) = *p else return null
                return ptr
            }
            return null
        }
    }



    public func get_parameter(&self, key : std::string_view) : std::string_view { unsafe {
        const p = parameters.get_ptr(&key)
        if(p == null) { return std::string_view() }
        if(p is PageParameter.Text) {
            var Text(view) = *p else unsafe { unreachable }
            return view
        }
        return std::string_view()
     }}

    // The parameter value as an `SsrText`, so it can be injected as an SSR
    // attribute (a `std::string_view` value has no direct attribute
    // representation; `SsrText` does, via `getSsrAttributeValue`). Used by the
    // generated router to pass `{param}` values to a route body (D-2.7).
    public func get_parameter_text(&self, key : std::string_view) : SsrText {
        const v = self.get_parameter(key)
        return SsrText { data : v.data(), size : v.size() as u64 }
    }

    // ── Phase 7: static-route SSR snapshot cache ─────────────────────────────
    // Appends the cached snapshot for `key`; returns false on a miss (the caller
    // renders and then stores). Reads are guarded by the process-global mutex.
    public func route_snapshot_append(&mut self, key : std::string_view) : bool {
        unsafe {
            if(!route_snapshots_enabled) { return false }
            ensure_route_snapshots()
            var k = route_snapshot_key(key)
            var lk = std::lock_guard(&mut route_snapshots_mutex)
            const p = route_snapshots.get_ptr(&k)
            if(p == null) { return false }
            // Read through the pointer (no copy: a copy of a `std::string` field
            // would allocate on every hit).
            pageHtml.append_with_len(p.data.data(), p.data.size())
            route_snapshot_hits = route_snapshot_hits + 1
            return true

        }
    }

    // Stores `pageHtml[start..]` as this route's snapshot. Published once; later
    // `route_snapshot_append` calls copy the bytes.
    public func route_snapshot_store(&mut self, key : std::string_view, start : ubigint) {
        unsafe {
            if(!route_snapshots_enabled) { return }
            ensure_route_snapshots()
            const len = pageHtml.size() - start
            var k = route_snapshot_key(key)
            var lk = std::lock_guard(&mut route_snapshots_mutex)
            route_snapshots.insert(k, RouteSnapshot { data : unsafe(std::string(pageHtml.data() + start, len)) })
            route_snapshot_misses = route_snapshot_misses + 1

        }
    }

    public func route_snapshot_hits_count(&self) : ubigint {
        unsafe {     return route_snapshot_hits
        }
    }
    public func route_snapshot_misses_count(&self) : ubigint {
        unsafe {     return route_snapshot_misses
        }
    }
    // Drops every cached snapshot. `clear()` runs each entry's destructor, so
    // the owned bytes are freed (no leak), unlike the previous `*char` handle.
    public func reset_route_snapshots(&mut self) {
        unsafe {
            ensure_route_snapshots()
            var lk = std::lock_guard(&mut route_snapshots_mutex)
            route_snapshots.clear()
            route_snapshot_hits = 0
            route_snapshot_misses = 0

        }
    }

    public func has_parameter(&self, key : std::string_view) : bool {
        return parameters.contains(&key)
    }

    public func mark_route_missing(&mut self) {
        route_missing_flag = true
    }

    public func route_missing(&self) : bool {
        return route_missing_flag
    }

    public func add_route_pattern(&mut self, router : std::string_view,
                                  pattern : std::string_view, id : std::string_view,
                                  is_fallback : bool) {
        route_manifest.push(RouteManifestEntry {
            router : router,
            pattern : pattern,
            id : id,
            is_fallback : is_fallback
        })
    }

    // Serializes the collected route manifest for static export
    // (`<name>.routes.json`). Routers appear in first-declaration order, routes
    // in declaration order. The document form (template 6, §15.3) additionally
    // carries the exporting page's name and base path.
    public func route_manifest_json(&self) : std::string {
        var out = std::string()
        out.append_view(std::string_view("{\"routers\":"))
        self.route_manifest_routers_json(&mut out)
        out.append_view(std::string_view("}"))
        return out
    }

    public func route_manifest_document(&self, name : std::string_view, base : std::string_view) : std::string {
        var out = std::string()
        out.append_view(std::string_view("{\"name\":\""))
        route_json_escape(name, &mut out)
        out.append_view(std::string_view("\",\"base\":\""))
        route_json_escape(base, &mut out)
        out.append_view(std::string_view("\",\"routers\":"))
        self.route_manifest_routers_json(&mut out)
        out.append_view(std::string_view("}"))
        return out
    }

    // The `[ { name, routes: [...] }, ... ]` array body.
    func route_manifest_routers_json(&self, out : &mut std::string) {
        unsafe {
            out.append_view(std::string_view("["))
            var emitted = std::unordered_map<std::string_view, bool>()
            var firstRouter = true
            for(var i : size_t = 0; i < route_manifest.size(); i++) {
                const entry = route_manifest.get_ptr(i)
                if(emitted.contains(&entry.router)) { continue }
                emitted.insert(entry.router, true)
                if(!firstRouter) { out.append(',') }
                firstRouter = false
                out.append_view(std::string_view("{\"name\":\""))
                route_json_escape(entry.router, out)
                out.append_view(std::string_view("\",\"routes\":["))
                var firstRoute = true
                for(var j : size_t = 0; j < route_manifest.size(); j++) {
                    const r = route_manifest.get_ptr(j)
                    if(!r.router.equals(&entry.router)) { continue }
                    if(!firstRoute) { out.append(',') }
                    firstRoute = false
                    out.append_view(std::string_view("{\"id\":\""))
                    route_json_escape(r.id, out)
                    out.append_view(std::string_view("\",\"pattern\":"))
                    if(r.is_fallback) {
                        out.append_view(std::string_view("null"))
                    } else {
                        out.append('"')
                        route_json_escape(r.pattern, out)
                        out.append('"')
                    }
                    out.append_view(std::string_view(",\"fallback\":"))
                    if(r.is_fallback) { out.append_view(std::string_view("true")) }
                    else { out.append_view(std::string_view("false")) }
                    out.append('}')
                }
                out.append_view(std::string_view("]}"))
            }
            out.append_view(std::string_view("]"))

        }
    }

    // Appends raw JS to `pageJsEnd`, which the runtime emits *after*
    // `$__universal_flush()` (D-7.4). Used by the generated router code for the
    // initial activation tail.
    public func append_js_end(&mut self, value : &std::str) {
        pageJsEnd.append_view(value.to_view())
    }

    func append_js_end_raw(&mut self, value : *char, len : size_t) {
        pageJsEnd.append_with_len(value, len);
    }

    // Server-side route selection (§3.4): the request parameter wins; otherwise
    // the declared default route. Used by the generated wrapper to render the
    // selected route already visible (no flash), and by the activation tail.
    public func route_selected(&self, router : std::string_view, id : std::string_view,
                               fallback : std::string_view) : bool {
        const sel = self.get_parameter(router)
        if(sel.size() > 0) { return sel.equals(&id) }
        return fallback.size() > 0 && id.equals(&fallback)
    }

    // Appends the initial `$__uni_activate_initial(...)` (and, for URL routers,
    // `$__uni_sync_url(...)`) to `pageJsEnd`, after `$__universal_flush()`.
    // Reads the server-selected route parameter, falling back to `fallback_id`.
    // Emits nothing when no route could be selected (the page renders inert).
    public func append_router_initial_activation(&mut self, router : std::string_view,
                                                 fallback_id : std::string_view,
                                                 sync_url : bool) {
        var id = self.get_parameter(router)
        if(id.size() == 0) { id = fallback_id }
        if(id.size() == 0) { return }
        pageJsEnd.append_view(std::string_view("window.$__uni_activate_initial(\""))
        append_router_js_quoted(router, &mut pageJsEnd)
        pageJsEnd.append_view(std::string_view("\", \""))
        append_router_js_quoted(id, &mut pageJsEnd)
        pageJsEnd.append_view(std::string_view("\", window.$__uni_initial_url());\n"))
        if(sync_url) {
            pageJsEnd.append_view(std::string_view("window.$__uni_sync_url(\""))
            append_router_js_quoted(router, &mut pageJsEnd)
            pageJsEnd.append_view(std::string_view("\");\n"))
        }
    }

    // Server-only: the app may serve the page under a mount prefix
    // (`set_route_url(path, "/app")`). The client match table is compile-time and
    // cannot contain it, so write it into the table before the activation tail
    // (§15.9-Q42). Emits nothing when no base was set.
    public func append_router_table_base(&mut self, router : std::string_view) {
        const base = self.get_parameter(std::string_view("__route_base"))
        if(base.size() == 0) { return }
        pageJsEnd.append_view(std::string_view("window.$__uni_set_table_base(\""))
        append_router_js_quoted(router, &mut pageJsEnd)
        pageJsEnd.append_view(std::string_view("\", \""))
        append_router_js_quoted(base, &mut pageJsEnd)
        pageJsEnd.append_view(std::string_view("\");\n"))
    }

    // Server-only: emits the selected route's `<title>` into the head when it
    // declares one (§13.2.1). A client-side navigation only updates
    // `document.title` (the rest of the head is not reactive in v1).
    public func emit_route_title(&mut self, router : std::string_view, id : std::string_view,
                                 fallback : std::string_view, title : std::string_view) {
        if(title.size() == 0) { return }
        if(!self.route_selected(router, id, fallback)) { return }
        const esc = escape_html_view(title)
        pageHead.append_view(std::string_view("<title>"))
        pageHead.append_view(esc.to_view())
        pageHead.append_view(std::string_view("</title>"))
    }

    // Typed head metadata. Calling it again **replaces** the previously emitted
    // block instead of appending a second `<title>`, so a layout can set a default
    // and a page can override it (last call wins).
    public func set_head(&mut self, head : &PageHead) {
        if(head_mark_valid) { pageHead.resize(head_mark_start) }
        head_mark_start = pageHead.size()
        head_mark_valid = true

        if(head.title.size() > 0) {
            const esc = escape_html_view(head.title)
            pageHead.append_view(std::string_view("<title>"))
            pageHead.append_view(esc.to_view())
            pageHead.append_view(std::string_view("</title>"))
        }
        if(head.description.size() > 0) {
            const esc = escape_html_view(head.description)
            pageHead.append_view(std::string_view("<meta name=\"description\" content=\""))
            pageHead.append_view(esc.to_view())
            pageHead.append_view(std::string_view("\">"))
        }
        if(head.canonical.size() > 0) {
            const esc = escape_html_view(head.canonical)
            pageHead.append_view(std::string_view("<link rel=\"canonical\" href=\""))
            pageHead.append_view(esc.to_view())
            pageHead.append_view(std::string_view("\">"))
        }
        if(head.noindex) {
            pageHead.append_view(std::string_view("<meta name=\"robots\" content=\"noindex\">"))
        }
    }

    // Server-only: a 404 (`route_missing`) body must not be indexed; a matched
    // fallback route should likewise not be indexed as if it were the real page
    // (§13.2.2).
    public func emit_route_noindex(&mut self) {
        if(!route_missing_flag) { return }
        pageHead.append_view(std::string_view("<meta name=\"robots\" content=\"noindex\">"))
    }

    func require_random_css_hash(&self, hash : size_t) : bool {
        unsafe {
            if(shared != null) { return !shared.done_random_classes.contains(&hash) }
            return !doneRandomClasses.contains(&hash)

        }
    }

    func set_random_css_hash(&mut self, hash : size_t) {
        unsafe {
            if(shared != null) { shared.done_random_classes.insert(hash, true); return }
            doneRandomClasses.insert(hash, true)

        }
    }

    func append_css_char_ptr(&mut self, value : *char) {
        unsafe {
            if(shared != null && local_css_depth == 0) { shared.css_data.append_char_ptr(value); return }
            pageCss.append_char_ptr(value);

        }
    }

    func append_css_char(&mut self, value : char) {
        unsafe {
            if(shared != null && local_css_depth == 0) { shared.css_data.append(value); return }
            pageCss.append(value)

        }
    }

    func append_css_integer(&mut self, value : bigint) {
        unsafe {
            if(shared != null && local_css_depth == 0) { shared.css_data.append_integer(value); return }
            pageCss.append_integer(value)

        }
    }

    func append_css_uinteger(&mut self, value : ubigint) {
        unsafe {
            if(shared != null && local_css_depth == 0) { shared.css_data.append_uinteger(value); return }
            pageCss.append_uinteger(value)

        }
    }

    func append_css_float(&mut self, value : float) {
        unsafe {
            if(shared != null && local_css_depth == 0) { shared.css_data.append_float(value, 3); return }
            pageCss.append_float(value, 3)

        }
    }

    func append_css_double(&mut self, value : double) {
        unsafe {
            if(shared != null && local_css_depth == 0) { shared.css_data.append_double(value, 3); return }
            pageCss.append_double(value, 3)

        }
    }

    public func append_js(&mut self, value : &std::str) {
        var view = value.to_view()
        unsafe {
            if(shared != null && local_js_depth == 0) { shared.js_data.append_with_len(view.data(), view.size()); return }
            pageJs.append_with_len(view.data(), view.size());
        }
    }

    func append_js_raw(&mut self, value : *char, len : size_t) {
        unsafe {
            if(shared != null && local_js_depth == 0) { shared.js_data.append_with_len(value, len); return }
            pageJs.append_with_len(value, len);

        }
    }

    // Serializes a `vector` of strings (or string views) as a JSON array of JS
    // string literals, for **aggregate props**: `<List items={fruits} />` used to
    // fail the build ("its type has no JS serialization"), so a component that
    // rendered a list on the server could not receive that list on the client.
    // Escaping is the scalar-string rule, so SSR and hydration read the same value
    // back. Aggregates without a JSON spelling (`vector<MyStruct>`) are still
    // rejected at conversion time rather than silently dropped.
    public func append_js_string_array(&mut self, items : &std::vector<std::string>) {
        unsafe {
            var out = std::string()
            out.append('[')
            var i : size_t = 0
            while(i < items.size()) {
                if(i > 0) { out.append(',') }
                const item = items.get_ptr(i)
                const view = item.to_view()
                out.append('\'')
                append_js_single_quoted_escaped(&mut out, view.data(), view.size())
                out.append('\'')
                i++
            }
            out.append(']')
            self.append_js_raw(out.data(), out.size())
        }
    }

    public func append_js_string_view_array(&mut self, items : &std::vector<std::string_view>) {
        unsafe {
            var out = std::string()
            out.append('[')
            var i : size_t = 0
            while(i < items.size()) {
                if(i > 0) { out.append(',') }
                const item = items.get_ptr(i)
                out.append('\'')
                append_js_single_quoted_escaped(&mut out, item.data(), item.size())
                out.append('\'')
                i++
            }
            out.append(']')
            self.append_js_raw(out.data(), out.size())
        }
    }

    func append_js_char_ptr(&mut self, value : *char) {
        unsafe {
            if(shared != null && local_js_depth == 0) { shared.js_data.append_char_ptr(value); return }
            pageJs.append_char_ptr(value);

        }
    }

    // Escapes the string for embedding inside a JS string literal: quotes,
    // backslashes, newlines/control chars and the `</` sequence that could
    // otherwise break out of an inline <script> block.
    func append_js_escaped_char_ptr(&mut self, value : *char) {
        unsafe {
            const view = unsafe(std::string_view(value, unsafe(strlen(value))))
            if(shared != null && local_js_depth == 0) { appendJsEscaped(&mut shared.js_data, &view); return }
            appendJsEscaped(&mut pageJs, &view)

        }
    }

    func append_js_escaped(&mut self, value : *char, len : size_t) {
        unsafe {
            const view = unsafe(std::string_view(value, len))
            if(shared != null && local_js_depth == 0) { appendJsEscaped(&mut shared.js_data, &view); return }
            appendJsEscaped(&mut pageJs, &view)

        }
    }

    func append_js_char(&mut self, value : char) {
        unsafe {
            if(shared != null && local_js_depth == 0) { shared.js_data.append(value); return }
            pageJs.append(value)

        }
    }

    func append_js_integer(&mut self, value : bigint) {
        unsafe {
            if(shared != null && local_js_depth == 0) { shared.js_data.append_integer(value); return }
            pageJs.append_integer(value)

        }
    }

    func append_js_uinteger(&mut self, value : ubigint) {
        unsafe {
            if(shared != null && local_js_depth == 0) { shared.js_data.append_uinteger(value); return }
            pageJs.append_uinteger(value)

        }
    }

    func append_js_float(&mut self, value : float) {
        unsafe {
            if(shared != null && local_js_depth == 0) { shared.js_data.append_float(value, 3); return }
            pageJs.append_float(value, 3)

        }
    }

    func append_js_double(&mut self, value : double) {
        unsafe {
            if(shared != null && local_js_depth == 0) { shared.js_data.append_double(value, 3); return }
            pageJs.append_double(value, 3)

        }
    }

    // Move the byte range [fromStart, fromEnd) so that it begins at `index`,
    // shifting whatever lies between the two.
    //
    // A free function over a `string` rather than a method on the page, because
    // the range being moved lives in DIFFERENT buffers depending on whether a
    // shared sink is attached (see get_js_pos below). The memmove body is the
    // same either way; only the buffer changes.
    public func move_js_range_in(buf : &mut std::string, fromStart : ubigint, fromEnd : ubigint, index : ubigint) {
        if (fromStart >= fromEnd || fromEnd > buf.size() || index > buf.size()) return;
        if (index >= fromStart && index <= fromEnd) return;

        buf.reserve(buf.size());

        var range_len = fromEnd - fromStart;
        var p_buf = buf.mutable_data();

        var stack_buf : [1024]char;
        var p_stack = &mut stack_buf[0];

        if (index < fromStart) {
            var m_start = index;
            var m_len = fromStart - index;
            if (m_len <= range_len) {
                var temp = if (m_len <= 1024) (p_stack as *mut char) else (unsafe(malloc(m_len)) as *mut char);
                unsafe { memcpy(temp as *mut void, (p_buf + m_start) as *mut void, m_len) };
                unsafe { memmove((p_buf + m_start) as *mut void, (p_buf + fromStart) as *mut void, range_len) };
                unsafe { memcpy((p_buf + m_start + range_len) as *mut void, temp as *mut void, m_len) };
                if (m_len > 1024) unsafe { free(temp as *mut void) };
            } else {
                var temp = if (range_len <= 1024) (p_stack as *mut char) else (unsafe(malloc(range_len)) as *mut char);
                unsafe { memcpy(temp as *mut void, (p_buf + fromStart) as *mut void, range_len) };
                unsafe { memmove((p_buf + m_start + range_len) as *mut void, (p_buf + m_start) as *mut void, m_len) };
                unsafe { memcpy((p_buf + m_start) as *mut void, temp as *mut void, range_len) };
                if (range_len > 1024) unsafe { free(temp as *mut void) };
            }
        } else {
            var m_start = fromEnd;
            var m_len = index - fromEnd;
            if (range_len <= m_len) {
                var temp = if (range_len <= 1024) (p_stack as *mut char) else (unsafe(malloc(range_len)) as *mut char);
                unsafe { memcpy(temp as *mut void, (p_buf + fromStart) as *mut void, range_len) };
                unsafe { memmove((p_buf + fromStart) as *mut void, (p_buf + m_start) as *mut void, m_len) };
                unsafe { memcpy((p_buf + fromStart + m_len) as *mut void, temp as *mut void, range_len) };
                if (range_len > 1024) unsafe { free(temp as *mut void) };
            } else {
                var temp = if (m_len <= 1024) (p_stack as *mut char) else (unsafe(malloc(m_len)) as *mut char);
                unsafe { memcpy(temp as *mut void, (p_buf + m_start) as *mut void, m_len) };
                unsafe { memmove((p_buf + fromStart + m_len) as *mut void, (p_buf + fromStart) as *mut void, range_len) };
                unsafe { memcpy((p_buf + fromStart) as *mut void, temp as *mut void, range_len) };
                if (m_len > 1024) unsafe { free(temp as *mut void) };
            }
        }
    }

    // The end of the buffer a component definition was actually appended to.
    //
    // With a shared sink attached, page-independent output - component
    // definitions above all - is routed into the SINK, not into `pageJs`. A
    // definition's hoist position is therefore an offset into the sink's
    // `js_data`, and measuring it against `pageJs` measures the wrong buffer.
    //
    // That is not a small mismatch. `move_js_range` bounds-checks its arguments
    // and returns silently when they are out of range, so with a sink attached
    // the hoist became a no-op, the definition stayed inline in the middle of
    // the `createElement(...)` argument list it was emitted into, and the whole
    // client bundle failed to parse. Every page that attaches a sink and renders
    // a component which uses another component was affected.
    func get_js_pos(&self) : ubigint {
        unsafe {
            if (self.shared != null) { return self.shared.js_data.size(); }
            return pageJs.size();

        }
    }

    func move_js_range(&mut self, fromStart : ubigint, fromEnd : ubigint, index : ubigint) {
        unsafe {
            // Same reasoning as get_js_pos: move inside the buffer the bytes are in.
            if (self.shared != null) { move_js_range_in(&mut self.shared.js_data, fromStart, fromEnd, index); return; }
            move_js_range_in(&mut self.pageJs, fromStart, fromEnd, index);

        }
    }

    func append_head_js(&mut self, value : *char, len : size_t) {
        pageHeadJs.append_with_len(value, len);
    }

    func append_head_js_char_ptr(&mut self, value : *char) {
        pageHeadJs.append_char_ptr(value);
    }

    // Appends raw JS to `pageHeadJs`, which `toString` emits as its OWN
    // `<script>` inside `<head>` -- before `pageHtml` and before the body JS
    // bundle. A test harness (see lang/libs/universal_test) uses this so it
    // lives in a separate script element from the page bundle it drives: a
    // script that fails to parse is skipped in its entirety, so anything
    // sharing its block (including the harness's own test registrations) is
    // lost with it, while the head block has already run and can still report.
    //
    // Always writes to this page's own head sink, never to a shared one.
    public func append_head_js_view(&mut self, value : &std::string_view) {
        pageHeadJs.append_view(value);
    }

    func append_head_js_char(&mut self, value : char) {
        pageHeadJs.append(value)
    }

    func append_head_js_integer(&mut self, value : bigint) {
        pageHeadJs.append_integer(value)
    }

    func append_head_js_uinteger(&mut self, value : ubigint) {
        pageHeadJs.append_uinteger(value)
    }

    func append_head_js_float(&mut self, value : float) {
        pageHeadJs.append_float(value, 3)
    }

    func append_head_js_double(&mut self, value : double) {
        pageHeadJs.append_double(value, 3)
    }

    func appendHtmlTagStart(str : &mut std::string, lang : std::string_view = "", htmlClass : std::string_view = "") {
        if(htmlClass.empty() && lang.empty()) {
            str.append_view("<html>")
        } else if(htmlClass.empty()) {
            str.append_view("<html lang=\"")
            str.append_view(&lang)
            str.append_view("\">")
        } else if(lang.empty()) {
            str.append_view("<html class=\"")
            str.append_view(&htmlClass)
            str.append_view("\">")
        } else {
            str.append_view("<html lang=\"")
            str.append_view(&lang)
            str.append_view("\" class=\"")
            str.append_view(&htmlClass)
            str.append_view("\">")
        }
    }

    func appendBodyTagStart(str : &mut std::string, bodyClass : std::string_view = "") {
        if(bodyClass.empty()) {
            str.append_view("<body>")
        } else {
            str.append_view("<body class=\"")
            str.append_view(&bodyClass)
            str.append_view("\">")
        }
    }

    func toString(&self, lang : std::string_view = "", htmlClass : std::string_view = "", bodyClass : std::string_view = "") : std::string {
        var str = std::string()
        str.reserve(pageHead.size() + pageCss.size() + pageHtml.size() + pageHeadJs.size() + pageJs.size() + 100)
        str.append_view(std::string_view("<!DOCTYPE html>"))
        appendHtmlTagStart(&mut str, lang, htmlClass)
        str.append_view("<head>")
        str.append_string(&pageHead)
        if(!pageCss.empty()) {
            str.append_view(std::string_view("<style>"))
            append_output_css(&mut str)
            str.append_view(std::string_view("</style>"))
        }
        if(!pageHeadJs.empty()) {
            str.append_view(std::string_view("<script>"))
            append_output_head_js(&mut str)
            str.append_view(std::string_view("</script>"))
        }
        str.append_view(std::string_view("</head>"))
        appendBodyTagStart(&mut str, bodyClass)
        str.append_string(&pageHtml)
        var finalizedJs = getFinalizedPageJs()
        if(!finalizedJs.empty()) {
            str.append_view(std::string_view("<script>"))
            str.append_string(&finalizedJs)
            str.append_view(std::string_view("</script>"))
        }
        str.append_view(std::string_view("</body></html>"))
        return str;
    }

    // Like `toString`, but the page's JS is NOT inlined: `externalScript` (a
    // complete `<script src=…></script>` tag, or "") is written in its place.
    // Used by an app that serves its compiled client JS as a cacheable asset
    // instead of re-sending it with every response.
    public func toStringExternalJs(&self, externalScript : std::string_view, lang : std::string_view = "", htmlClass : std::string_view = "", bodyClass : std::string_view = "") : std::string {
        var str = std::string()
        str.reserve(pageHead.size() + pageCss.size() + pageHtml.size() + pageHeadJs.size() + externalScript.size() + 100)
        str.append_view(std::string_view("<!DOCTYPE html>"))
        appendHtmlTagStart(&mut str, lang, htmlClass)
        str.append_view("<head>")
        str.append_string(&pageHead)
        if(!pageCss.empty()) {
            str.append_view(std::string_view("<style>"))
            append_output_css(&mut str)
            str.append_view(std::string_view("</style>"))
        }
        if(!pageHeadJs.empty()) {
            str.append_view(std::string_view("<script>"))
            append_output_head_js(&mut str)
            str.append_view(std::string_view("</script>"))
        }
        str.append_view(std::string_view("</head>"))
        appendBodyTagStart(&mut str, bodyClass)
        str.append_string(&pageHtml)
        str.append_view(&externalScript)
        str.append_view(std::string_view("</body></html>"))
        return str;
    }

    // Like `toString`, but with externally served JS:
    //   - `headAssets` (complete tags: a shared bundle's `<link>` / `<script src>`)
    //     is written into <head> before the page's own `<style>`/`<script>`;
    //   - when `bodyScriptSrc` is non-empty the page's own JS is *not* inlined: a
    //     `<script src>` for it is written at the end of <body> instead.
    //
    // Why the shared bundle is a classic script in <head>: the page's own script
    // (inline or `src`) runs at the end of <body> and calls the hydration runtime
    // the bundle defines. A `type="module"` script would be deferred past it.
    public func toStringWithAssets(&self, headAssets : std::string_view, bodyScriptSrc : std::string_view, lang : std::string_view = "", htmlClass : std::string_view = "", bodyClass : std::string_view = "") : std::string {
        var str = std::string()
        str.reserve(pageHead.size() + pageCss.size() + pageHtml.size() + pageHeadJs.size() + pageJs.size() + headAssets.size() + bodyScriptSrc.size() + 100)
        str.append_view(std::string_view("<!DOCTYPE html>"))
        appendHtmlTagStart(&mut str, lang, htmlClass)
        str.append_view("<head>")
        str.append_string(&pageHead)
        if(!headAssets.empty()) {
            str.append_view(&headAssets)
        }
        if(!pageCss.empty()) {
            str.append_view(std::string_view("<style>"))
            append_output_css(&mut str)
            str.append_view(std::string_view("</style>"))
        }
        if(!pageHeadJs.empty()) {
            str.append_view(std::string_view("<script>"))
            append_output_head_js(&mut str)
            str.append_view(std::string_view("</script>"))
        }
        str.append_view(std::string_view("</head>"))
        appendBodyTagStart(&mut str, bodyClass)
        str.append_string(&pageHtml)
        if(!bodyScriptSrc.empty()) {
            str.append_view(std::string_view("<script src=\""))
            str.append_view(&bodyScriptSrc)
            str.append_view(std::string_view("\"></script>"))
        } else {
            var finalizedJs = getFinalizedPageJs()
            if(!finalizedJs.empty()) {
                str.append_view(std::string_view("<script>"))
                str.append_string(&finalizedJs)
                str.append_view(std::string_view("</script>"))
            }
        }
        str.append_view(std::string_view("</body></html>"))
        return str;
    }

    func toStringHeadOnly(&self) : std::string {
        return pageHead.copy();
    }

    func toStringHeadJsOnly(&self) : std::string {
        return pageHeadJs.copy();
    }

    func toStringHtmlOnly(&self) : std::string {
        var str = std::string()
        str.reserve(pageHtml.size())
        str.append_string(&pageHtml)
        return str;
    }

    func toStringCssOnly(&self) : std::string {
        var str = std::string()
        str.reserve(pageCss.size())
        str.append_string(&pageCss)
        return str;
    }

    func toStringJsOnly(&self) : std::string {
        return pageJs.copy()
    }

    // The per-page JS tail (activation, initial URL). Not cacheable: it carries
    // this request's selected route. `render_page` writes it inline after the
    // external framework script.
    public func toStringJsEndOnly(&self) : std::string {
        return pageJsEnd.copy()
    }

    func appendTitle(&mut self, view : &std::string_view) {
        pageHead.append_view("<title>")
        pageHead.append_view(view)
        pageHead.append_view("</title>")
    }

    func appendFavicon(&mut self, type : &std::string_view, path : &std::string_view) {
        pageHead.append_view("<link rel=\"icon\" type=\"")
        pageHead.append_view(type)
        pageHead.append_view("\" href=\"")
        pageHead.append_view(path)
        pageHead.append_view("\">")
    }

    func appendPngFavicon(&mut self, path : &std::string_view) {
        appendFavicon(std::string_view("image/png"), path)
    }

    func appendViewportMeta(&mut self) {
        pageHead.append_view("""<meta name="viewport" content="width=device-width, initial-scale=1.0">""")
    }

    func appendCharsetUTF8Meta(&mut self) {
        pageHead.append_view("""<meta charset="utf-8">""")
    }

    func defaultPrepare(&mut self) {
        appendCharsetUTF8Meta();
        appendViewportMeta();
        // Default no-op favicon so the browser does not auto-request
        // /favicon.ico (a 404 with no asset). A page that calls appendFavicon
        // later appends its own link, which takes precedence.
        pageHead.append_view(std::string_view("<link rel=\"icon\" href=\"data:,\">"));
    }

    func defaultUniversalSetup(&mut self) {
        unsafe {
            // With a shared sink attached the runtime is written into it once for
            // the whole page set; later pages only get their per-page flush tail.
            if(shared != null && shared.runtime_emitted) {
                pageJsEnd.append_view(std::string_view("window.$__universal_flush();"))
                return
            }
            if(shared != null) { shared.runtime_emitted = true }

            // The universal hydration runtime is JavaScript, not Chemical: it is
            // embedded from `runtime/*.js` at compile time (paths resolve against
            // this file's directory), so it stays editable, syntax-highlighted and
            // `node --check`-able, and nothing in this module has to carry a
            // two-thousand-line string literal through the front end.
            var runtime_css_text = std::string(intrinsics::embed_file("runtime/universal.css"))
            const runtime_css_view = runtime_css_text.to_view()
            runtime_css(&runtime_css_view)
            // we must not put anything else in the head js
            // everything else must go into body js
            var runtime_head_text = std::string(intrinsics::embed_file("runtime/universal_head.js"))
            const runtime_head_view = runtime_head_text.to_view()
            runtime_head_js(&runtime_head_view)
            var runtime_body_text = std::string(intrinsics::embed_file("runtime/universal.js"))
            const runtime_body_view = runtime_body_text.to_view()
            runtime_js(&runtime_body_view)
            // The per-page flush runs after every dispatch on the page.
            pageJsEnd.append_view(std::string_view("window.$__universal_flush();"))
        }
    }

    // Emits the universal router runtime and its one hide rule, once per page,
    // only when a page actually declares a router (§15.2, INV-18). The router
    // emitter calls this before emitting registry/stub/tail code.
    public func ensure_router_runtime(&mut self) {
        unsafe {
            if(router_runtime_emitted) { return }
            router_runtime_emitted = true
            var rr = router_runtime_js()
            if(shared != null) {
                shared.css_data.append_view(std::string_view(".chx-route[data-uni-route-active=\"false\"]{display:none !important;}"))
                shared.js_data.append_view(&rr)
            } else {
                pageCss.append_view(std::string_view(".chx-route[data-uni-route-active=\"false\"]{display:none !important;}"))
                pageJs.append_view(&rr)
            }

        }
    }

    func getFinalizedPageJs(&self) : std::string {
        var str = std::string();
        str.reserve(pageJs.size() + pageJsEnd.size())
        str.append_view(pageJs.to_view())
        str.append_view(pageJsEnd.to_view())
        if(minify_output) {
            return minify_js(str.to_view())
        }
        return str;
    }

    // given name -> {name}.css, {name}_head.js, {name}.js assets are assumed to exist
    func htmlPageToString(&self, name : &std::string_view, lang : std::string_view = "", htmlClass : std::string_view = "", bodyClass : std::string_view = "") : std::string {
        var str = std::string()
        str.reserve(pageHead.size() + pageHtml.size() + 128)
        str.append_view(std::string_view("<!DOCTYPE html>"))
        appendHtmlTagStart(&mut str, lang, htmlClass)
        str.append_view("<head>")
        str.append_string(&pageHead)
        if(!pageCss.empty()) {
            str.append_view(std::string_view("<link rel=\"stylesheet\" href=\""));
            str.append_view(name)
            str.append_view(std::string_view(".css\">"));
        }
        if(!pageHeadJs.empty()) {
            str.append_view(std::string_view("<script src=\""));
            str.append_view(name)
            str.append_view(std::string_view("_head.js\"></script>"));
        }
        str.append_view(std::string_view("</head>"))
        appendBodyTagStart(&mut str, bodyClass)
        str.append_string(&pageHtml)
        if(!pageJs.empty()) {
            str.append_view(std::string_view("<script src=\""));
            str.append_view(name)
            str.append_view(std::string_view(".js\"></script>"));
        }
        str.append_view(std::string_view("</body></html>"))
        return str;
    }

    func writeToFile(&self, path : &std::string_view) {
        var completePage = toString();
        unsafe(fs::raw::write_text_file(path.data(), completePage.data() as *u8, completePage.size()))
    }

    /// Deprecated: writing one page on its own duplicates the hydration runtime and
    /// every shared component in every file. Build the pages through
    /// `page.StaticSite` (or `web::Static` / `web::App`) instead: they render first,
    /// freeze one content-hashed bundle, and then write HTML that points at it.
    /// Kept as a shim for one release.
    @deprecated("use page.StaticSite or web::Static: per-page output duplicates the shared bundle")
    // given name -> {name}.css, {name}_head.js, {name}.js assets maybe generated
    func writeToDirectory(&self, path : &std::string_view, name : &std::string_view, lang : std::string_view = "", htmlClass : std::string_view = "", bodyClass : std::string_view = "") {

        // TODO only if not exists
        unsafe(fs::raw::mkdir(path.data()));

        // creating the route file at
        var htmlFile = unsafe(std::string(path.data(), path.size()))
        htmlFile.append('/');
        htmlFile.append_view(name);
        htmlFile.append_char_ptr(".html")

        // writing only html to route
        var htmlPage = htmlPageToString(name, lang, htmlClass, bodyClass)
        unsafe(fs::raw::write_text_file(htmlFile.data(), htmlPage.data() as *u8, htmlPage.size()))

        // {name}.css
        if(!pageCss.empty()) {
            // The bytes written are the serialized form (minified when enabled),
            // never the render buffer: a page may write itself more than once.
            var css_out = std::string()
            append_output_css(&mut css_out)
            // mutated through `append` below, so must be a `var`
            var cssFile = unsafe(std::string(path.data(), path.size()))
            cssFile.append('/');
            cssFile.append_view(name)
            cssFile.append_view(".css")
            unsafe(fs::raw::write_text_file(cssFile.data(), css_out.data() as *u8, css_out.size()))
        }

        // {name}_head.js
        if(!pageHeadJs.empty()) {
            var head_js_out = std::string()
            append_output_head_js(&mut head_js_out)
            var jsHeadFile = unsafe(std::string(path.data(), path.size()))
            jsHeadFile.append('/');
            jsHeadFile.append_view(name)
            jsHeadFile.append_view("_head.js")
            unsafe(fs::raw::write_text_file(jsHeadFile.data(), head_js_out.data() as *u8, head_js_out.size()))
        }

        // {name}.js
        var finalizedJs = getFinalizedPageJs()
        if(!finalizedJs.empty()) {
            var jsFile = unsafe(std::string(path.data(), path.size()))
            jsFile.append('/');
            jsFile.append_view(name)
            jsFile.append_view(".js")
            unsafe(fs::raw::write_text_file(jsFile.data(), finalizedJs.data() as *u8, finalizedJs.size()))
        }

        // {name}.routes.json — static-export deep-link manifest (§13.2.3)
        if(route_manifest.size() > 0) {
            var manifest = route_manifest_document(*name, std::string_view(""))
            var manifestFile = unsafe(std::string(path.data(), path.size()))
            manifestFile.append('/');
            manifestFile.append_view(name)
            manifestFile.append_view(".routes.json")
            unsafe(fs::raw::write_text_file(manifestFile.data(), manifest.data() as *u8, manifest.size()))
        }

    }

}

// ── Site-level static-export rewrite aggregate (§13.2.3) ─────────────────────
// A site is many page files, each with its own `<name>.routes.json`. Deep links
// need one URL → page-file map, so the build emits a neutral aggregate (a JSON
// array of the per-page documents) and a deploy tool translates each `{pattern}`
// into its host's rewrite syntax (nginx, Netlify, ...). Host syntax is
// deliberately out of scope.

// Builds the aggregate document from already-serialized per-page documents.
public func site_routes_aggregate(documents : &std::vector<std::string_view>) : std::string {
    var out = std::string()
    out.append_view(std::string_view("["))
    for(var i : size_t = 0; i < documents.size(); i++) {
        if(i > 0) { out.append(',') }
        out.append_view(&documents.at(i))
    }
    out.append_view(std::string_view("]"))
    return out
}

// Reads `<dir>/<name>.routes.json` for each page and writes the aggregate to
// `<dir>/routes.json`. Pages without a router (no manifest file) are skipped.
// Returns the number of page manifests aggregated.
public func write_site_routes(dir : std::string_view, page_names : &std::vector<std::string_view>) : int {
        unsafe {
        var owned = std::vector<std::string>()
        for(var i : size_t = 0; i < page_names.size(); i++) {
            var manifestPath = unsafe(std::string(dir.data(), dir.size()))
            manifestPath.append('/')
            manifestPath.append_view(&page_names.at(i))
            manifestPath.append_view(std::string_view(".routes.json"))
            if(!fs::raw::exists(manifestPath.data())) { continue }
            var read_result = fs::raw::read_entire_file(manifestPath.data())
            if(read_result is Result.Err) { continue }
            var Ok(bytes) = read_result else unsafe { unreachable }
            owned.push(unsafe(std::string(bytes.data() as *char, bytes.size())))
        }
        var docs = std::vector<std::string_view>()
        for(var i : size_t = 0; i < owned.size(); i++) {
            docs.push(owned.get_ptr(i).to_view())
        }
        const agg = site_routes_aggregate(&docs)
        var sitePath = unsafe(std::string(dir.data(), dir.size()))
        sitePath.append_view(std::string_view("/routes.json"))
        var _ = fs::raw::write_text_file(sitePath.data(), agg.data() as *u8, agg.size())
        return owned.size() as int

        }
    }


