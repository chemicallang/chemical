// Mixed-delivery fixture for `web::App`: two pre-rendered routes, two per-request
// routes, a redirect, a 404 route and a mounted copy, all from one bundle.
//
// The executable prints one line per check so `scripts/web-app-test.sh` can assert
// the serving contract against the real bytes.
using std::Option;
using std::string;
using std::string_view;

public func Home(page : &mut HtmlPage, req : &http::Request) {
    #html {
        <main>
            <h1>Home</h1>
            <p>pre-rendered once</p>
        </main>
    }
}

public func Pricing(page : &mut HtmlPage, req : &http::Request) {
    #html {
        <main>
            <h1>Pricing</h1>
            <p>three plans</p>
        </main>
    }
}

public func Account(page : &mut HtmlPage, req : &http::Request) {
    var who = req.param("id")
    var id = string_view("?")
    if(who is Option.Some) {
        var Some(v) = who else unsafe { unreachable }
        id = v
    }
    #html {
        <main>
            <h1>Account</h1>
            <p>id={id}</p>
        </main>
    }
}

public func Files(page : &mut HtmlPage, req : &http::Request) {
    var rest = req.param("path")
    var path = string_view("?")
    if(rest is Option.Some) {
        var Some(v) = rest else unsafe { unreachable }
        path = v
    }
    #html { <main><h1>File</h1><p>path={path}</p></main> }
}

public func Missing(page : &mut HtmlPage, req : &http::Request) {
    #html { <main><h1>Nothing here</h1></main> }
}

// ── Shell + slots (§3.12) ──────────────────────────────────────────────────
//
// `DashboardShell` marks its personalised region with `page.slot("account")`; the
// chrome around it (a header plus a universal-component body) is rendered once and
// frozen. `DashboardFull` is the full-SSR twin: the identical markup with the same
// value inlined, so the gate can compare the two documents byte for byte.
#universal DashBody(props) {
    return <section class="route dashboard"><h1>Dashboard</h1><p>rows</p></section>
}

public func DashboardShell(page : &mut HtmlPage, req : &http::Request) {
    page.append_html("<div class=\"dash\"><header class=\"bar\">")
    page.slot("account")
    page.append_html("</header>")
    #html { <DashBody /> }
    page.append_html("</div>")
}

public func DashboardFull(page : &mut HtmlPage, req : &http::Request) {
    page.append_html("<div class=\"dash\"><header class=\"bar\">nobody</header>")
    #html { <DashBody /> }
    page.append_html("</div>")
}

public func DashboardFills(req : &http::Request) : web::SlotFills {
    var f = web::SlotFills()
    // Text, so a hostile value cannot break out of the region.
    f.set_text("account", "nobody")
    // A slot the shell does not have: the shell decides the positions, so this is
    // ignored rather than appended somewhere arbitrary.
    f.set("not_a_slot", "<b>ignored</b>")
    return f
}

public func DashboardEscaped(page : &mut HtmlPage, req : &http::Request) {
    page.append_html("<div class=\"dash\"><header class=\"bar\">")
    page.slot("account")
    page.append_html("</header></div>")
}

public func EscapedFills(req : &http::Request) : web::SlotFills {
    var f = web::SlotFills()
    f.set_text("account", "<script>alert(1)</script>")
    return f
}


// ── Sessions ────────────────────────────────────────────────────────────────
func <T> flag(found : Option<T>) : int {
    if(found is Option.Some) { return 1 }
    return 0
}

