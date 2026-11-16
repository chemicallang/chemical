// webserve over a real socket: build -> install -> http::Server -> wire.
//
// `fixtures/webserve` proves the request -> response mapping without a socket.
// This fixture covers what that cannot: the catch-all route, the framework's own
// header encoding, keep-alive framing and the 304 on the wire. It dials with raw
// sockets (like `lang/tests/server/src/http_framing_test.ch`) because a normal
// client normalises exactly the bytes under test.
//
// `scripts/static-site-test.sh` runs it and greps the printed header blocks.

using std::Option;
using std::string;
using std::string_view;
using std::Result;

#universal SocketCounter(props) {
    state n = 0
    return <button class="counter" onClick={() => { n = n + 1 }}>{n}</button>
}

func render_index(page : &mut HtmlPage) {
    page.appendTitle("Index")
    #html {
        <div class="page">
            <h1>Index</h1>
            <SocketCounter />
        </div>
    }
}

// Send `raw` on a fresh connection to 127.0.0.1:port and read until the peer
// closes. Returns the response text, or an empty string when nothing arrived.
func raw_exchange(port : uint, raw : &string, budget_ms : int) : string {
    unsafe {
        var acc = string::empty_str()
        var s = net::raw::dial("127.0.0.1", port)
        if(s == 0u || (s as longlong) < 0) { return acc }
        net::raw::set_recv_timeout(s, budget_ms / 1000, 0)
        net::raw::send_all(s, raw.data(), raw.size() as int)
        var chunk : [4096]u8
        while(true) {
            var n = net::raw::recv_all(s, &raw mut chunk[0], 4096u)
            if(n <= 0) { break }
            var piece = std::string_view(&chunk[0] as *char, n as size_t)
            acc.append_view(&piece)
        }
        net::raw::close_socket(s)
        return acc
    }
}

// The response head (status line + headers), i.e. everything before the blank
// line. Printing just this keeps 100+ KB of bundle out of the log.
func head_of(raw : &string) : std::string_view {
    var text = raw.to_view()
    var i : size_t = 0
    while(i + 3 < text.size()) {
        if(text.at(i) == '\r' && text.at(i + 1) == '\n' && text.at(i + 2) == '\r' && text.at(i + 3) == '\n') {
            return unsafe(std::string_view(text.data(), i))
        }
        i = i + 1
    }
    return text
}

func get_request(path : std::string_view) : string {
    var out = string("GET ")
    out.append_view(&path)
    out.append_view(" HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n")
    return out
}

func print_exchange(label : std::string_view, port : uint, raw : &string) {
    var resp = raw_exchange(port, raw, 3000)
    if(resp.size() == 0) {
        unsafe { printf("%s -> NO RESPONSE\n", label.data()) }
        return
    }
    var head = head_of(&resp)
    // Status line + header block; the body is asserted by size only.
    var body_bytes = resp.size() - head.size() - 4
    var summary = string()
    summary.append_view(&head)
    unsafe {
        printf("%s -> %s | %llu B body\n", label.data(), summary.data(), body_bytes)
    }
}

public func main() : int {
    const PORT : uint = 19897u

    var site = StaticSite.make("lang/compiled/webserve_socket_out", "app", "assets")

    var index = HtmlPage()
    site.add(&mut index, "index.html")
    index.defaultUniversalSetup()
    render_index(&mut index)

    if(!site.build()) {
        unsafe { printf("build error: %s\n", site.error_message().data()) }
        return 1
    }
    unsafe { webserve.install(&raw mut site) }

    var cfg = server::ServerConfig()
    var addr = string("127.0.0.1:")
    addr.append_uinteger(PORT as ubigint)
    cfg.addr = addr
    cfg.header_timeout_secs = 2

    // A fixed port, like the http framing tests use (19890+). A busy port fails
    // loudly, which is the honest outcome for a gate.
    var bound = server.Server.bind(cfg)
    if(bound is Result.Err) {
        unsafe { printf("could not bind 127.0.0.1:19897\n") }
        return 1
    }
    var Ok(srv) = bound else unsafe { unreachable }

    srv.router.add("GET", "/:path*", ||(req) => {
        return webserve.respond_to_request(req)
    })

    var stop = srv.handle()
    var thread = srv.serve_async()
    std::concurrent.sleep_ms(300u)

    // A page, the shared bundle, an unknown route, and a conditional request.
    var root_req = get_request("/")
    print_exchange(std::string_view("GET /"), PORT, &root_req)

    var bundle = site.bundle_js_url()
    var bundle_path = string("/")
    bundle_path.append_view(&bundle)
    var bundle_path_view = bundle_path.to_view()
    var bundle_req = get_request(bundle_path_view)
    print_exchange(std::string_view("GET bundle"), PORT, &bundle_req)

    var missing_req = get_request("/nope")
    print_exchange(std::string_view("GET /nope"), PORT, &missing_req)

    // The ETag the page carries, sent back as If-None-Match, must be a 304 with
    // no body on the wire.
    var first = raw_exchange(PORT, &root_req, 3000)
    var etag_value = std::string_view("")
    var head = head_of(&first)
    var i : size_t = 0
    while(i + 5 < head.size()) {
        if(head.at(i) == 'E' && head.at(i + 1) == 'T' && head.at(i + 2) == 'a' && head.at(i + 3) == 'g' && head.at(i + 4) == ':' && head.at(i + 5) == ' ') {
            var start = i + 6
            var end = start
            while(end < head.size() && head.at(end) != '\r') { end = end + 1 }
            etag_value = unsafe(std::string_view(head.data() + start, end - start))
            break
        }
        i = i + 1
    }
    var conditional = string("GET / HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\nIf-None-Match: ")
    conditional.append_view(&etag_value)
    conditional.append_view("\r\n\r\n")
    print_exchange(std::string_view("GET / if-none-match"), PORT, &conditional)

    stop.shutdown()
    thread.join()
    return 0
}
