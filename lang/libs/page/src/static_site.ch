// Build a multi-page site into one artifact set: a shared JS/CSS bundle, one
// HTML page per route, and one script chunk per page.
//
// The hydration runtime, component definitions and global CSS are identical for
// every page. Sent inline, each page repeats them (measured on the components app:
// 78.7% of the second page's bytes were duplicates of the first). `StaticSite`
// routes those bytes into one `SharedAssets` sink, freezes it once, publishes it
// under a content-hashed name, and gives every page a `<script src>` / `<link>`
// that points at it — so N pages carry the runtime once and browsers cache it.
//
// Two ways to deliver the same build:
//
//   *disk* — `finish()` writes every artifact under `out_dir` (static hosting):
//
//     var site = StaticSite.make("out", "app", "assets")
//     var index = HtmlPage()
//     site.add(&mut index, "index.html")   // attaches the sink: render AFTER
//     index.defaultUniversalSetup()
//     #html { ... }
//     if(!site.finish()) { /* site.error_message() */ }
//
//   *memory* — `build()` produces the same artifacts with no filesystem access,
//   and `respond()` answers a request path directly (a server renders once and
//   then serves from RAM):
//
//     if(!site.build()) { ... }
//     var res = site.respond("/about/")     // status, headers, body view
//
// The artifact list is the build's manifest: `artifact_count()` / `artifact_path()`
// / `artifact_bytes()` enumerate exactly what would be written.
//
// Lifetime contract (a build, so it is simple): pages passed to `add` and paths
// passed to `add` must outlive `build()`. The site holds a pointer to each page
// and a view of each path, never a copy — so the pages are read from their own
// buffers, at the moment the shared bundle's hash is known. A `SiteResponse`'s
// body borrows the site's buffers, so it is valid until the site is changed.
@direct_init
public struct StaticSite {

    private var assets : SharedAssetsHandle

    private var pages : std::vector<*mut HtmlPage>
    // Views: a build passes literals (`"index.html"`), which outlive the site.
    private var paths : std::vector<std::string_view>

    private var out_dir : std::string
    // URL prefix the bundle is served under, relative to the page ("assets" ->
    // "assets/app.<hash>.js"). Also the directory it is written into.
    private var asset_prefix : std::string

    // The `<head>` tags for a page at the site root (empty before `build()`).
    private var head_tags : std::string
    // Site-relative URL of the frozen bundle ("" when that half is empty).
    private var bundle_js_path : std::string
    private var bundle_css_path : std::string

    // Everything the build produces, site-relative (`index.html`,
    // `assets/app.<hash>.js`, …). Filled by `build()`, written by `finish()`.
    private var artifacts : std::vector<SiteArtifact>

    private var built : bool
    /// True when `build()` failed. A failed *build* must not be served; a failed
    /// *write* must (the memory build is fine, only the files are missing).
    private var build_failed : bool
    private var written : bool
    private var failed : bool
    private var failure : std::string

    @make
    public func make(out_dir : std::string_view, base_name : std::string_view = "app", asset_prefix : std::string_view = "assets") : StaticSite {
        var dir_s = std::string()
        dir_s.append_view(&out_dir)
        var base_s = std::string()
        base_s.append_view(&base_name)
        var prefix_s = std::string()
        prefix_s.append_view(&asset_prefix)
        return StaticSite {
            assets : SharedAssetsHandle.make(&base_s),
            pages : std::vector<*mut HtmlPage>(),
            paths : std::vector<std::string_view>(),
            out_dir : dir_s,
            asset_prefix : prefix_s,
            head_tags : std::string(),
            bundle_js_path : std::string(),
            bundle_css_path : std::string(),
            artifacts : std::vector<SiteArtifact>(),
            built : false,
            build_failed : false,
            written : false,
            failed : false,
            failure : std::string()
        }
    }

    /// Attach the shared sink to `page` and record it at `path` (relative to the
    /// output directory). Call this before rendering into the page: everything the
    /// page emits while attached that is page-independent goes to the bundle.
    public func add(&mut self, page : &mut HtmlPage, path : std::string_view) {
        if(!assets.is_valid()) {
            fail_with("shared asset allocation failed")
            return
        }
        var shared = assets.get_mut()
        page.attach_shared_ref(shared)
        pages.push_back(page as *mut HtmlPage)
        paths.push_back(path)
    }

    public func page_count(&self) : ubigint {
        return pages.size()
    }

