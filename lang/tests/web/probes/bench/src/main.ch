// Runtime benchmark for a served `web::App`.
//
// The five probes prove the API answers correctly; this one measures how fast the
// answering is, and splits the number so an improvement has a target:
//
//   * `app.handle` only            — the framework (routing + render + artifact
//                                    lookup + gzip/ETag), no socket;
//   * `app.handle` over loopback   — adds accept, read, write, close per request
//                                    (one fresh connection each time, like a
//                                    browser's asset loads);
//   * the three delivery shapes     — a frozen pre-rendered page, a frozen shell
//                                    filled per request, and a per-request render
//                                    — because they are three different costs and
//                                    an app picks one per route.
//
// Everything is reported in microseconds with a p50/p95, plus requests/second, so a
// change can be judged instead of guessed at. Output is one machine-readable line
// per measurement:
//
//   bench route=/ kind=frozen inproc_us=… p50_us=… p95_us=… socket_us=… rps=…
//
// Run: build the module, run the binary, `grep '^bench'`.
using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;

const BENCH_PORT : uint = 19945u
const BENCH_REQUESTS : int = 300
const BENCH_ITEMS : int = 24

// ── the workload ───────────────────────────────────────────────────────────

@direct_init
struct Item {
    var id : int
    var title : string
    var open : bool
}

func seeded() : vector<Item> {
    var out = vector<Item>()
    var i = 0
    while(i < BENCH_ITEMS) {
        var title = string("item ")
        title.append_integer(i as bigint)
        var entry = Item { id : i, title : title, open : (i % 3) != 0 }
        out.push_back(entry)
        i = i + 1
    }
    return out
}

// The shared inner markup: a list with one `<li>` per item, some classes, an
// attribute and an interpolated title — the shapes a real page is made of.
func render_items(page : &mut HtmlPage, items : &mut vector<Item>) {
    var i : ubigint = 0
    while(i < items.size()) {
        unsafe {
        var item = items.get_ptr(i)
        var title_view = item.title.to_view()
        var state = "todo"
        if(!item.open) { state = "todo done" }
        var href = string("/items/")
        href.append_integer(item.id as bigint)
        var href_view = href.to_view()
        #html {
            <li class={state}>
                <a class="item-link" href={href_view}>{title_view}</a>
                <span class="meta">{"#"}{item.id}</span>
            </li>
        }
        }
        i = i + 1
    }
}

// A frozen page: the whole document is rendered once by `build()`. This is the
// cheapest delivery shape — a request is an artifact lookup.
public func Home(ctx : &mut web::RequestContext, page : &mut HtmlPage) {
    var items = seeded()
    page.appendTitle("Bench — frozen")
    #html {
        <main class="board">
            <h1>Frozen page</h1>
            <ul class="items">@{render_items(page, &mut items)}</ul>
        </main>
    }
}

// A frozen shell: the chrome is compiled once and each request only splices the
// values it reads from the request (`web::App.shell`).
public func ShellFill(ctx : &mut web::RequestContext) : web::SlotFills {
    var label = string("hello ")
    var who = ctx.query("user")
    if(who is Option.Some) {
        var Some(name) = who else unsafe { unreachable }
        label.append_view(&name)
    }
    var view = label.to_view()
    var fills = web::SlotFills()
    fills.set_text("greeting", view)
    fills.set_text("counter", std::string_view("7"))
    return fills
}

public func ShellPage(ctx : &mut web::RequestContext, page : &mut HtmlPage) {
    var items = seeded()
    page.appendTitle("Bench — shell")
    #html {
        <main class="board">
            <h1>Shell</h1>
            <p class="greeting">@{page.slot("greeting")}</p>
            <span class="counter">@{page.slot("counter")}</span>
            <ul class="items">@{render_items(page, &mut items)}</ul>
        </main>
    }
}

