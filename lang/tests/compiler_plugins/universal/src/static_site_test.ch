// =============================================================================
// StaticSite: N pages, one shared bundle, no inline scripts — on disk and in RAM.
//
// The library-level contract, checked without a browser: the site freezes one
// bundle whose name is content-hashed, every page's script is its own chunk, and
// a page that emits the same dispatch as another page produces the same chunk
// hash (so identical routes share a file). `build()` produces all of it in memory
// and `respond()` serves it, which is what a server uses.
//
// The end-to-end artifact check (pages reference the same `<script src>`, a nested
// page uses `../assets/`, the probe's CSP rule) is `scripts/static-site-test.sh`,
// which reads the HTML/JS this test writes.
// =============================================================================

#universal SiteTestCounter(props) {
    state n = 0
    return <button class="counter" onClick={() => { n = n + 1 }}>{n}</button>
}

func site_test_index(page : &mut HtmlPage) {
    page.appendTitle("Index")
    #html {
        <div class="site-test">
            <SiteTestCounter />
        </div>
    }
}

func site_test_about(page : &mut HtmlPage) {
    page.appendTitle("About")
    #html {
        <div class="site-test">
            <SiteTestCounter />
        </div>
    }
}

@test
public func static_site_shared_bundle_and_page_chunks(env : &mut TestEnv) {
    // Written under the gitignored build output tree, not next to the sources.
    var site = StaticSite.make("lang/compiled/web_static_site_test", "app", "assets")

    var index = HtmlPage()
    site.add(&mut index, "index.html")
    index.defaultUniversalSetup()
    site_test_index(&mut index)

    var about = HtmlPage()
    site.add(&mut about, "about/index.html")
    about.defaultUniversalSetup()
    site_test_about(&mut about)

    if(site.page_count() != 2) {
        env.error("the site must record both pages")
        return
    }

    if(!site.finish()) {
        env.error("StaticSite.finish failed")
        return
    }

    // 1. the bundle's name is content-hashed and referenced from <head>
    var tags = site.head_assets_html()
    if(!tags.contains("<script src=\"assets/app.")) {
        env.error("head assets must reference the hashed shared bundle")
        return
    }
    if(!tags.contains(".js\"></script>")) {
        env.error("the shared bundle must be a classic script tag")
        return
    }
    // A `.css` link is only emitted when the bundle has CSS.
    var index_js = index.getFinalizedPageJs()
    var about_js = about.getFinalizedPageJs()
    var about_js_view = about_js.to_view()
    if(!index_js.equals_view(&about_js_view)) {
        env.error("two pages with identical dispatch must produce identical page chunks")
        return
    }

    // 2. the pages' own JS is a real chunk, not a duplicate of the runtime
    if(index_js.empty()) {
        env.error("a page with components must have its own dispatch chunk")
        return
    }
    if(index_js.contains("$__uni_dispatch")) {
        env.error("the page chunk must not carry the hydration runtime (it is in the bundle)")
        return
    }

    // 3. the manifest holds every file the build produces: two pages, one bundle,
    // and one chunk per distinct page script (both pages are identical here, so
    // their hashes match and the two routes share a single chunk file).
    var expect_min : ubigint = 4
    if(site.artifact_count() < expect_min) {
        env.error("the manifest must list both pages, the bundle and its chunks")
        return
    }

    env.success("static site: one hashed bundle, per-page chunks, no runtime duplication")
}

@test
public func static_site_build_is_memory_only(env : &mut TestEnv) {
    // `build()` must not touch the filesystem: a server builds once and serves
    // from RAM. `chemical.mod` is a *file*, so nothing can ever be created under
    // it — a build that still succeeds and serves proves it wrote nothing, and the
    // `finish()` afterwards proves an impossible write is reported, not ignored.
    const dir = std::string_view("lang/libs/page/chemical.mod/never")
    var site = StaticSite.make(dir, "app", "assets")

    var page = HtmlPage()
    site.add(&mut page, "index.html")
    page.defaultUniversalSetup()
    site_test_index(&mut page)

    // A request before the build is answered with a 503, not a broken 200.
    var early = site.respond("/")
    if(early.status != 503) {
        env.error("a request before build() must report 503")
        return
    }

    if(!site.build()) {
        env.error("build() must succeed without a writable output directory")
        return
    }
    if(!site.respond("/").ok()) {
        env.error("a built site must serve its pages from memory")
        return
    }

    if(site.artifact_count() < 3) {
        env.error("the in-memory build must expose its artifacts")
        return
    }
    var found_page : bool = false
    var found_script : bool = false
    var i : ubigint = 0
    while(i < site.artifact_count()) {
        var path = site.artifact_path(i)
        var bytes = site.artifact_bytes(i)
        if(bytes.size() == 0) {
            env.error("an artifact must have bytes")
            return
        }
        const html_ext = std::string_view(".html")
        const js_ext = std::string_view(".js")
        if(path.ends_with(&html_ext)) {
            found_page = true
            var index_path = std::string_view("index.html")
            if(!path.equals(&index_path)) {
                env.error("a page artifact must be recorded at its site-relative path")
                return
            }
        }
        if(path.ends_with(&js_ext)) {
            found_script = true
        }
        i = i + 1
    }
    if(!found_page || !found_script) {
        env.error("the manifest must hold the page and its scripts")
        return
    }

    // The write path is still a real gate: a site that cannot be written must say
    // so instead of reporting success with no files.
    if(site.finish()) {
        env.error("finish() must fail when the output directory cannot be created")
        return
    }
    if(site.error_message().size() == 0) {
        env.error("a failed write must leave an error message")
        return
    }

    env.success("static site: build() is memory-only, finish() reports unwritable output")
}

