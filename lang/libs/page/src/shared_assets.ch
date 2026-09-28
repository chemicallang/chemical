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

    var name : std::string

    var js_data : std::string
    var css_data : std::string

    var done_components : std::unordered_map<ubigint, bool>
    var done_classes : std::unordered_map<ubigint, bool>
    var done_random_classes : std::unordered_map<ubigint, bool>
    // Global JS blocks emitted via `#globaljs`, keyed by source location.
    var done_js : std::unordered_map<ubigint, bool>

    // Set once the runtime has been written into `js_data`, so N attached
    // pages emit the hydration runtime exactly once.
    var runtime_emitted : bool = false

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
        fs::mkdir(dir.data())

        if(!js_data.empty()) {
            var js_path = std::string(dir.data(), dir.size())
            js_path.append('/')
            js_path.append_view(base_name)
            js_path.append_view(".js")
            fs::write_text_file(js_path.data(), js_data.data() as *u8, js_data.size())
        }

        if(!css_data.empty()) {
            var css_path = std::string(dir.data(), dir.size())
            css_path.append('/')
            css_path.append_view(base_name)
            css_path.append_view(".css")
            fs::write_text_file(css_path.data(), css_data.data() as *u8, css_data.size())
        }
    }
}

// Heap-allocated so a set of pages can share one instance by pointer.
public func shared_assets(name : std::string_view = "") : *mut SharedAssets {
    var s = new SharedAssets()
    s.name.append_view(&name)
    return s
}