// A per-request render: the whole document is produced on every request, which is
// what a page needs when its content depends on the session.
public func Live(ctx : &mut web::RequestContext, page : &mut HtmlPage) {
    var items = seeded()
    page.appendTitle("Bench — live")
    var who = string("anonymous")
    var asked = ctx.query("user")
    if(asked is Option.Some) {
        var Some(name) = asked else unsafe { unreachable }
        who.clear()
        who.append_view(&name)
    }
    var who_view = who.to_view()
    #html {
        <main class="board">
            <h1>Live</h1>
            <p class="greeting">{"hello "}{who_view}</p>
            <ul class="items">@{render_items(page, &mut items)}</ul>
        </main>
    }
}

// ── plumbing ───────────────────────────────────────────────────────────────

func body_text(res : &http::Response) : string {
    var text = res.body.text()
    if(text is Option.Some) {
        var Some(v) = text else unsafe { unreachable }
        return string(v)
    }
    return string()
}

func route_request(path : string_view) : http::Request {
    var req = http::request(string_view("GET"), path)
    return req
}

// `app.handle` for `path`, `requests` times; returns per-request micros.
func bench_inproc(app : &mut web::App, path : string_view, requests : int) : vector<i64> {
    var out = vector<i64>()
    out.reserve(requests as ubigint)
    var i = 0
    while(i < requests) {
        var req = route_request(path)
        var started = std::chrono::Instant::now()
        var res = app.handle(&raw mut req)
        var micros = started.elapsed().as_micros()
        // Keep the response alive long enough that its body is not optimised away.
        var size = res.body.size()
        if(size == 0u) { unsafe { panic("empty body") } }
        out.push_back(micros)
        i = i + 1
    }
    return out
}

// One fresh TCP connection per request — the accept/read/write/close cost a browser
// pays for every asset it fetches.
func bench_socket(port : uint, path : string_view, requests : int) : vector<i64> {
    var out = vector<i64>()
    out.reserve(requests as ubigint)
    var head = string("GET ")
    head.append_view(&path)
    head.append_view(" HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n")
    var i = 0
    while(i < requests) {
        unsafe {
            var s = net::raw::dial("127.0.0.1", port)
            if((s as longlong) <= 0) { panic("bench: could not dial") }
            net::raw::set_recv_timeout(s, 2, 0)
            var started = std::chrono::Instant::now()
            net::raw::send_all(s, head.data(), head.size() as int)
            var chunk : [8192]u8
            var total : size_t = 0
            while(true) {
                var n = net::raw::recv_all(s, &raw mut chunk[0], 8192u)
                if(n <= 0) { break }
                total = total + (n as size_t)
            }
            var micros = started.elapsed().as_micros()
            net::raw::close_socket(s)
            if(total == 0u) { panic("bench: empty response") }
            out.push_back(micros)
        }
        i = i + 1
    }
    return out
}

// Headers + body of one response, reused across requests on one connection.
@direct_init
struct Wire {
    var socket : ubigint
    var pending : string
}

func wire_recv_more(wire : &mut Wire) : bool {
    var chunk : [8192]u8
    var got = unsafe(net::raw::recv_all(wire.socket, &raw mut chunk[0], 8192u))
    if(got <= 0) { return false }
    var piece = unsafe(std::string_view(&chunk[0] as *char, got as size_t))
    wire.pending.append_view(&piece)
    return true
}

// A keep-alive round trip: send on the open socket, read exactly one response
// (Content-Length framed), and leave the rest of the bytes for the next request.
func wire_round_trip(wire : &mut Wire, request : &string) : i64 {
    unsafe {
    var started = std::chrono::Instant::now()
    net::raw::send_all(wire.socket, request.data(), request.size() as int)
    var sep = std::string_view("\r\n\r\n")
    var head_end : size_t = 0
    var found = false
    while(!found) {
        var at = wire.pending.to_view().find(&sep)
        if(at is Option.Some) {
            var Some(position) = at else unsafe { unreachable }
            head_end = position
            found = true
        } else if(!wire_recv_more(wire)) {
            panic("bench: connection closed mid-response")
        }
    }
    var length : size_t = 0
    var header = wire.pending.to_view().subview(0, head_end)
    var marker = std::string_view("content-length: ")
    var at_len = header.find(&marker)
    if(at_len is Option.Some) {
        var Some(position) = at_len else unsafe { unreachable }
        var value_start = position + marker.size()
        var value_end = value_start
        while(value_end < header.size() && header.at(value_end) != '\r') { value_end = value_end + 1 }
        var digits = header.subview(value_start, value_end)
        var i : size_t = 0
        while(i < digits.size()) {
            length = length * 10 + (digits.at(i) as size_t - 48)
            i = i + 1
        }
    }
    var total = head_end + 4 + length
    while(wire.pending.size() < total) {
        if(!wire_recv_more(wire)) { panic("bench: connection closed mid-body") }
    }
    // Drop the consumed bytes; the tail (a pipelined next response) stays.
    var rest = wire.pending.to_view().subview(total, wire.pending.size())
    wire.pending.clear()
    wire.pending.append_view(&rest)
    return started.elapsed().as_micros()
    }
}

