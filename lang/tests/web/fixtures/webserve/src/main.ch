// webserve fixture: the built site answered through the HTTP bridge.
//
// No socket: the bridge is a pure `Request -> Response` mapping, so the fixture
// constructs requests and checks what a server would send. The socket half is
// `http::Server`'s own territory; what is new here is the mapping.
//
// `scripts/static-site-test.sh` runs this and greps the printed lines.

using std::Option;

#universal ServeCounter(props) {
    state n = 0
    return <button class="counter" onClick={() => { n = n + 1 }}>{n}</button>
}

func render_index(page : &mut HtmlPage) {
    page.appendTitle("Index")
    #html {
        <div class="page">
            <h1>Index</h1>
            <ServeCounter />
        </div>
    }
}

func header_or_empty(res : &http.Response, name : std::string_view) : std::string_view {
    var found = res.headers.get(name)
    if(found is Option.Some) {
        var Some(value) = found else unsafe { unreachable }
        return value
    }
    return std::string_view("")
}

func report(label : std::string_view, res : &http.Response) {
    var content_type = header_or_empty(res, std::string_view("Content-Type"))
    var cache = header_or_empty(res, std::string_view("Cache-Control"))
    var etag = header_or_empty(res, std::string_view("ETag"))
    var etag_text = etag
    if(etag.empty()) {
        etag_text = std::string_view("no-etag")
    }
    unsafe {
        printf("%s -> %u | %s | %s | %s | %llu B\n", label.data(), res.status,
            content_type.data(), cache.data(), etag_text.data(), res.body.size())
    }
}

func request_for(path : std::string_view) : http.Request {
    var req = http.Request()
    var p = std::string()
    p.append_view(&path)
    req.path = p
    return req
}

public func main() : int {
    var site = StaticSite.make("lang/compiled/webserve_out", "app", "assets")

    var index = HtmlPage()
    site.add(&mut index, "index.html")
    index.defaultUniversalSetup()
    render_index(&mut index)

    if(!site.build()) {
        unsafe { printf("build error: %s\n", site.error_message().data()) }
        return 1
    }

    var site_ptr = &raw mut site
    unsafe { webserve.install(site_ptr) }

    // A page at the root.
    var root_req = request_for("/")
    var root = webserve.serve(site_ptr, &root_req)
    report(std::string_view("serve /"), &root)

    // The route handler form reads the installed site.
    var via_handler = webserve.respond_to_request(request_for("/"))
    report(std::string_view("handler /"), &via_handler)

    // The shared bundle.
    var bundle = site.bundle_js_url()
    var bundle_path = std::string()
    bundle_path.append_view("/")
    bundle_path.append_view(&bundle)
    var bundle_path_view = bundle_path.to_view()
    var bundle_req = request_for(bundle_path_view)
    var bundle_res = webserve.serve(site_ptr, &bundle_req)
    report(std::string_view("serve bundle"), &bundle_res)

    // A conditional request built from the first response's validator.
    var cached_req = request_for("/")
    var inm_name = std::string_view("If-None-Match")
    var root_etag = header_or_empty(&root, std::string_view("ETag"))
    cached_req.headers.insert_view(&inm_name, &root_etag)
    var cached = webserve.serve(site_ptr, &cached_req)
    report(std::string_view("serve / if-none-match"), &cached)

    // A stale validator gets the body again.
    var stale_req = request_for("/")
    var stale_etag = std::string_view("\"stale\"")
    stale_req.headers.insert_view(&inm_name, &stale_etag)
    var stale = webserve.serve(site_ptr, &stale_req)
    report(std::string_view("serve / stale-etag"), &stale)

    // Missing paths and traversal attempts.
    var missing_req = request_for("/nope")
    var missing = webserve.serve(site_ptr, &missing_req)
    report(std::string_view("serve /nope"), &missing)
    var traversal_req = request_for("/../secret")
    var traversal = webserve.serve(site_ptr, &traversal_req)
    report(std::string_view("serve /../secret"), &traversal)

    // No installed site is a 503, never a crash.
    var uninstalled = webserve.serve(null, &root_req)
    report(std::string_view("serve null"), &uninstalled)

    return 0
}
