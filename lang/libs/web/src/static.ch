// Multi-page static export: a set of pages, one shared content-hashed bundle.
//
// This is the `--static` (§3.14) entry point, and the replacement for writing each
// page on its own with `HtmlPage.writeToDirectory`. Per-page output duplicates the
// hydration runtime and every shared component in every file; this writer renders
// all pages first, freezes one bundle, and only then writes HTML that points at it
// — the only order in which the bundle's content hash can be known while the pages
// that reference it are being written.
//
//     var st = web::Static.make("dist")
//     var home = HtmlPage()
//     var about = HtmlPage()
//     st.add(&mut home, "index.html")
//     st.add(&mut about, "about/index.html")
//     // render into the pages here (after add: that is what routes their bytes)
//     if(!st.finish()) { /* st.error_message() */ }
//
// Lifetime: like `StaticSite`, the pages and the path strings passed to `add` must
// outlive `build()`. Paths are views (they are usually literals), pages are
// pointers.
//
// One project can export some routes and serve others: `web::App` is the same
// writer plus a route table, and both freeze the same bundle format, so a browser
// moving between an exported page and a served one stays cache-warm.
public namespace web {

using std::string;
using std::string_view;
using std::vector;

/// How many pages a `Static` can hold. `StaticSite` keeps a pointer to every page
/// and a view of every path, so the backing vectors are reserved up front and may
/// never move; exceeding the capacity is a build error, not a reallocation.
public const PAGE_CAPACITY : ubigint = 512u

@direct_init
public struct Static {

    private var site : StaticSite
    private var built : bool
    private var failed : bool
    private var failure : string
    /// Pre-render metadata the export adds when the build runs (they need the
    /// registered pages, so they are deferred until `build()`).
    private var want_sitemap : bool
    private var want_robots : bool
    private var site_url : string
    private var want_profile : bool
    /// Pages the site owns (see `page`). Reserved up front so `add`'s pointers and
    /// path views stay valid.
    private var pages : vector<HtmlPage>
    private var page_paths : vector<string>

    @make
    public func make(out_dir : string_view, base_name : string_view = "app",
                     asset_prefix : string_view = "assets") : Static {
        var owned_pages = vector<HtmlPage>()
        owned_pages.reserve(PAGE_CAPACITY as size_t)
        var owned_paths = vector<string>()
        owned_paths.reserve(PAGE_CAPACITY as size_t)
        return Static {
            site : StaticSite.make(out_dir, base_name, asset_prefix),
            pages : owned_pages,
            page_paths : owned_paths,
            built : false,
            failed : false,
            failure : string(),
            want_sitemap : false,
            want_robots : false,
            site_url : string(),
            want_profile : false
        }
    }

    /// Export a non-HTML artifact (`search-index.json`, `rss.xml`, a web manifest).
    /// Call before `build()`; `finish()` writes it and `respond` answers it.
    public func add_artifact(&mut self, path : string_view, bytes : string_view,
                             content_type : string_view, immutable : bool = false) : bool {
        return site.add_artifact(path, bytes, content_type, immutable)
    }

    /// Start a page at `path` and return the page **the site owns**; render into it
    /// before the next call.
    ///
    /// This is the safe form of `add`. `add` takes a page the caller owns and keeps
    /// a pointer to it until `build()`, which is a use-after-free waiting for the
    /// natural loop:
    ///
    ///     var page = HtmlPage()          // inside the loop
    ///     site.add(&mut page, path)      // the site keeps the pointer
    ///     // loop ends, `page` is destroyed, build() reads freed memory
    ///
    /// With `page`, the site owns every page and nothing can outlive it:
    ///
    ///     var page = site.page("about/index.html")
    ///     render_about(page)
    public func page(&mut self, path : string_view) : &mut HtmlPage {
        if(pages.size() >= PAGE_CAPACITY) {
            fail_with("Static.page: more pages than PAGE_CAPACITY; raise the capacity")
        }
        var p = string()
        p.append_view(&path)
        page_paths.push_back(p)
        pages.push_back(HtmlPage())
        var slot = pages.size() - 1u
        var stored = unsafe(&*page_paths.get_ptr(slot))
        var path_view = stored.to_view()
        var page_ptr = pages.get_ptr(slot)
        var page_mut = unsafe(&mut *page_ptr)
        site.add(page_mut, path_view)
        return page_mut
    }

