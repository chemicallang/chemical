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

    // ── Serving helper (optional) ────────────────────────────────────────
    // Writes "<dir>/<base_name>.js" and "<dir>/<base_name>.css". Callers that
    // want content-hashed names can hash `js()` / `css()` themselves and serve
    // the buffers directly instead of using this.
    public func write_to(&self, dir : &std::string_view, base_name : &std::string_view) {
        fs::raw::mkdir(dir.data())

        if(!js_data.empty()) {
            var js_path = unsafe(std::string(dir.data(), dir.size()))
            js_path.append('/')
            js_path.append_view(base_name)
            js_path.append_view(".js")
            fs::raw::write_text_file(js_path.data(), js_data.data() as *u8, js_data.size())
        }

        if(!css_data.empty()) {
            var css_path = unsafe(std::string(dir.data(), dir.size()))
            css_path.append('/')
            css_path.append_view(base_name)
            css_path.append_view(".css")
            fs::raw::write_text_file(css_path.data(), css_data.data() as *u8, css_data.size())
        }
    }
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

// --- Legacy raw allocation (kept for existing callers) -------------------
//
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
