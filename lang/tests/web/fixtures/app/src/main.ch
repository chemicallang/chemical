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