func run_session_flow() {
    var store = web::SessionStore.make()
    var empty = http::Request()
    var anonymous = store.load(&empty)
    unsafe {
        printf("session anonymous=%d count=%zu\n", flag(anonymous), store.count())
    }

    var session = store.create()
    var id = session.id.to_view()
    unsafe {
        printf("session created id_len=%zu count=%zu\n", id.size(), store.count())
    }

    store.set(&session, "user", "ada")
    store.set(&session, "role", "admin")
    var token = store.csrf_token(&session)
    var token_copy = string()
    token_copy.append_view(&token)
    unsafe {
        printf("session values user=%d role=%d csrf_len=%zu count=%zu\n",
            flag(store.get(&session, "user")),
            flag(store.get(&session, "role")),
            token_copy.size(), store.count())
    }
    var fetched = store.get(&session, "user")
    var user = string_view("")
    if(fetched is Option.Some) {
        var Some(v) = fetched else unsafe { unreachable }
        user = v
    }
    unsafe { printf("session user=%.*s\n", user.size() as int, user.data()) }

    var res = http::Response()
    store.attach(&mut res, &session)

    var set_cookie = string_view("")
    var i : size_t = 0
    while(i < res.headers.len()) {
        var hname = res.headers.name_at(i)
        if(hname.equals(string_view("Set-Cookie"))) {
            set_cookie = res.headers.value_at(i)
        }
        i = i + 1
    }
    unsafe {
        printf("session setcookie=%.*s\n", set_cookie.size() as int, set_cookie.data())
    }
    var opts = http::CookieOptions.make("/", "Lax", true, true, 3600)
    var custom = http::set_cookie_value("theme", "dark", &opts)
    var cv = custom.to_view()
    unsafe {
        printf("cookie custom=%.*s\n", cv.size() as int, cv.data())
    }
    var bad = http::set_cookie_value("bad name", "x", &opts)
    unsafe { printf("cookie bad_len=%zu\n", bad.size()) }

    // A request carrying the cookie loads the same session back.
    var req = http::Request()
    var cookie_name = web::session_cookie_name()
    var header = string()
    header.append_view(&cookie_name)
    header.append('=')
    header.append_view(&id)
    var hdr_view = header.to_view()
    var key = string()
    key.append_view("Cookie")
    req.headers.insert(key, header)
    var loaded = store.load(&req)
    var loaded_user = string_view("")
    if(loaded is Option.Some) {
        var Some(s) = loaded else unsafe { unreachable }
        var u = store.get(&s, "user")
        if(u is Option.Some) {
            var Some(uv) = u else unsafe { unreachable }
            loaded_user = uv
        }
    }
    unsafe {
        printf("session loaded=%d user=%.*s\n", flag(loaded), loaded_user.size() as int, loaded_user.data())
    }

    // CSRF: the token itself verifies, any other value does not.
    var ok = store.verify_csrf(&session, token_copy.to_view())
    var wrong = store.verify_csrf(&session, string_view("nope"))
    unsafe { printf("session csrf_ok=%d csrf_bad=%d\n", ok as int, wrong as int) }

    // Parse a jar with two cookies.
    var jar = http::CookieJar.of(&req)
    var extra = string()
    extra.append_view("theme=dark; other=1")
    jar.parse(extra.to_view())
    var theme = string_view("")
    var got = jar.get(string_view("theme"))
    if(got is Option.Some) {
        var Some(tv) = got else unsafe { unreachable }
        theme = tv
    }
    unsafe {
        printf("jar cookies=%zu theme=%.*s\n", jar.len(), theme.size() as int, theme.data())
    }

    // Two new sessions never share an id.
    var a = store.create()
    var b = store.create()
    var av = a.id.to_view()
    var bv = b.id.to_view()
    unsafe { printf("session distinct=%d\n", (!av.equals(&bv)) as int) }

    // Destroy ends it on the server too.
    var res2 = http::Response()
    store.destroy(&mut res2, &session)
    var after = store.load(&req)
    unsafe {
        printf("session destroyed loaded=%d count=%zu\n", flag(after), store.count())
    }
}

func header(res : &http::Response, name : &str) : string_view {
    var out = string_view("")
    var got = res.headers.get(name)
    if(got is Option.Some) {
        var Some(v) = got else unsafe { unreachable }
        out = v
    }
    return out
}

func body_of(res : &http::Response) : string_view {
    var out = string_view("")
    if(res.body is http::ResponseBody.Text) {
        unsafe {
            var Text(text) = res.body else unreachable
            out = text.to_view()
        }
    }
    return out
}