    /// The `<head>` tags a page at the site root needs (empty before `build()`).
    public func head_assets_html(&self) : std::string_view {
        return head_tags.to_view()
    }

    /// Site-relative URLs of the shared bundle, e.g. `assets/app.<hash>.js`.
    /// Empty when that half of the bundle is empty (or before `build()`).
    public func bundle_js_url(&self) : std::string_view {
        return bundle_js_path.to_view()
    }

    public func bundle_css_url(&self) : std::string_view {
        return bundle_css_path.to_view()
    }

    public func error_message(&self) : std::string_view {
        return failure.to_view()
    }

    // ── Manifest ─────────────────────────────────────────────────────────
    public func artifact_count(&self) : ubigint {
        return artifacts.size()
    }

    /// Site-relative path of artifact `i` (`index.html`, `assets/app.<hash>.js`).
    /// Out of range is an empty view, not a panic: this is a read-only query a
    /// caller may loop over.
    public func artifact_path(&self, i : ubigint) : std::string_view {
        if(i >= artifacts.size()) { return std::string_view("") }
        var a = unsafe(&*artifacts.get_ptr(i))
        return a.path.to_view()
    }

    public func artifact_bytes(&self, i : ubigint) : std::string_view {
        if(i >= artifacts.size()) { return std::string_view("") }
        var a = unsafe(&*artifacts.get_ptr(i))
        return a.bytes.to_view()
    }

    public func artifact_content_type(&self, i : ubigint) : std::string_view {
        if(i >= artifacts.size()) { return std::string_view("") }
        var a = unsafe(&*artifacts.get_ptr(i))
        return a.content_type
    }

    /// Build the whole site **in memory**: freeze the shared bundle, then render
    /// every page with a reference to it plus its own script chunk. Touches no
    /// files, so a server can build once and answer every request from RAM with
    /// `respond()`. `finish()` is `build()` plus writing the artifacts to disk.
    ///
    /// Render every page before calling this: a page rendered afterwards would
    /// change the bundle bytes after its name was fixed.
    public func build(&mut self) : bool {
        var ok = build_in_memory()
        if(!ok) { build_failed = true }
        return ok
    }