    /// Register `page` at `path` (site-relative, e.g. `about/index.html`). Call
    /// this **before** rendering into the page: while attached, everything the page
    /// emits that is page-independent goes into the shared bundle instead.
    public func add(&mut self, page : &mut HtmlPage, path : string_view) {
        // Attach now, not at `build()`: everything the page emits while attached
        // and page-independent goes into the shared bundle, so a page rendered
        // before `add` would carry its own copy of the runtime.
        site.add(page, path)
    }

    public func set_minify(&mut self, on : bool) {
        site.set_minify(on)
    }

    public func set_gzip(&mut self, on : bool) {
        site.set_gzip(on)
    }

    /// Add `profile.json` (per-route bytes, chunk composition, navigation cost) to
    /// the export.
    public func set_profile(&mut self, on : bool) {
        want_profile = on
    }

    /// Build the pages in memory (freeze the bundle, render the HTML) without
    /// touching the filesystem. `respond` answers a path from RAM.
    public func build(&mut self) : bool {
        if(built) { return !failed }
        built = true

        if(want_sitemap || want_robots) {
            if(site_url.empty()) {
                fail_with("export_sitemap() needs a site URL")
                return false
            }
            var url_view = site_url.to_view()
            site.set_site_url(url_view)
            if(want_sitemap && !site.export_sitemap()) {
                fail_with(site.error_message())
                return false
            }
            if(want_robots && !site.export_robots(want_sitemap)) {
                fail_with(site.error_message())
                return false
            }
        }

        if(!site.build()) {
            fail_with(site.error_message())
            return false
        }
        if(want_profile) {
            if(!site.export_profile()) {
                fail_with(site.error_message())
                return false
            }
        }
        return !failed
    }

    /// `build()` plus writing the artifact set (and a `<path>.gz` beside every
    /// artifact that has a compressed variant) under the output directory.
    public func finish(&mut self) : bool {
        if(!build()) { return false }
        if(!site.finish()) {
            fail_with(site.error_message())
            return false
        }
        return true
    }

    /// Publish `sitemap.xml` (and `robots.txt`) for the exported routes. Call
    /// before `build()`; the artifacts are added during the build, so the sitemap
    /// and the pages it lists can never disagree.
    public func export_sitemap(&mut self, url : string_view, robots : bool = true) {
        site_url.clear()
        site_url.append_view(&url)
        want_sitemap = true
        want_robots = robots
    }

    /// Answer a request path from the built artifacts (the same contract as
    /// `StaticSite.respond`: 200/304/404/503, borrowed body, ETag).
    public func respond(&self, request_path : string_view) : SiteResponse {
        return site.respond(request_path)
    }

    public func respond_negotiated(&self, request_path : string_view, if_none_match : string_view,
                                   accept_encoding : string_view) : SiteResponse {
        return site.respond_negotiated(request_path, if_none_match, accept_encoding)
    }

    public func error_message(&self) : string_view {
        return failure.to_view()
    }

    func fail_with(&mut self, message : string_view) {
        failed = true
        if(!failure.empty()) {
            failure.append_view("; ")
        }
        failure.append_view(&message)
    }

    public func bundle_js_url(&self) : string_view {
        return site.bundle_js_url()
    }

    public func bundle_css_url(&self) : string_view {
        return site.bundle_css_url()
    }

    public func page_count(&self) : ubigint {
        return site.page_count()
    }

    public func artifact_count(&self) : ubigint {
        return site.artifact_count()
    }

    public func artifact_path(&self, i : ubigint) : string_view {
        return site.artifact_path(i)
    }

    /// The bytes the build froze for artifact `i` (empty before `build()`, or for
    /// an out-of-range index). Reading them is how a build script reports its own
    /// output size without touching the filesystem.
    public func artifact_bytes(&self, i : ubigint) : string_view {
        return site.artifact_bytes(i)
    }

    public func has_gzip_variants(&self) : bool {
        return site.has_gzip_variants()
    }

    /// The per-route byte report (`StaticSite.profile_report`), empty before
    /// `build()`.
    public func profile_report(&self) : std::string {
        return site.profile_report()
    }

    /// How many component definitions were moved out of the bundle into a route's
    /// own chunk, and how many stayed shared.
    public func chunked_unit_count(&self) : ubigint {
        return site.chunked_unit_count()
    }

    public func retained_unit_count(&self) : ubigint {
        return site.retained_unit_count()
    }
}

} // namespace web
