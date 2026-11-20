// Static site fixture: two pages sharing one JS/CSS bundle.
//
// Both pages instantiate the same components, so without a shared bundle the
// second page repeats the whole hydration runtime and every component
// definition. `StaticSite` routes those into one content-hashed asset and gives
// each page a `<script src>` / `<link>` that points at it.
//
// The output directory is what `scripts/web_probe.py analyze <dir>` measures.
// The same build is also served from memory through `respond()`, which is what a
// server does — printing the answers here lets `scripts/static-site-test.sh`
// check them without a socket.

// Nanoseconds to answer `iterations` requests through the `web.Router` bridge —
// the path a real server takes, including the `http::Response` construction.
func time_serve(site : *mut StaticSite, req : &mut http::Request, iterations : int) : i64 {
    var start = std::chrono::Instant.now()
    var i : int = 0
    while(i < iterations) {
        var res = webserve.serve(site, req)
        if(res.body.is_empty()) { return -1 }
        i = i + 1
    }
    return start.elapsed().as_nanos()
}

#universal Counter(props) {
    state n = 0
    return <button class="counter" onClick={() => { n = n + 1 }}>{n}</button>
}

#universal Badge(props) {
    return <span class="badge">{props.children}</span>
}

// Used *only* by the per-request route: it reaches the shared bundle through the
// warm pass, not through a pre-rendered page.
#universal Ticker(props) {
    state t = 0
    return <span class="ticker">{props.label}:{t}</span>
}

// Never rendered before build(): the per-request render must refuse it rather than
// append to the frozen bundle.
#universal Late(props) {
    return <span class="late">{props.children}</span>
}

func render_index(page : &mut HtmlPage) {
    page.appendTitle("Index")
    #html {
        <div class="page index">
            <h1>Index</h1>
            <Counter />
            <Badge>shared</Badge>
        </div>
    }
}

func render_about(page : &mut HtmlPage) {
    page.appendTitle("About")
    #html {
        <div class="page about">
            <h1>About</h1>
            <Counter />
            <Badge>shared</Badge>
        </div>
    }
}

// A route rendered per request. `who` makes two requests differ, which is what
// makes it dynamic (a pre-rendered page would be byte-identical).
func render_account(page : &mut HtmlPage, who : std::string_view) {
    page.appendTitle("Account")
    #html {
        <div class="page account">
            <h1>Account</h1>
            <Badge>shared</Badge>
            <Ticker label={who} />
        </div>
    }
}

func render_late(page : &mut HtmlPage) {
    #html {
        <div class="page late"><Late>too late</Late></div>
    }
}

// Print one response as `<status> <content-type> <cache-control> <bytes>` so the
// shell gate can check the serving contract.
func report(site : &StaticSite, path : std::string_view) {
    var res = site.respond(path)
    unsafe {
        printf("respond %s -> %d | %s | %s | %llu B\n",
            path.data(), res.status,
            res.content_type.data(), res.cache_control.data(), res.body.size())
    }
}

// Two pages, one per-request route and one warm pass: everything `build()` needs to
// freeze a bundle. Shared by the raw build and its minified twin so the two outputs
// differ in exactly one thing (the site's `set_minify` flag).
func build_site(site : &mut StaticSite) : bool {
    var index = HtmlPage()
    site.add(&mut index, "index.html")
    index.defaultUniversalSetup()
    render_index(&mut index)

    var about = HtmlPage()
    site.add(&mut about, "about/index.html")
    about.defaultUniversalSetup()
    render_about(&mut about)

    // Warm pass: render every per-request route once before `build()`, so the
    // components only it uses (`Ticker`) are part of the frozen bundle. The warmed
    // page is discarded — nothing here becomes an artifact.
    var warm = HtmlPage()
    site.attach_dynamic(&mut warm)
    warm.defaultUniversalSetup()
    render_account(&mut warm, std::string_view("warm"))

    // In-memory build: freeze the bundle and plan the per-route chunks.
    if(!site.build()) {
        unsafe { printf("site error: %s\n", site.error_message().data()) }
        return false
    }
    unsafe { printf("artifacts=%llu\n", site.artifact_count()) }
    return true
}