    func build_in_memory(&mut self) : bool {
        if(built) { return !failed }
        built = true

        if(!assets.is_valid()) {
            fail_with("shared asset allocation failed")
            return false
        }

        // Freeze before naming anything: the bundle is served from a long-lived
        // cache, so its URL may only be published once every page has rendered.
        var shared = assets.get_mut()
        shared.freeze()
        if(shared.is_stale()) {
            fail_with("the shared bundle changed after it was frozen; render every page before build()")
            return false
        }

        var prefix_view = asset_prefix.to_view()
        var js_name = shared.js_name()
        var css_name = shared.css_name()

        // The bundle is an artifact like any other, so `respond()` needs no special
        // case and `finish()` writes everything from one list.
        if(!js_name.empty()) {
            var name_view = js_name.to_view()
            bundle_js_path.append_view(&prefix_view)
            bundle_js_path.append('/')
            bundle_js_path.append_view(&name_view)

            var artifact_path = std::string()
            var bjp_view = bundle_js_path.to_view()
            artifact_path.append_view(&bjp_view)
            var bytes = std::string()
            var js_view = shared.js()
            bytes.append_view(&js_view)
            push_artifact(artifact_path, bytes, std::string_view("text/javascript; charset=utf-8"), true)
        }
        if(!css_name.empty()) {
            var name_view = css_name.to_view()
            bundle_css_path.append_view(&prefix_view)
            bundle_css_path.append('/')
            bundle_css_path.append_view(&name_view)

            var artifact_path = std::string()
            var bcp_view = bundle_css_path.to_view()
            artifact_path.append_view(&bcp_view)
            var bytes = std::string()
            var css_view = shared.css()
            bytes.append_view(&css_view)
            push_artifact(artifact_path, bytes, std::string_view("text/css; charset=utf-8"), true)
        }

        // Stylesheet first, then the script: a classic script in <head> has run by
        // the time the page's own script at the end of <body> executes. The root
        // page's tags do not depend on which page was registered first.
        var root_prefix = std::string()
        root_prefix.append_view(&prefix_view)
        root_prefix.append('/')
        var root_prefix_view = root_prefix.to_view()
        var js_name_view = js_name.to_view()
        var css_name_view = css_name.to_view()
        var root_tags = head_tags_for(&root_prefix_view, &css_name_view, &js_name_view)
        var root_tags_view = root_tags.to_view()
        head_tags.append_view(&root_tags_view)

        var i : ubigint = 0
        while(i < pages.size()) {
            var target = paths.at(i)
            var page_ptr = pages.at(i)
            // The site keeps raw pointers so it never copies a page (a page owns
            // several buffers and maps). Dereferencing one is the single audited
            // step: it came from a `&mut` the caller promised outlives `build()`.
            var page_ref = unsafe(&mut *page_ptr)

            // A page's asset URLs are relative to the *page*, not to the site root:
            // `about/index.html` must reach `../assets/…`, not `assets/…`. Getting
            // this wrong is invisible locally and breaks every nested route.
            var url_prefix_s = page_url_prefix(target, prefix_view)
            var url_prefix = url_prefix_s.to_view()

            // The page's own JS (its dispatch statements) becomes its own hashed
            // chunk: cacheable per route, and the HTML ends up with no inline
            // <script> at all (the CSP rule).
            var body_src = std::string()
            var page_js = page_ref.getFinalizedPageJs()
            if(!page_js.empty()) {
                var stem = page_chunk_stem(target)
                var stem_view = stem.to_view()
                var page_js_view = page_js.to_view()
                var page_hash = hex12(fnv1_hash_view(&page_js_view))
                var hash_view = page_hash.to_view()

                // The URL the page uses…
                body_src.append_view(&url_prefix)
                body_src.append_view(&stem_view)
                body_src.append('.')
                body_src.append_view(&hash_view)
                body_src.append_view(".js")

                // …and the same file, site-relative, for the artifact list.
                var chunk_path = std::string()
                chunk_path.append_view(&prefix_view)
                chunk_path.append('/')
                chunk_path.append_view(&stem_view)
                chunk_path.append('.')
                chunk_path.append_view(&hash_view)
                chunk_path.append_view(".js")
                var chunk_bytes = std::string()
                chunk_bytes.append_view(&page_js_view)
                push_artifact(chunk_path, chunk_bytes, std::string_view("text/javascript; charset=utf-8"), true)
            }

            var page_tags = head_tags_for(&url_prefix, &css_name_view, &js_name_view)
            var page_tags_view = page_tags.to_view()
            var body_src_view = body_src.to_view()
            var html = page_ref.toStringWithAssets(page_tags_view, body_src_view)

            var page_path = std::string()
            page_path.append_view(&target)
            push_artifact(page_path, html, std::string_view("text/html; charset=utf-8"), false)
            i = i + 1
        }
        return !failed
    }

    /// `build()` plus writing every artifact under `out_dir`. Returns false and
    /// sets `error_message()` on failure — including a write that reports success
    /// but leaves no file, which is how a "successful" empty build hides.
    public func finish(&mut self) : bool {
        if(written) { return !failed }
        if(!build()) { return false }
        written = true

        var out_view = out_dir.to_view()
        // `mkdir` here is single-level (`_mkdir`/`mkdir(2)`), so the output root has
        // to exist before anything below it can be created.
        ensure_dir_path(out_view)

        var i : ubigint = 0
        while(i < artifacts.size()) {
            var a = unsafe(&*artifacts.get_ptr(i))
            var rel = a.path.to_view()
            ensure_parent_dirs(out_view, rel)

            var file = std::string()
            file.append_view(&out_view)
            file.append('/')
            file.append_view(&rel)
            var file_view = file.to_view()
            var result = unsafe(fs::raw::write_text_file(file_view.data(), a.bytes.data() as *u8, a.bytes.size()))
            if(result is std::Result.Err) {
                // A page that silently failed to write is a build that "succeeds"
                // with a missing page, so this is fatal and names the file.
                fail_with("could not write")
                fail_with(rel)
                return false
            }
            i = i + 1
        }

        if(artifacts.size() > 0) {
            var first = unsafe(&*artifacts.get_ptr(0))
            if(!artifact_exists(out_view, &first.path.to_view())) {
                fail_with("no artifact reached the output directory")
                return false
            }
        }
        return true
    }

