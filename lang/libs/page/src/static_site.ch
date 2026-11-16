// Multi-page static build: one shared JS/CSS bundle, one HTML file per page.
//
// The hydration runtime, component definitions and global CSS are identical for
// every page. Sent inline, each page repeats them (measured on the components app:
// 78.7% of the second page's bytes were duplicates of the first). `StaticSite`
// routes those bytes into one `SharedAssets` sink, freezes it once, writes it
// under a content-hashed name, and gives every page a `<script src>` / `<link>`
// that points at it — so N pages carry the runtime once and browsers cache it.
//
//   var site = StaticSite.make("out", "app", "assets")
//
//   var index = HtmlPage()
//   site.add(&mut index, "index.html")   // attaches the sink: render AFTER
//   index.defaultUniversalSetup()
//   #html { ... }
//
//   var about = HtmlPage()
//   site.add(&mut about, "about/index.html")
//   #html { ... }
//
//   if(!site.finish()) { printf("%s\n", site.error_message().data()) }
//
// Lifetime contract (a build, so it is simple): pages passed to `add` and paths
// passed to `add` must outlive `finish()`. The site holds a pointer to each page
// and a view of each path, never a copy — so `finish()` writes each page from its
// own buffers, at the moment the shared bundle's hash is known.
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

    // The tags inserted into every page's <head>, built once in `finish()`.
    private var head_tags : std::string

    private var finished : bool
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
            finished : false,
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

    /// The `<head>` tags `finish()` writes into every page (empty before then).
    public func head_assets_html(&self) : std::string_view {
        return head_tags.to_view()
    }

    public func error_message(&self) : std::string_view {
        return failure.to_view()
    }

    /// Freeze the bundle, write it under its content-hashed name, then write every
    /// recorded page with a reference to it. Returns false and sets
    /// `error_message()` on failure.
    public func finish(&mut self) : bool {
        if(finished) { return !failed }
        finished = true

        if(!assets.is_valid()) {
            fail_with("shared asset allocation failed")
            return false
        }

        // Freeze before writing anything: a hashed name may only be published once
        // every page has been rendered, or the name would describe bytes that are
        // still growing (a stale sink refuses to produce a name).
        var shared = assets.get_mut()
        shared.freeze()
        if(shared.is_stale()) {
            fail_with("the shared bundle changed after it was frozen; render every page before finish()")
            return false
        }

        var out_view = out_dir.to_view()
        var prefix_view = asset_prefix.to_view()

        // `mkdir` here is single-level (`_mkdir`/`mkdir(2)`), so the output root has
        // to exist before the bundle's own directory can be created under it.
        ensure_dir_path(out_view)

        var asset_dir = std::string()
        asset_dir.append_view(&out_view)
        asset_dir.append('/')
        asset_dir.append_view(&prefix_view)
        var asset_dir_view = asset_dir.to_view()
        if(!shared.write_frozen(&asset_dir_view)) {
            fail_with("could not write the shared bundle")
            return false
        }
        // `write_frozen` cannot see the filesystem's answer, so verify what it
        // should have produced. A bundle that was not written must not be linked
        // from the pages.
        if(!asset_exists(asset_dir_view, shared.js_name())) {
            fail_with("the shared JS bundle was not written")
            return false
        }
        if(!asset_exists(asset_dir_view, shared.css_name())) {
            fail_with("the shared CSS bundle was not written")
            return false
        }

        // Stylesheet first, then the script: a classic script in <head> has run by
        // the time the page's own script at the end of <body> executes.
        var css_name = shared.css_name()
        var js_name = shared.js_name()

        var tags = head_tags.to_view()
        var i : ubigint = 0
        while(i < pages.size()) {
            var target = paths.at(i)
            ensure_parent_dirs(out_view, target)

            var page_ptr = pages.at(i)
            var page_ref = unsafe(&mut *page_ptr)

            // A page's asset URLs are relative to the *page*, not to the site root:
            // `about/index.html` must reach `../assets/…`, not `assets/…`. Getting
            // this wrong is invisible locally and breaks every nested route.
            var url_prefix_s = page_url_prefix(target, prefix_view)
            var url_prefix = url_prefix_s.to_view()

            var page_tags = std::string()
            if(!css_name.empty()) {
                var css_view = css_name.to_view()
                page_tags.append_view("<link rel=\"stylesheet\" href=\"")
                page_tags.append_view(&url_prefix)
                page_tags.append_view(&css_view)
                page_tags.append_view("\">")
            }
            if(!js_name.empty()) {
                var js_view = js_name.to_view()
                page_tags.append_view("<script src=\"")
                page_tags.append_view(&url_prefix)
                page_tags.append_view(&js_view)
                page_tags.append_view("\"></script>")
            }
            if(i == 0) {
                // The root page's tags, for `head_assets_html()` (a caller that
                // builds its own HTML from the same bundle).
                var root_tags = page_tags.to_view()
                head_tags.append_view(&root_tags)
            }

            // The page's own JS (its dispatch statements) becomes its own hashed
            // chunk: cacheable per route, and the HTML ends up with no inline
            // <script> at all (the CSP rule).
            var body_src = std::string()
            var page_js = page_ref.getFinalizedPageJs()
            if(!page_js.empty()) {
                var page_js_view = page_js.to_view()
                var page_hash = hex12(fnv1_hash_view(&page_js_view))
                var stem = page_chunk_stem(target)
                var stem_view = stem.to_view()
                body_src.append_view(&url_prefix)
                body_src.append_view(&stem_view)
                body_src.append('.')
                var hash_view = page_hash.to_view()
                body_src.append_view(&hash_view)
                body_src.append_view(".js")

                var chunk_path = std::string()
                chunk_path.append_view(&out_view)
                chunk_path.append('/')
                chunk_path.append_view(&prefix_view)
                chunk_path.append('/')
                chunk_path.append_view(&stem_view)
                chunk_path.append('.')
                chunk_path.append_view(&hash_view)
                chunk_path.append_view(".js")
                var chunk_view = chunk_path.to_view()
                var written = unsafe(fs::raw::write_text_file(
                    chunk_view.data(), page_js.data() as *u8, page_js.size()))
                if(written is std::Result.Err) {
                    fail_with("could not write the page script")
                    return false
                }
            }

            var file = std::string()
            file.append_view(&out_view)
            file.append('/')
            file.append_view(&target)

            var page_tags_view = page_tags.to_view()
            var body_src_view = body_src.to_view()
            var html = page_ref.toStringWithAssets(page_tags_view, body_src_view)
            var file_view = file.to_view()
            var write_result = unsafe(fs::raw::write_text_file(file_view.data(), html.data() as *u8, html.size()))
            if(write_result is std::Result.Err) {
                // A page that silently failed to write is a build that "succeeds"
                // with a missing page, so this is fatal.
                fail_with("could not write page")
                fail_with(target)
                return false
            }
            i = i + 1
        }
        return true
    }

    func fail_with(&mut self, message : std::string_view) {
        failed = true
        if(!failure.empty()) {
            failure.append_view("; ")
        }
        failure.append_view(&message)
    }
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

// True when `<dir>/<name>` exists; an empty `name` means "not expected".
func asset_exists(dir : std::string_view, name : std::string) : bool {
    if(name.empty()) { return true }
    var path = std::string()
    path.append_view(&dir)
    path.append('/')
    var name_view = name.to_view()
    path.append_view(&name_view)
    var path_view = path.to_view()
    return unsafe(fs::raw::exists(path_view.data()))
}

// Create `path` and every parent directory of it. Two levels exist because
// `mkdir` is single-level: `<out>` before the bundle directory under it, and the
// parents of a page path like `about/index.html`.
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
    // bundle/page write, which `finish` reports.
    unsafe(fs::raw::mkdir(full_view.data()))
}

// Create every parent directory of `path` (relative to `root`), so a page may be
// written to `blog/post.html` without the caller preparing directories.
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
            // failed page write.
            unsafe(fs::raw::mkdir(dir_view.data()))
        }
        i = i + 1
    }
}
