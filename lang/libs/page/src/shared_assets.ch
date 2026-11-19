// Shared, page-independent JS/CSS extracted from a set of pages.
//
// A page opts in with `page.attach_shared(shared)`. While attached,
// page-independent appends (the hydration runtime, component definitions, the
// router registry, component classes, and `style { }` / `#globalcss` /
// `#globaljs` blocks) are routed into this object and de-duplicated across every
// attached page. Page-specific bytes (SSR HTML, dispatch statements, and the
// page JS/CSS tail) stay on the page.
//
// Intended use: render every page first, then read `js()` / `css()` and serve
// them however the caller likes. `write_to` is an optional convenience.
//
// This object is heap-allocated (see `shared_assets`) so any number of pages
// can share one instance by pointer. Without an attached sink, a page's
// behaviour is unchanged.
public struct SharedAssets {

    internal var name : std::string

    internal var js_data : std::string
    internal var css_data : std::string

    internal var done_components : std::unordered_map<ubigint, bool>
    internal var done_classes : std::unordered_map<ubigint, bool>
    internal var done_random_classes : std::unordered_map<ubigint, bool>
    // Global JS blocks emitted via `#globaljs`, keyed by source location.
    internal var done_js : std::unordered_map<ubigint, bool>

    // Set once the runtime has been written into `js_data`, so N attached
    // pages emit the hydration runtime exactly once.
    internal var runtime_emitted : bool = false

    // ── Unit graph (§3.13 / Phase 4) ─────────────────────────────────────
    // Each component definition is emitted into `js_data` and hoisted to the
    // front of it, so every definition occupies one contiguous byte range. The
    // component emitter records that range here (`record_unit`), and every use
    // site records *which page* used the component (`note_usage`, from
    // `require_component`). A build can then move a definition used by exactly
    // one pre-rendered route into that route's chunk — the route loads only what
    // it runs, while anything shared stays in this bundle.
    //
    // Both tables are build-time only: they are frozen with the bytes, and a
    // mutation attempt after `freeze()` is ignored (a request-time render must
    // not touch them, or two threads serving the same site would race).
    internal var units : std::vector<SharedUnit>
    internal var usage : std::unordered_map<ubigint, std::vector<ubigint>>
    // The keys of `usage`, in the order they were first seen. A `vector` walk is
    // what the build needs ("is every used component defined?"); the map is what
    // makes a per-use lookup cheap.
    internal var usage_keys : std::vector<ubigint>
    // End of the hoisted-definition prefix: where the next attached page's first
    // definition goes. A page's own `js_hoist_pos` is seeded from it (see
    // `HtmlPage.attach_shared_ref`), so a later page appends to the prefix
    // instead of inserting in front of ranges an earlier page already recorded.
    internal var hoist_boundary : ubigint = 0

    // ── Buffers ──────────────────────────────────────────────────────────
    // The page-independent JS (runtime + component definitions + router
    // registry) and CSS (component classes). No dispatch statements.
    public func js(&self) : std::string_view {
        return js_data.to_view()
    }

    public func css(&self) : std::string_view {
        return css_data.to_view()
    }

    public func js_size(&self) : ubigint {
        return js_data.size()
    }

    public func css_size(&self) : ubigint {
        return css_data.size()
    }

    // ── Freeze + content-hashed names ────────────────────────────────────
    // A shared bundle is written once and served from a long-lived cache, so its
    // URL must change whenever its bytes do. `freeze()` records the current
    // content hash; any append after that (a page rendered late) makes the sink
    // *stale*, and a stale sink refuses to hand out a name — so a build can never
    // publish a file whose name disagrees with its content.
    private var frozen : bool = false
    private var frozen_js_hash : size_t = 0
    private var frozen_css_hash : size_t = 0

    // ── Unit graph API ───────────────────────────────────────────────────

