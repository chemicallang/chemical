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

#universal Counter(props) {
    state n = 0
    return <button class="counter" onClick={() => { n = n + 1 }}>{n}</button>
}

#universal Badge(props) {
    return <span class="badge">{props.children}</span>
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

public func main() : int {
    var site = StaticSite.make("lang/compiled/static_site_out", "app", "assets")

    var index = HtmlPage()
    site.add(&mut index, "index.html")
    index.defaultUniversalSetup()
    render_index(&mut index)

    var about = HtmlPage()
    site.add(&mut about, "about/index.html")
    about.defaultUniversalSetup()
    render_about(&mut about)

    // In-memory build + serving, before anything touches the filesystem.
    if(!site.build()) {
        unsafe { printf("site error: %s\n", site.error_message().data()) }
        return 1
    }
    unsafe { printf("artifacts=%llu\n", site.artifact_count()) }
    report(&site, "/")
    report(&site, "/index.html")
    report(&site, "/about/")
    report(&site, "/about")
    report(&site, "/missing")
    report(&site, "/../secret")

    var bundle = site.bundle_js_url()
    unsafe { printf("bundle=%s\n", bundle.data()) }
    var bundle_req = std::string()
    bundle_req.append_view("/")
    bundle_req.append_view(&bundle)
    var bundle_req_view = bundle_req.to_view()
    report(&site, bundle_req_view)

    // `finish()` is build + write: the build is already done, so this is the write.
    if(!site.finish()) {
        unsafe { printf("site error: %s\n", site.error_message().data()) }
        return 1
    }
    unsafe { printf("pages=%llu\n", site.page_count()) }
    return 0
}