func request(app : &mut web::App, path : string_view, inm : string_view) : http::Response {
    var req = http::Request()
    req.path.append_view(&path)
    if(!inm.empty()) {
        var k = string()
        k.append_view("If-None-Match")
        var v = string()
        v.append_view(&inm)
        req.headers.insert(k, v)
    }
    return app.handle(&mut req)
}

func bodies_equal(a : &http::Response, b : &http::Response) : bool {
    var av = body_of(a)
    var bv = body_of(b)
    return av.equals(&bv)
}

func contains_text(body : &string_view, needle : string_view) : bool {
    return body.contains(&needle)
}

// Nanoseconds for `iterations` requests to `path`. Each iteration includes the
// response body copy, so the difference is the render cost alone.
func time_path(app : &mut web::App, path : string_view, iterations : int) : i64 {
    var start = std::chrono::Instant.now()
    var i : int = 0
    while(i < iterations) {
        var res = request(app, path, "")
        var body = body_of(&res)
        if(body.size() == 0) { return -1 }
        i = i + 1
    }
    var dur = start.elapsed()
    return dur.as_nanos()
}

func report(label : string_view, res : &http::Response) {
    var cc = header(res, "Cache-Control")
    var etag = header(res, "ETag")
    var loc = header(res, "Location")
    var body = body_of(res)
    unsafe {
        printf("app %.*s -> %d | %.*s | %.*s | %.*s | %zu\n",
            label.size() as int, label.data(), res.status as int,
            cc.size() as int, cc.data(),
            etag.size() as int, etag.data(),
            loc.size() as int, loc.data(),
            body.size())
    }
}

// Bodies are printed one line each: a newline would split a body across log lines
// and hide the rest of it from a line-oriented check.
func report_body(label : string_view, res : &http::Response) {
    var body = body_of(res)
    var flat = string()
    var i : ubigint = 0
    while(i < body.size()) {
        var c = body.at(i)
        if(c == '\n') {
            flat.append_view("\\n")
        } else {
            flat.append(c)
        }
        i = i + 1
    }
    var fv = flat.to_view()
    unsafe { printf("body %.*s = %.*s\n", label.size() as int, label.data(), fv.size() as int, fv.data()) }
}