    /// Record one component definition's byte range in `js_data`, keyed by the
    /// component's encoded location (the same key `require_component` uses).
    /// Called by the generated component emitter right after the definition has
    /// been hoisted, with the range it now occupies. Re-recording the same
    /// component (a second page needs it, but the sink already has it) is a
    /// no-op: the bytes exist once.
    public func record_unit(&mut self, hash : ubigint, name : std::string_view, start : ubigint, length : ubigint) {
        if(frozen) { return }
        if(length == 0) { return }
        if(unit_index_of(hash) >= 0) { return }
        if(start + length > hoist_boundary) { hoist_boundary = start + length }
        var owned = std::string()
        owned.append_view(&name)
        unsafe {
            units.push_back(SharedUnit {
                hash : hash,
                name : owned,
                start : start,
                length : length
            })
        }
    }

    /// Record that `owner` (a page's route token, or `unit_owner_dynamic()`) uses
    /// the component at `hash`. Every use site calls this — including a use the
    /// sink already holds bytes for — so the owners of a component are its actual
    /// route closure, not just whoever rendered it first.
    public func note_usage(&mut self, hash : ubigint, owner : ubigint) {
        if(frozen) { return }
        if(owner == unit_owner_none()) { return }
        var slot = usage.get_ptr(&hash)
        if(slot == null) {
            unsafe { usage.insert(hash, std::vector<ubigint>()) }
            slot = usage.get_ptr(&hash)
            if(slot == null) { return }
            unsafe { usage_keys.push_back(hash) }
        }
        var owners = unsafe(&*slot)
        var i : ubigint = 0
        while(i < owners.size()) {
            if(owners.at(i) == owner) { return }
            i = i + 1
        }
        unsafe { slot.push_back(owner) }
    }

    /// End of the hoisted-definition prefix (bytes): the offset a newly attached
    /// page begins hoisting at.
    public func hoist_end(&self) : ubigint {
        return hoist_boundary
    }

    public func unit_count(&self) : ubigint {
        return units.size()
    }

    public func unit_name(&self, i : ubigint) : std::string_view {
        if(i >= units.size()) { return std::string_view("") }
        var u = unsafe(&*units.get_ptr(i))
        return u.name.to_view()
    }

    public func unit_hash(&self, i : ubigint) : ubigint {
        if(i >= units.size()) { return 0 }
        var u = unsafe(&*units.get_ptr(i))
        return u.hash
    }

    public func unit_length(&self, i : ubigint) : ubigint {
        if(i >= units.size()) { return 0 }
        var u = unsafe(&*units.get_ptr(i))
        return u.length
    }

    /// How many distinct owners the component at `hash` has (0 when nothing used
    /// it after the sink was populated — e.g. a definition emitted by a page that
    /// has since been discarded).
    public func owner_count(&self, hash : ubigint) : ubigint {
        var slot = usage.get_ptr(&hash)
        if(slot == null) { return 0 }
        var owners = unsafe(&*slot)
        return owners.size()
    }

    public func owner_at(&self, hash : ubigint, i : ubigint) : ubigint {
        var slot = usage.get_ptr(&hash)
        if(slot == null) { return unit_owner_none() }
        var owners = unsafe(&*slot)
        if(i >= owners.size()) { return unit_owner_none() }
        return owners.at(i)
    }

    /// How many components a route used without a recorded byte range.
    ///
    /// Not a failure: an SSR-only component (one that emits no client JS) has
    /// nothing to place, and an inline definition that is never hoisted stays in
    /// the bundle's tail, which the plan never moves. It is a diagnostic — a
    /// component that appears here and *does* have client JS is one the graph
    /// cannot place, so it simply stays shared.
    public func unbacked_usage_count(&self) : ubigint {
        var out : ubigint = 0
        var i : ubigint = 0
        while(i < usage_keys.size()) {
            if(unit_index_of(usage_keys.at(i)) < 0) { out = out + 1 }
            i = i + 1
        }
        return out
    }

