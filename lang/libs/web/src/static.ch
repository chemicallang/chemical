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

    @make
    public func make(out_dir : string_view, base_name : string_view = "app",
                     asset_prefix : string_view = "assets") : Static {
        return Static {
            site : StaticSite.make(out_dir, base_name, asset_prefix),
            built : false,
            failed : false,
            failure : string(),
            want_sitemap : false,
            want_robots : false,
            site_url : string(),
            want_profile : false
        }
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