public func main() : int {
    var site = StaticSite.make("lang/compiled/static_site_out", "app", "assets")
    if(!build_site(&mut site)) { return 1 }
    report(&site, "/")
    report(&site, "/index.html")
    report(&site, "/about/")
    report(&site, "/about")
    report(&site, "/missing")
    report(&site, "/../secret")

    // ── serving cost: bytes are written, not copied ──────────────────────
    // A bundle asset is the dominant response size (100+ KB) and is immutable, so
    // per-request work should be the head plus a write of the frozen bytes. Timing
    // it next to a small page makes a body copy show up as a ratio, not a guess.
    var page_req = http::Request()
    var root_path = std::string_view("/")
    page_req.path.append_view(&root_path)

    var bundle = site.bundle_js_url()
    unsafe { printf("bundle=%s\n", bundle.data()) }
    var bundle_req = std::string()
    bundle_req.append_view("/")
    bundle_req.append_view(&bundle)
    var bundle_req_view = bundle_req.to_view()
    report(&site, bundle_req_view)

    // Conditional request: the same path with the ETag the first response carried
    // is a 304 with no body, so a browser revalidates without re-downloading.
    var asset_req = http::Request()
    asset_req.path.append_view(&bundle_req_view)
    var site_ptr = &raw mut site
    var asset_ns = time_serve(site_ptr, &mut asset_req, 300)
    var small_ns = time_serve(site_ptr, &mut page_req, 300)
    var asset_res = site.respond(bundle_req_view)
    var page_res = site.respond("/")
    var served_asset = webserve.serve(site_ptr, &mut asset_req)
    var served_page = webserve.serve(site_ptr, &mut page_req)
    unsafe {
        printf("serve asset=%llu B %lld ns/req view=%d | page=%llu B %lld ns/req view=%d\n",
            asset_res.body.size(), asset_ns / 300, served_asset.body.is_view() as int,
            page_res.body.size(), small_ns / 300, served_page.body.is_view() as int)
    }

    var full = site.respond(bundle_req_view)
    unsafe { printf("etag=%s bytes=%llu\n", full.etag.data(), full.body.size()) }
    var cached = site.respond_if_none_match(bundle_req_view, full.etag)
    unsafe {
        printf("conditional bundle -> %d | %llu B | %s\n",
            cached.status, cached.body.size(), cached.cache_control.data())
    }
    var root = site.respond("/")
    var cached_root = site.respond_if_none_match("/", root.etag)
    unsafe {
        printf("conditional page -> %d | %llu B | %s\n",
            cached_root.status, cached_root.body.size(), cached_root.cache_control.data())
    }
    var stale = site.respond_if_none_match("/", std::string_view("\"stale\""))
    unsafe { printf("stale etag -> %d | %llu B\n", stale.status, stale.body.size()) }

    // ── per-request routes ───────────────────────────────────────────────
    // Same URL, different bytes: this is what makes the route dynamic, and why it
    // is answered `no-store` with no validator. It still loads the site's frozen
    // bundle (there is no second copy of the runtime).
    var dyn_a = HtmlPage()
    site.attach_dynamic(&mut dyn_a)
    dyn_a.defaultUniversalSetup()
    render_account(&mut dyn_a, std::string_view("ada"))
    var html_a = site.render_dynamic(&mut dyn_a)
    var res_a = site.dynamic_response(&html_a)
    unsafe {
        printf("dynamic ada -> %d | %s | %llu B | etag=[%s]\n",
            res_a.status, res_a.cache_control.data(), res_a.body.size(), res_a.etag.data())
    }
    unsafe { printf("---dynamic ada---\n%s\n---end---\n", html_a.data()) }

    var dyn_b = HtmlPage()
    site.attach_dynamic(&mut dyn_b)
    dyn_b.defaultUniversalSetup()
    render_account(&mut dyn_b, std::string_view("grace"))
    var html_b = site.render_dynamic(&mut dyn_b)
    var res_b = site.dynamic_response(&html_b)
    unsafe {
        printf("dynamic grace -> %d | %s | %llu B | same_bytes=%d\n",
            res_b.status, res_b.cache_control.data(), res_b.body.size(),
            (html_a.size() == html_b.size()) as int)
    }
    unsafe { printf("---dynamic grace---\n%s\n---end---\n", html_b.data()) }

    // A component the frozen bundle does not contain cannot be served: the render
    // is refused (empty document -> 503) instead of publishing HTML whose hydration
    // would miss a definition. Do this last: appending to the sink marks it stale.
    var dyn_late = HtmlPage()
    site.attach_dynamic(&mut dyn_late)
    dyn_late.defaultUniversalSetup()
    render_late(&mut dyn_late)
    var html_late = site.render_dynamic(&mut dyn_late)
    var res_late = site.dynamic_response(&html_late)
    unsafe {
        printf("dynamic late -> %d | %s | %llu B | error=%s\n",
            res_late.status, res_late.cache_control.data(), res_late.body.size(),
            site.dynamic_error_message().data())
    }

    // `finish()` is build + write: the build is already done, so this is the write.
    if(!site.finish()) {
        unsafe { printf("site error: %s\n", site.error_message().data()) }
        return 1
    }
    unsafe { printf("pages=%llu\n", site.page_count()) }

    // ── minification (Phase 5 item 1) ────────────────────────────────────
    // The same site, frozen with `set_minify(true)`. Nothing else differs, so the
    // sizes below are the minifier's whole effect, and the graph checks that run on
    // this build prove the unit ranges survived it (a chunk is cut *after*
    // minification, from a range that minification recomputed).
    var mini = StaticSite.make("lang/compiled/static_site_min_out", "app", "assets")
    mini.set_minify(true)
    if(!build_site(&mut mini)) { return 1 }
    var mini_js = mini.bundle_js_url()
    var mini_css = mini.bundle_css_url()
    var raw_js = site.bundle_js_url()
    var raw_css = site.bundle_css_url()
    // Both builds must chunk the same way: minification rewrites the bytes a unit
    // range points at, so a recomputed range that is off by one would move the
    // wrong code into a route's chunk (or none at all).
    unsafe {
        printf("minify js=%llu->%llu css=%llu->%llu chunked=%llu/%llu min_js_path=%s\n",
            site.respond(raw_js).body.size(), mini.respond(mini_js).body.size(),
            site.respond(raw_css).body.size(), mini.respond(mini_css).body.size(),
            site.chunked_unit_count(), mini.chunked_unit_count(), mini_js.data())
    }
    // The page of the minified build references that build's hashed bundle (the
    // content hash changed with the bytes) and the bundle is served from it.
    var mini_page = mini.respond("/")
    unsafe {
        printf("minify page=%llu B refs_own=%d\n", mini_page.body.size(),
            mini_page.body.contains(&mini_js) as int)
    }
    if(!mini.finish()) {
        unsafe { printf("site error: %s\n", mini.error_message().data()) }
        return 1
    }
    return 0
}