    /// True when the recorded ranges are ordered, disjoint and inside `js_data`,
    /// which is the precondition for moving one out without corrupting another.
    /// A definition emitted inside another definition's range (a hoist that
    /// included a nested child) fails this, and the build then keeps everything
    /// in the bundle instead of producing broken JavaScript.
    public func ranges_are_safe(&self) : bool {
        var prev_end : ubigint = 0
        var i : ubigint = 0
        while(i < units.size()) {
            var u = unsafe(&*units.get_ptr(i))
            if(u.length == 0) { i = i + 1; continue }
            if(u.start < prev_end) { return false }
            if(u.start + u.length > js_data.size()) { return false }
            prev_end = u.start + u.length
            i = i + 1
        }
        return true
    }

    /// True when the recorded range holds exactly one definition and nothing else.
    ///
    /// This is the guard that makes moving a unit out safe. A definition is
    /// hoisted as `function name(props) { … }\n`, and every hoisted definition is
    /// recorded as its own unit — so a second line beginning with `function `
    /// inside one range means something else was emitted inside it (a nested
    /// definition). That unit then stays in the shared bundle: bailing out on a
    /// unit is always correct, just larger, while moving it could take bytes a
    /// different route needs.
    public func unit_is_self_contained(&self, i : ubigint) : bool {
        if(i >= units.size()) { return false }
        var u = unsafe(&*units.get_ptr(i))
        if(u.length == 0 || u.start + u.length > js_data.size()) { return false }
        var data = js_data.data()
        if(data == null) { return false }
        var view = unsafe(std::string_view(data + u.start, u.length))
        const marker = std::string_view("function ")
        if(!view.contains(&marker)) { return false }
        const nested = std::string_view("\nfunction ")
        if(view.contains(&nested)) { return false }
        // Every definition this emitter produces is a `function` declaration, so a
        // second `function ` inside the range means another definition was emitted
        // inside it (whatever its line position). Such a range is not one unit's
        // bytes and must not move.
        if(occurrences(&view, &marker) != 1) { return false }
        return true
    }

    /// Remove the (already recorded) unit at `i` from `js_data` and return its
    /// bytes, so the caller can serve them from another file. Ranges of the units
    /// after it shift left by the removed length; the removed unit is marked empty
    /// and stays in the table (so indices handed out earlier stay valid).
    ///
    /// Rebuilds the buffer rather than memmoving in place: `std::string` exposes
    /// no shrink operation to a consumer, and a rebuild is one allocation at build
    /// time.
    public func take_unit(&mut self, i : ubigint) : std::string {
        var out = std::string()
        if(i >= units.size()) { return out }
        var u = unsafe(&*units.get_ptr(i))
        if(u.length == 0) { return out }
        if(u.start + u.length > js_data.size()) { return out }

        const removed = u.length
        const start = u.start
        var bytes = js_data.mutable_data()
        unsafe { out.append_with_len(bytes + start, removed) }

        var rebuilt = std::string()
        if(start > 0) {
            unsafe { rebuilt.append_with_len(bytes, start) }
        }
        var tail = start + removed
        if(tail < js_data.size()) {
            unsafe { rebuilt.append_with_len(bytes + tail, js_data.size() - tail) }
        }
        js_data = rebuilt

        var j : ubigint = 0
        while(j < units.size()) {
            var other = unsafe(&mut *units.get_ptr(j))
            if(j == i) {
                other.start = 0
                other.length = 0
            } else if(other.length > 0 && other.start >= tail) {
                other.start = other.start - removed
            }
            j = j + 1
        }
        return out
    }

    func unit_index_of(&self, hash : ubigint) : int {
        var i : ubigint = 0
        while(i < units.size()) {
            var u = unsafe(&*units.get_ptr(i))
            if(u.hash == hash) { return i as int }
            i = i + 1
        }
        return -1
    }

    public func freeze(&mut self) {
        frozen = true
        var js_view = js_data.to_view()
        var css_view = css_data.to_view()
        frozen_js_hash = fnv1_hash_view(&js_view)
        frozen_css_hash = fnv1_hash_view(&css_view)
    }

    public func is_frozen(&self) : bool {
        return frozen
    }

