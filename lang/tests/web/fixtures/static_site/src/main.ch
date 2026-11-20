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

// How many times `needle` occurs in `hay` (a page must not carry a script tag it
// does not need).
func count_of(hay : std::string_view, needle : std::string_view) : ubigint {
    if(needle.size() == 0 || hay.size() < needle.size()) { return 0 }
    var count : ubigint = 0
    var i : size_t = 0
    var last = hay.size() - needle.size()
    while(i <= last) {
        var k : size_t = 0
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

// A page with no components and no client behaviour: `#html` only.
func render_plain(page : &mut HtmlPage) {
    page.appendTitle("Plain")
    #html {
        <div class="page plain"><h1>Plain</h1><p>No JavaScript at all.</p></div>
    }
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

    // ── gzip precompression (Phase 5 item 1) ─────────────────────────────
    // A third build, differing only by `set_gzip(true)`. It reports the size of
    // each representation and what a request with and without `Accept-Encoding`
    // gets back, so the gate can check the negotiation as well as the bytes.
    var zipped = StaticSite.make("lang/compiled/static_site_gz_out", "app", "assets")
    zipped.set_minify(true)
    zipped.set_gzip(true)
    if(!build_site(&mut zipped)) { return 1 }
    var gz_js = zipped.bundle_js_url()
    var gz_js_view = gz_js
    const gzip_lit = std::string_view("gzip")
    const accept_lit = std::string_view("Accept-Encoding")
    var plain_res = zipped.respond(gz_js_view)
    var gz_res = zipped.respond_negotiated(gz_js_view, std::string_view(""), std::string_view("gzip, deflate, br"))
    var refused_res = zipped.respond_negotiated(gz_js_view, std::string_view(""), std::string_view("identity"))
    var q0_res = zipped.respond_negotiated(gz_js_view, std::string_view(""), std::string_view("gzip;q=0"))
    unsafe {
        printf("gzip js=%llu->%llu enc=%d vary=%d plain_enc=%d refused=%d q0=%d\n",
            plain_res.body.size(), gz_res.body.size(),
            gz_res.content_encoding.equals(&gzip_lit) as int,
            gz_res.vary.equals(&accept_lit) as int,
            plain_res.content_encoding.size(),
            refused_res.body.size() == plain_res.body.size(),
            q0_res.body.size() == plain_res.body.size())
    }
    // The page is compressed too, and the compressed bytes are actually smaller
    // than the HTML (a page this small would otherwise grow).
    var gz_page_plain = zipped.respond("/")
    var gz_page = zipped.respond_negotiated("/", std::string_view(""), std::string_view("gzip"))
    unsafe {
        printf("gzip page=%llu->%llu has_variants=%d\n",
            gz_page_plain.body.size(), gz_page.body.size(),
            zipped.has_gzip_variants() as int)
    }
    // A 304 for one representation must not answer the other: the ETags differ.
    var in_same = std::string_view("")
    var etag_gz = gz_res.etag
    var not_modified = zipped.respond_negotiated(gz_js_view, etag_gz, std::string_view("gzip"))
    var cross_rep = zipped.respond_negotiated(gz_js_view, etag_gz, std::string_view(""))
    unsafe {
        printf("gzip etag_distinct=%d not_modified=%d cross=%llu\n",
            (gz_res.etag.equals(&plain_res.etag) == false) as int,
            not_modified.not_modified() as int,
            cross_rep.body.size())
    }
    var _ = in_same
    if(!zipped.finish()) {
        unsafe { printf("site error: %s\n", zipped.error_message().data()) }
        return 1
    }
    unsafe { printf("gzip written=%s\n", gz_js.data()) }

    // ── profile (Phase 5 item 1) ─────────────────────────────────────────
    // `--profile` is a build definition; without it `set_profile` is simply not
    // called. The fixture always asks, so the gate can check the numbers.
    var profiled = StaticSite.make("lang/compiled/static_site_profile_out", "app", "assets")
    profiled.set_minify(true)
    profiled.set_gzip(true)
    if(!build_site(&mut profiled)) { return 1 }
    if(!profiled.export_profile()) {
        unsafe { printf("profile error: %s\n", profiled.error_message().data()) }
        return 1
    }
    if(!profiled.finish()) {
        unsafe { printf("profile error: %s\n", profiled.error_message().data()) }
        return 1
    }
    var report = profiled.profile_report()
    var report_view = report.to_view()
    unsafe { printf("---profile---\n%s---end---\n", report_view.data()) }

    // ── web::Static: the `--static` multi-page export ────────────────────
    // Two pages, one bundle. The first is `#html` only, so it must ship **no**
    // JavaScript: the "a static page without components has no script" invariant is
    // the reason a marketing page is not forced to download the runtime.
    var mpa = web::Static.make("lang/compiled/static_site_mpa_out", "app", "assets")
    mpa.set_minify(true)
    mpa.set_gzip(true)
    var plain_page = HtmlPage()
    var rich_page = HtmlPage()
    mpa.add(&mut plain_page, "index.html")
    mpa.add(&mut rich_page, "rich/index.html")
    render_plain(&mut plain_page)
    rich_page.defaultUniversalSetup()
    render_index(&mut rich_page)
    if(!mpa.finish()) {
        unsafe { printf("mpa error: %s\n", mpa.error_message().data()) }
        return 1
    }
    var mpa_plain = mpa.respond("/")
    var mpa_rich = mpa.respond("/rich/")
    const script_tag = std::string_view("<script")
    const runtime_marker = std::string_view("$__uni")
    unsafe {
        printf("mpa pages=%llu artifacts=%llu plain_scripts=%llu plain_runtime=%d rich_scripts=%llu bundle=%d\n",
            mpa.page_count(), mpa.artifact_count(),
            count_of(mpa_plain.body, script_tag), count_of(mpa_plain.body, runtime_marker) as int,
            count_of(mpa_rich.body, script_tag),
            (mpa_rich.body.contains(&mpa.bundle_js_url()) as int))
    }
    return 0
}