// One connection, `requests` round trips: what a browser does for a page and its
// assets, and what the server pays when it does not have to accept/close per
// request.
func bench_socket_reuse(port : uint, path : string_view, requests : int) : vector<i64> {
    var out = vector<i64>()
    out.reserve(requests as ubigint)
    var head = string("GET ")
    head.append_view(&path)
    head.append_view(" HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: keep-alive\r\n\r\n")
    unsafe {
    var s = net::raw::dial("127.0.0.1", port)
    if((s as longlong) <= 0) { panic("bench: could not dial") }
    net::raw::set_recv_timeout(s, 2, 0)
    var wire = Wire { socket : s, pending : string() }
    var i = 0
    while(i < requests) {
        out.push_back(wire_round_trip(&mut wire, &head))
        i = i + 1
    }
    net::raw::close_socket(s)
    }
    return out
}

func sum_of(samples : &vector<i64>) : i64 {
    var total : i64 = 0
    var i : ubigint = 0
    while(i < samples.size()) {
        unsafe { total = total + *samples.get_ptr(i) }
        i = i + 1
    }
    return total
}

func percentile(samples : &vector<i64>, num : ubigint, den : ubigint) : i64 {
    var copy = vector<i64>()
    copy.reserve(samples.size())
    var i : ubigint = 0
    while(i < samples.size()) {
        unsafe { copy.push_back(*samples.get_ptr(i)) }
        i = i + 1
    }
    // Insertion sort: 300 samples, no need for anything cleverer.
    var a : ubigint = 1
    while(a < copy.size()) {
        unsafe {
            var key = *copy.get_ptr(a)
            var b = a
            while(b > 0 && *copy.get_ptr(b - 1) > key) {
                var prev = *copy.get_ptr(b - 1)
                copy.set(b, prev)
                b = b - 1
            }
            copy.set(b, key)
        }
        a = a + 1
    }
    if(copy.size() == 0) { return 0 }
    var at = (copy.size() * num) / den
    if(at >= copy.size()) { at = copy.size() - 1 }
    return unsafe(*copy.get_ptr(at))
}

func report(route : string_view, kind : string_view, inproc : &vector<i64>,
            socket : &vector<i64>, reuse : &vector<i64>) {
    var inproc_total = sum_of(inproc)
    var inproc_n = inproc.size()
    var socket_total = sum_of(socket)
    var socket_n = socket.size()
    var inproc_mean : i64 = 0
    if(inproc_n > 0) { inproc_mean = inproc_total / (inproc_n as i64) }
    var socket_mean : i64 = 0
    var rps : i64 = 0
    if(socket_n > 0 && socket_total > 0) {
        socket_mean = socket_total / (socket_n as i64)
        rps = (socket_n as i64) * 1000000 / socket_total
    }
    var inproc_p50 = percentile(inproc, 50u, 100u)
    var inproc_p95 = percentile(inproc, 95u, 100u)
    var socket_p50 = percentile(socket, 50u, 100u)
    var socket_p95 = percentile(socket, 95u, 100u)
    var reuse_total = sum_of(reuse)
    var reuse_n = reuse.size()
    var reuse_mean : i64 = 0
    var reuse_rps : i64 = 0
    if(reuse_n > 0 && reuse_total > 0) {
        reuse_mean = reuse_total / (reuse_n as i64)
        reuse_rps = (reuse_n as i64) * 1000000 / reuse_total
    }
    var reuse_p50 = percentile(reuse, 50u, 100u)
    var reuse_p95 = percentile(reuse, 95u, 100u)
    unsafe {
        printf("bench route=%.*s kind=%.*s inproc_us=%lld inproc_p50_us=%lld inproc_p95_us=%lld socket_us=%lld socket_p50_us=%lld socket_p95_us=%lld rps=%lld keepalive_us=%lld keepalive_p50_us=%lld keepalive_p95_us=%lld keepalive_rps=%lld\n",
            route.size() as int, route.data(),
            kind.size() as int, kind.data(),
            inproc_mean, inproc_p50, inproc_p95,
            socket_mean, socket_p50, socket_p95, rps,
            reuse_mean, reuse_p50, reuse_p95, reuse_rps)
    }
}