    /// True when bytes were appended after `freeze()`, so the recorded name no
    /// longer describes the content. A build should treat this as an error.
    public func is_stale(&self) : bool {
        if(!frozen) { return false }
        var js_view = js_data.to_view()
        var css_view = css_data.to_view()
        const js_changed = fnv1_hash_view(&js_view) != frozen_js_hash
        const css_changed = fnv1_hash_view(&css_view) != frozen_css_hash
        return js_changed || css_changed
    }

    /// The bundle's base name (`SharedAssetsHandle.make("app")` -> "app"),
    /// defaulting to "shared".
    public func base_name(&self) : std::string_view {
        if(name.empty()) { return std::string_view("shared") }
        return name.to_view()
    }

    /// `"<base>.<hash>.js"`, or empty when the sink is not frozen or is stale.
    public func js_name(&self) : std::string {
        var out = std::string()
        if(!frozen) { return out }
        if(is_stale()) { return out }
        var base = base_name()
        var hash_s = hex12(frozen_js_hash)
        var hash = hash_s.to_view()
        out.append_view(&base)
        out.append('.')
        out.append_view(&hash)
        out.append_view(".js")
        return out
    }

    /// `"<base>.<hash>.css"`, or empty when the sink is not frozen or is stale.
    public func css_name(&self) : std::string {
        var out = std::string()
        if(!frozen) { return out }
        if(is_stale()) { return out }
        var base = base_name()
        var hash_s = hex12(frozen_css_hash)
        var hash = hash_s.to_view()
        out.append_view(&base)
        out.append('.')
        out.append_view(&hash)
        out.append_view(".css")
        return out
    }

    /// Write both buffers under their content-hashed names. Returns false (and
    /// writes nothing) when the sink is not frozen or went stale — the caller
    /// must render every page before freezing.
    public func write_frozen(&self, dir : &std::string_view) : bool {
        if(!frozen) { return false }
        if(is_stale()) { return false }
        unsafe(fs::raw::mkdir(dir.data()))

        if(!js_data.empty()) {
            var js_name_s = js_name()
            var js_name_v = js_name_s.to_view()
            var p = std::string()
            p.append_view(dir)
            p.append('/')
            p.append_view(&js_name_v)
            unsafe(fs::raw::write_text_file(p.data(), js_data.data() as *u8, js_data.size()))
        }
        if(!css_data.empty()) {
            var css_name_s = css_name()
            var css_name_v = css_name_s.to_view()
            var p = std::string()
            p.append_view(dir)
            p.append('/')
            p.append_view(&css_name_v)
            unsafe(fs::raw::write_text_file(p.data(), css_data.data() as *u8, css_data.size()))
        }
        return true
    }

    // ── Serving helper (optional) ────────────────────────────────────────
    // Writes "<dir>/<base_name>.js" and "<dir>/<base_name>.css". Callers that
    // want content-hashed names can hash `js()` / `css()` themselves and serve
    // the buffers directly instead of using this.
    public func write_to(&self, dir : &std::string_view, base_name : &std::string_view) {
        unsafe(fs::raw::mkdir(dir.data()))

        if(!js_data.empty()) {
            var js_path = unsafe(std::string(dir.data(), dir.size()))
            js_path.append('/')
            js_path.append_view(base_name)
            js_path.append_view(".js")
            unsafe(fs::raw::write_text_file(js_path.data(), js_data.data() as *u8, js_data.size()))
        }

        if(!css_data.empty()) {
            var css_path = unsafe(std::string(dir.data(), dir.size()))
            css_path.append('/')
            css_path.append_view(base_name)
            css_path.append_view(".css")
            unsafe(fs::raw::write_text_file(css_path.data(), css_data.data() as *u8, css_data.size()))
        }
    }
}

