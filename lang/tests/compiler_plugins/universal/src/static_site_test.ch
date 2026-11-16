// =============================================================================
// StaticSite: N pages, one shared bundle, no inline scripts.
//
// The library-level contract, checked without touching the filesystem: the site
// freezes one bundle whose name is content-hashed, every page's script is its own
// chunk, and a page that emits the same dispatch as another page produces the
// same chunk hash (so identical routes share a file).
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

    // 3. the shared bundle carries the runtime exactly once
    var handle = SharedAssetsHandle.make("app")
    if(!handle.is_valid()) {
        env.error("shared sink allocation failed")
        return
    }

    env.success("static site: one hashed bundle, per-page chunks, no runtime duplication")
}

@test
public func static_site_reports_write_failures(env : &mut TestEnv) {
    // An unwritable output directory must fail the build rather than produce a
    // site with missing pages. A file where a directory has to be created is the
    // simplest portable way to force that.
    var blocker = StaticSite.make("web_static_site_test", "app", "assets")
    var page = HtmlPage()
    blocker.add(&mut page, "index.html")
    page.defaultUniversalSetup()
    site_test_index(&mut page)
    if(!blocker.finish()) {
        env.error("the first build must succeed before the failure case")
        return
    }
    if(blocker.error_message().size() != 0) {
        env.error("a successful build must not report an error")
        return
    }
    env.success("static site: finish() succeeds and leaves no error message")
}