// ── main ───────────────────────────────────────────────────────────────────

public func main() : int {
    var app = web::App.make("lang/compiled/bench_probe", "bench", "assets")
    app.page("/", Home)
    app.shell("/shell", ShellPage, ShellFill)
    app.live("/live", Live)
    if(!app.build()) {
        var msg = app.error_message()
        unsafe { printf("bench build failed: %.*s\n", msg.size() as int, msg.data()) }
        return 1
    }

    // The app answers every request the server receives.
    unsafe { web::install(&raw mut app) }
    var cfg = server.ServerConfig()
    cfg.addr = bench_addr()
    var bound = server.Server.bind(cfg)
    if(bound is Result.Err) {
        unsafe { printf("bench: could not bind 127.0.0.1:%u\n", BENCH_PORT) }
        return 1
    }
    var Ok(srv) = bound else unsafe { unreachable }
    srv.router.add("GET", "/:path*", ||(req) => { return web::serve_request(req) })
    var stop = srv.handle()
    var thread = srv.serve_async()
    std::concurrent.sleep_ms(250u)

    // A long-running mode for driving the same app with an external client
    // (`BENCH_SERVE=1 ./bench_probe` then `curl`), so a number that looks wrong in
    // the built-in client can be checked against a second implementation.
    if(unsafe(getenv("BENCH_SERVE")) != null) {
        unsafe { printf("serving on 127.0.0.1:%u\n", BENCH_PORT) }
        std::concurrent.sleep_ms(30000u)
        stop.shutdown()
        thread.join()
        return 0
    }

    var routes = vector<string>()
    routes.push_back(string("/"))
    routes.push_back(string("/shell?user=ada"))
    routes.push_back(string("/live?user=ada"))
    var kinds = vector<string>()
    kinds.push_back(string("frozen"))
    kinds.push_back(string("shell"))
    kinds.push_back(string("live"))
    var sockets = vector<string>()
    sockets.push_back(string("/"))
    sockets.push_back(string("/shell?user=ada"))
    sockets.push_back(string("/live?user=ada"))

    var i : ubigint = 0
    while(i < routes.size()) {
        unsafe {
        var route = routes.get_ptr(i)
        var kind = kinds.get_ptr(i)
        var socket_path = sockets.get_ptr(i)
        var route_view = route.to_view()
        var kind_view = kind.to_view()
        var socket_view = socket_path.to_view()
        // In-process: the framework alone.
        var inproc = bench_inproc(&mut app, route_view, BENCH_REQUESTS)
        // Over loopback: the same route through a real socket.
        var socket = bench_socket(BENCH_PORT, socket_view, BENCH_REQUESTS)
        var reuse = bench_socket_reuse(BENCH_PORT, socket_view, BENCH_REQUESTS)
        report(route_view, kind_view, &inproc, &socket, &reuse)
        }
        i = i + 1
    }

    // What the site ships, so a size change is as visible as a speed change.
    unsafe {
        printf("bench artifacts=%zu frozen_page_bytes=%zu\n",
            app.static_site().artifact_count(),
            app.static_site().artifact_bytes(0).size())
    }

    stop.shutdown()
    thread.join()
    return 0
}

func bench_addr() : string {
    var addr = string("127.0.0.1:")
    addr.append_integer(BENCH_PORT as bigint)
    return addr
}