    // ── Serving ──────────────────────────────────────────────────────────
    /// Answer `request_path` from the built artifacts. `/` and `/about/` resolve
    /// to their `index.html`; `/about` does too (a directory without its trailing
    /// slash is the common hand-typed URL). Content-hashed assets are marked
    /// immutable, HTML is not. Unknown paths get a 404 page; a request before
    /// `build()` (or after a failed build) gets a 503 rather than a half-built
    /// site. A failed *write* does not affect serving: the memory build is intact.
    ///
    /// The body borrows the site's buffers: serve it before mutating the site.
    public func respond(&self, request_path : std::string_view) : SiteResponse {
        if(!built || build_failed) {
            return site_response(503, std::string_view("text/plain; charset=utf-8"),
                std::string_view("no-store"), std::string_view("site not built"))
        }
        var target_s = normalize_request_path(request_path)
        if(target_s.empty()) {
            return site_not_found()
        }
        var target = target_s.to_view()
        var index = artifact_index_of(&target)

        if(index < 0) {
            // `/about` -> `about/index.html`: the directory form of a route.
            var as_dir = std::string()
            as_dir.append_view(&target)
            as_dir.append_view("/index.html")
            var as_dir_view = as_dir.to_view()
            index = artifact_index_of(&as_dir_view)
        }
        if(index < 0) {
            return site_not_found()
        }

        var a = unsafe(&*artifacts.get_ptr(index as ubigint))
        if(a.immutable) {
            return site_response(200, a.content_type,
                std::string_view("public, max-age=31536000, immutable"), a.bytes.to_view())
        }
        return site_response(200, a.content_type, std::string_view("no-cache"), a.bytes.to_view())
    }

    // ── Internals ────────────────────────────────────────────────────────
    /// Index of the artifact at `path`, or -1. Content-hashed names make the path
    /// an identity, so this is the whole manifest lookup.
    func artifact_index_of(&self, path : &std::string_view) : int {
        var i : ubigint = 0
        while(i < artifacts.size()) {
            var a = unsafe(&*artifacts.get_ptr(i))
            if(a.path.equals_view(path)) {
                return i as int
            }
            i = i + 1
        }
        return -1
    }

    /// Record one artifact. Two artifacts at the same path with the same bytes are
    /// one file (two routes with identical output share a chunk); different bytes
    /// at the same path is a build bug, not a merge.
    func push_artifact(&mut self, path : std::string, bytes : std::string, content_type : std::string_view, immutable : bool) {
        var path_view = path.to_view()
        var existing = artifact_index_of(&path_view)
        if(existing >= 0) {
            var a = unsafe(&*artifacts.get_ptr(existing as ubigint))
            var bytes_view = bytes.to_view()
            if(!a.bytes.equals_view(&bytes_view)) {
                fail_with("two different artifacts at the same path")
                fail_with(path_view)
            }
            return
        }
        artifacts.push_back(SiteArtifact {
            path : path,
            bytes : bytes,
            content_type : content_type,
            immutable : immutable
        })
    }

    func fail_with(&mut self, message : std::string_view) {
        failed = true
        if(!failure.empty()) {
            failure.append_view("; ")
        }
        failure.append_view(&message)
    }
}

/// One built file, held in memory: what `finish()` writes and `respond()` serves.
@direct_init
public struct SiteArtifact {
    /// Site-relative path (`index.html`, `assets/app.<hash>.js`).
    public var path : std::string
    public var bytes : std::string
    public var content_type : std::string_view
    /// Content-hashed, so the URL changes whenever the bytes do: cache forever.
    public var immutable : bool
}

/// A static response. `body` borrows the site's buffer (no copy per request), so
/// it is valid until the site is rebuilt or dropped.
@direct_init
public struct SiteResponse {
    public var status : int
    public var content_type : std::string_view
    public var cache_control : std::string_view
    public var body : std::string_view

    public func ok(&self) : bool {
        return status == 200
    }

    public func not_found(&self) : bool {
        return status == 404
    }
}

func site_response(status : int, content_type : std::string_view, cache_control : std::string_view, body : std::string_view) : SiteResponse {
    return SiteResponse {
        status : status,
        content_type : content_type,
        cache_control : cache_control,
        body : body
    }
}

func site_not_found() : SiteResponse {
    return site_response(404, std::string_view("text/html; charset=utf-8"), std::string_view("no-store"),
        std::string_view("<!DOCTYPE html><html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1></body></html>"))
}

// `<link>`/`<script src>` tags for a page whose asset URLs start with
// `url_prefix`. Stylesheet first: the bundle must have run before the page's own
// script at the end of <body>.
func head_tags_for(url_prefix : &std::string_view, css_name : &std::string_view, js_name : &std::string_view) : std::string {
    var out = std::string()
    if(!css_name.empty()) {
        out.append_view("<link rel=\"stylesheet\" href=\"")
        out.append_view(url_prefix)
        out.append_view(css_name)
        out.append_view("\">")
    }
    if(!js_name.empty()) {
        out.append_view("<script src=\"")
        out.append_view(url_prefix)
        out.append_view(js_name)
        out.append_view("\"></script>")
    }
    return out
}