public func main() : int {
    var app = web::App.make("lang/compiled/app_out", "app", "assets")
    app.page("/", Home)
    app.page("/pricing", Pricing)
    app.live("/account/{id}", Account)
    app.live("/files/{path*}", Files)
    app.shell("/dash", DashboardShell, DashboardFills)
    app.shell("/dash-escaped", DashboardEscaped, EscapedFills)
    app.live("/dash-full", DashboardFull)
    app.redirect("/prices", "/pricing")
    app.not_found(Missing)
    app.export_sitemap("https://example.com", true)

    var built = app.build()
    var build_msg = app.error_message()
    unsafe { printf("build=%d msg=%.*s\n", built as int, build_msg.size() as int, build_msg.data()) }
    if(!built) { return 1 }

    var site = app.static_site()
    unsafe {
        printf("artifacts=%zu pages=%zu\n", site.artifact_count(), site.page_count())
    }
    var i : ubigint = 0
    while(i < site.artifact_count()) {
        var p = site.artifact_path(i)
        unsafe { printf("artifact %.*s\n", p.size() as int, p.data()) }
        i = i + 1
    }

    var root = request(&mut app, "/", "")
    report("/", &root)
    report_body("root", &root)

    var etag = header(&root, "ETag")
    var conditional = request(&mut app, "/", etag)
    report("/ conditional", &conditional)

    var account = request(&mut app, "/account/42", "")
    report("/account/42", &account)
    report_body("account42", &account)

    var other = request(&mut app, "/account/7", "")
    report("/account/7", &other)
    report_body("account7", &other)

    var files = request(&mut app, "/files/reports/2026/q1.pdf", "")
    report("/files/reports/2026/q1.pdf", &files)
    report_body("files", &files)

    // ── Shell route: frozen chrome, spliced fill ─────────────────────────
    var shell_route = unsafe(app.shell_route_at(0))
    var shell_route_ref = unsafe(&*shell_route)
    unsafe {
        printf("shell routes=%zu frozen=%zu compiled=%d slots=%zu\n",
            app.shell_count(), app.shell_frozen_bytes(),
            shell_route_ref.is_compiled() as int,
            shell_route_ref.slot_count())
    }
    var dash = request(&mut app, "/dash", "")
    report("/dash", &dash)
    var dash_body = body_of(&dash)
    unsafe {
        printf("shell fill=%d chrome=%d\n",
            contains_text(&dash_body, "nobody") as int,
            contains_text(&dash_body, "class=\"route dashboard\"") as int)
    }
    var full = request(&mut app, "/dash-full", "")
    unsafe { printf("shell eq=%d\n", bodies_equal(&dash, &full) as int) }

    var escaped = request(&mut app, "/dash-escaped", "")
    var escaped_body = body_of(&escaped)
    unsafe {
        printf("shell escaped=%d raw=%d\n",
            contains_text(&escaped_body, "&lt;script&gt;") as int,
            contains_text(&escaped_body, "<script>alert") as int)
    }

    // Compile-time contract: a malformed marker is refused, and `emit` refuses to
    // fill a document with the wrong number of values instead of serving a shell
    // with a hole in it.
    var bad = web::Shell.compile("<!DOCTYPE html><html><body><!--chx-slot:oops")
    var one = web::Shell.compile("<p><!--chx-slot:a--></p>")
    var no_values = std::vector<std::string_view>()
    var refused = one.emit(&no_values)
    unsafe {
        printf("shell malformed=%d arity=%d slots=%zu\n",
            (!bad.is_valid()) as int, refused.empty() as int, one.slot_count())
    }

    var iters = 2000
    var shell_ns = time_path(&mut app, "/dash", iters)
    var ssr_ns = time_path(&mut app, "/dash-full", iters)
    unsafe {
        var ratio = 0.0
        if(shell_ns > 0) { ratio = (ssr_ns as double) / (shell_ns as double) }
        printf("shell ns_shell=%lld ns_ssr=%lld ratio=%.2f\n", shell_ns, ssr_ns, ratio)
    }

    var moved = request(&mut app, "/prices", "")
    report("/prices", &moved)

    var missing = request(&mut app, "/nope", "")
    report("/nope", &missing)
    report_body("missing", &missing)

    var escape = request(&mut app, "/assets/../etc/passwd", "")
    report("/assets/../etc/passwd", &escape)

    var sitemap = request(&mut app, "/sitemap.xml", "")
    report("/sitemap.xml", &sitemap)
    report_body("sitemap", &sitemap)

    var robots = request(&mut app, "/robots.txt", "")
    report("/robots.txt", &robots)
    report_body("robots", &robots)

    // One app, mounted under a prefix: the same build (and the same bundle) served
    // at /app/*, with asset URLs that keep the mount.
    var mounted = web::App.make("lang/compiled/app_mount_out", "app", "assets")
    mounted.set_base_path("/app")
    mounted.page("/", Home)
    mounted.live("/account/{id}", Account)
    var mount_built = mounted.build()
    var mount_msg = mounted.error_message()
    unsafe { printf("mount build=%d msg=%.*s\n", mount_built as int, mount_msg.size() as int, mount_msg.data()) }
    if(!mount_built) { return 1 }
    var mroot = request(&mut mounted, "/app/", "")
    report("/app/ mounted", &mroot)
    report_body("mount", &mroot)
    var outside = request(&mut mounted, "/", "")
    report("/ mounted", &outside)

    // The router-shaped entry point serves the installed app.
    unsafe { web::install(&raw mut app) }
    var installed = web::serve_request(http::Request())
    report("installed", &installed)
    web::uninstall()

    // The pre-rendered pages must have reached the filesystem.
    var finished = app.finish()
    var finish_msg = app.error_message()
    unsafe { printf("finish=%d msg=%.*s\n", finished as int, finish_msg.size() as int, finish_msg.data()) }
    if(!finished) { return 1 }

    run_session_flow()

    // A second build of the same app is a no-op, not a re-render.
    var again = request(&mut app, "/account/42", "")
    report("/account/42 again", &again)
    return 0
}