// How many times `needle` occurs in `hay`. Used by the unit graph's containment
// guard (`unit_is_self_contained`), where "more than once" is the signal that
// something other than one definition is inside a recorded range.
func occurrences(hay : &std::string_view, needle : &std::string_view) : ubigint {
    if(needle.size() == 0 || hay.size() < needle.size()) { return 0 }
    var count : ubigint = 0
    var i : ubigint = 0
    var last = hay.size() - needle.size()
    while(i <= last) {
        var k : ubigint = 0
        var matched = true
        while(k < needle.size()) {
            if(hay.at(i + k) != needle.at(k)) { matched = false; break }
            k = k + 1
        }
        if(matched) {
            count = count + 1
            i = i + needle.size()
        } else {
            i = i + 1
        }
    }
    return count
}

// One component definition inside a shared JS bundle: the byte range it occupies
// after hoisting, and the identity (`hash` = the component's encoded source
// location, the same key `require_component` uses) that ties it to its uses.
@direct_init
public struct SharedUnit {
    public var hash : ubigint
    public var name : std::string
    public var start : ubigint
    public var length : ubigint
}

/// Owner token for a route rendered per request (a live or shell route): its
/// component units must stay in the shared bundle, because the per-request page
/// dispatches them from the bundle rather than from its own chunk.
public func unit_owner_dynamic() : ubigint {
    return 0xFFFFFFFFFFFFFFFF
}

/// Owner token for "no route" (a page that is not part of a `StaticSite`):
/// usage recorded under it is ignored, so rendering a page on its own never
/// affects a site's chunk plan.
public func unit_owner_none() : ubigint {
    return 0
}

// Owning handle for a shared sink. This is the canonical way to create and
// dispose a `SharedAssets`: the handle owns the heap object, is move-only, and
// frees it in its `@delete` destructor. Pages borrow the sink for the render
// (they never take ownership), so detach pages before dropping the handle.
//
// `SharedAssets` itself owns all of its state (`std::string` buffers and the
// visited hash maps); nothing in the public surface hands out an internal raw
// pointer.
@direct_init
public struct SharedAssetsHandle {
    private var ptr : *mut SharedAssets

    @make
    public func make(name : &std::str = "") : SharedAssetsHandle {
        unsafe {
            var s = new SharedAssets()
            if(s == null) { return SharedAssetsHandle { ptr : null } }
            s.name.append_view(name.to_view())
            return SharedAssetsHandle { ptr : s }
        }
    }

    public func is_valid(&self) : bool {
        return self.ptr != null
    }

    // Borrow the sink for reading (js/css/size). Requires `is_valid()`; an
    // allocation-failed handle has no sink to borrow.
    public func get(&self) : &SharedAssets {
        unsafe { return &*self.ptr }
    }

    // Borrow the sink for mutation (e.g. attaching pages or clearing).
    // Requires `is_valid()`.
    public func get_mut(&mut self) : &mut SharedAssets {
        unsafe { return &mut *self.ptr }
    }

    @delete
    func destruct(&self) {
        unsafe {
            if(self.ptr != null) { delete self.ptr }
        }
    }
}

// Content hashes are shortened to 12 hex characters: enough for build-asset
// identity (48 bits) and short enough to keep URLs readable.
func hex12(value : size_t) : std::string {
    const HEX = std::string_view("0123456789abcdef")
    var out = std::string()
    var shift : size_t = 44
    while(true) {
        const nib = (value >> shift) & 15
        out.append(HEX.at(nib as uint))
        if(shift == 0) { break }
        shift = shift - 4
    }
    return out
}

// --- Legacy raw allocation (kept for existing callers) -------------------//
// Prefer `SharedAssetsHandle`. These two functions are the raw pointer form:
// the caller owns the returned pointer and must release it with
// `shared_assets_free` exactly once.
public func shared_assets(name : std::string_view = "") : *mut SharedAssets {
        unsafe {
        var s = new SharedAssets()
        s.name.append_view(&name)
        return s

        }
    }

/// Release a `SharedAssets` heap handle created by `shared_assets`.
/// Explicit because the handle is shared by pointer between pages.
public unsafe func shared_assets_free(s : *mut SharedAssets) {
    if(s == null) { return }
    unsafe { delete s }
}