// A request path -> the artifact path it names: strip the leading `/`, drop the
// query/fragment, map a directory to its `index.html`, and reject anything that
// could leave the site root. Returns "" for a path that must not be served.
func normalize_request_path(path : std::string_view) : std::string {
    var out = std::string()
    var i : ubigint = 0
    while(i < path.size() && path.at(i) == '/') { i = i + 1 }
    while(i < path.size()) {
        const c = path.at(i)
        if(c == '?' || c == '#') { break }
        // A backslash is a path separator on some hosts and never a legal URL
        // character; refuse rather than guess.
        if(c == '\\') { return std::string() }
        out.append(c)
        i = i + 1
    }

    // Reject `..` as a whole segment. The site is served from memory, so nothing
    // can escape, but a caller wiring this to a disk-backed variant must not have
    // to re-check.
    var j : ubigint = 0
    while(j + 1 < out.size()) {
        if(out.at(j) == '.' && out.at(j + 1) == '.') {
            const at_start = j == 0 || out.at(j - 1) == '/'
            const at_end = j + 2 == out.size() || out.at(j + 2) == '/'
            if(at_start && at_end) { return std::string() }
        }
        j = j + 1
    }

    if(out.empty()) {
        out.append_view("index.html")
        return out
    }
    if(out.at(out.size() - 1) == '/') {
        out.append_view("index.html")
    }
    return out
}

// The URL prefix one page needs to reach the site's asset directory: a page at
// `about/index.html` needs `../assets/`, the root page needs `assets/`.
func page_url_prefix(target : std::string_view, asset_prefix : std::string_view) : std::string {
    var out = std::string()
    var i : ubigint = 0
    while(i < target.size()) {
        if(target.at(i) == '/') {
            out.append_view("../")
        }
        i = i + 1
    }
    out.append_view(&asset_prefix)
    out.append('/')
    return out
}

// `about/index.html` -> `about_index`: the per-page script's file stem.
func page_chunk_stem(path : std::string_view) : std::string {
    var n = path.size()
    const html_ext = std::string_view(".html")
    if(path.ends_with(&html_ext) && n > html_ext.size()) {
        n = n - html_ext.size()
    }
    var out = std::string()
    var i : ubigint = 0
    while(i < n) {
        const c = path.at(i)
        if(c == '/') {
            out.append('_')
        } else {
            out.append(c)
        }
        i = i + 1
    }
    if(out.empty()) {
        out.append_view("page")
    }
    return out
}

// True when `<dir>/<name>` exists.
func artifact_exists(dir : std::string_view, name : &std::string_view) : bool {
    var path = std::string()
    path.append_view(&dir)
    path.append('/')
    path.append_view(name)
    var path_view = path.to_view()
    return unsafe(fs::raw::exists(path_view.data()))
}

// Create `path` and every parent directory of it. The parents exist because
// `mkdir` is single-level: `<out>` before the bundle directory under it.
func ensure_dir_path(path : std::string_view) {
    var i : ubigint = 0
    while(i < path.size()) {
        if(path.at(i) == '/') {
            var dir = std::string()
            if(i > 0) {
                var prefix = unsafe(std::string_view(path.data(), i))
                dir.append_view(&prefix)
            }
            var dir_view = dir.to_view()
            unsafe(fs::raw::mkdir(dir_view.data()))
        }
        i = i + 1
    }
    var full = std::string()
    full.append_view(&path)
    var full_view = full.to_view()
    // An existing directory is not an error; a failed create surfaces as a failed
    // artifact write, which `finish` reports.
    unsafe(fs::raw::mkdir(full_view.data()))
}

// Create every parent directory of `path` (relative to `root`), so an artifact
// may live at `blog/post.html` or `assets/app.<hash>.js` without the caller
// preparing directories.
func ensure_parent_dirs(root : std::string_view, path : std::string_view) {
    var i : ubigint = 0
    while(i < path.size()) {
        if(path.at(i) == '/') {
            var dir = std::string()
            dir.append_view(&root)
            dir.append('/')
            if(i > 0) {
                var segment = unsafe(std::string_view(path.data(), i))
                dir.append_view(&segment)
            }
            var dir_view = dir.to_view()
            // An existing directory is not an error; a failed create shows up as a
            // failed artifact write.
            unsafe(fs::raw::mkdir(dir_view.data()))
        }
        i = i + 1
    }
}