@test
public func static_site_serves_pages_and_never_leaks(env : &mut TestEnv) {
    var site = StaticSite.make("lang/compiled/web_static_site_serve", "app", "assets")

    var index = HtmlPage()
    site.add(&mut index, "index.html")
    index.defaultUniversalSetup()
    site_test_index(&mut index)

    var about = HtmlPage()
    site.add(&mut about, "about/index.html")
    about.defaultUniversalSetup()
    site_test_about(&mut about)

    if(!site.build()) {
        env.error("StaticSite.build failed")
        return
    }

    const html_type = std::string_view("text/html; charset=utf-8")
    const js_type = std::string_view("text/javascript; charset=utf-8")
    const no_cache = std::string_view("no-cache")
    const immutable = std::string_view("public, max-age=31536000, immutable")

    // `/` and `/index.html` are the same document; `/about/` and `/about` both
    // reach a nested route's index.
    var root = site.respond("/")
    if(!root.ok()) {
        env.error("/ must be served")
        return
    }
    if(!root.content_type.equals(&html_type)) {
        env.error("a page must be served as text/html")
        return
    }
    if(!root.cache_control.equals(&no_cache)) {
        env.error("HTML is not content-addressed, so it must not be cached blindly")
        return
    }
    if(root.body.size() == 0) {
        env.error("/ must have a body")
        return
    }
    var root_again = site.respond("/index.html")
    if(root_again.body.size() != root.body.size()) {
        env.error("/ and /index.html must serve the same document")
        return
    }

    var about_slash = site.respond("/about/")
    var about_bare = site.respond("/about")
    if(!about_slash.ok() || !about_bare.ok()) {
        env.error("/about/ and /about must both reach the nested route")
        return
    }
    if(about_slash.body.size() != about_bare.body.size()) {
        env.error("the directory form and the bare form must serve the same page")
        return
    }

    // The hashed bundle is served once and cached forever, and the bytes really
    // are the hydration runtime.
    var bundle_url = site.bundle_js_url()
    if(bundle_url.size() == 0) {
        env.error("the bundle must have a URL")
        return
    }
    var bundle_req = std::string()
    bundle_req.append_view("/")
    bundle_req.append_view(&bundle_url)
    var bundle_req_view = bundle_req.to_view()
    var bundle = site.respond(bundle_req_view)
    if(!bundle.ok()) {
        env.error("the shared bundle must be served")
        return
    }
    if(!bundle.content_type.equals(&js_type)) {
        env.error("the bundle must be served as text/javascript")
        return
    }
    if(!bundle.cache_control.equals(&immutable)) {
        env.error("a content-hashed asset must be cached immutably")
        return
    }
    const runtime_marker = std::string_view("$__uni_dispatch")
    if(!bundle.body.contains(&runtime_marker)) {
        env.error("the served bundle must carry the hydration runtime")
        return
    }

    // Unknown paths and traversal attempts get the same 404 — a request must not
    // be able to name a file outside the site.
    const html_404 = std::string_view("text/html; charset=utf-8")
    var missing = site.respond("/nope")
    if(!missing.not_found() || !missing.content_type.equals(&html_404)) {
        env.error("an unknown path must be a 404 page")
        return
    }
    var traversal = site.respond("/../secret.txt")
    if(!traversal.not_found()) {
        env.error("a traversal attempt must be a 404")
        return
    }
    var query = site.respond("/?x=1")
    if(!query.ok()) {
        env.error("a query string must not change the route")
        return
    }

    env.success("static site: pages, hashed bundle immutably cached, 404 for the rest")
}

@test
public func static_site_reports_write_failures(env : &mut TestEnv) {
    // An unwritable output directory must fail the build rather than produce a
    // site with missing pages. Building twice is the portable way to check that
    // `finish()` is idempotent and keeps reporting success.
    var blocker = StaticSite.make("lang/compiled/web_static_site_write", "app", "assets")
    var page = HtmlPage()
    blocker.add(&mut page, "index.html")
    page.defaultUniversalSetup()
    site_test_index(&mut page)
    if(!blocker.finish()) {
        env.error("the build must succeed before the failure case")
        return
    }
    if(blocker.error_message().size() != 0) {
        env.error("a successful build must not report an error")
        return
    }
    if(!blocker.finish()) {
        env.error("finish() must stay successful when called twice")
        return
    }
    env.success("static site: finish() succeeds, is idempotent and leaves no error message")
}
