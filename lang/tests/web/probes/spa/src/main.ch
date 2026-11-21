// Probe 3 driver: build the SPA shell, then answer the two requests the browser
// makes (the shell, and the JSON it fetches). Prints machine-checkable lines.
//
// `web::App` cannot host a JSON endpoint (path-only routing, page-only handlers),
// so the shell render and the endpoint are dispatched by hand — the same
// workaround the storefront probe needed. See `lang/docs/web-stack-gaps.md`.
using std::Option;
using std::string;
using std::string_view;

func string_of(v : string_view) : string {
    var out = string()
    out.append_view(&v)
    return out
}

public func handle(app : &mut web::App, req : &mut http::Request) : http::Response {
    var path = req.path.to_view()
    if(path.equals(&string_view("/api/tasks"))) {
        var body = tasks_json()
        var body_view = body.to_view()
        return http::response_json_text(200u, &body_view)
    }
    return app.handle(req)
}

func body_text(res : &http::Response) : string {
    var text = res.body.text()
    if(text is Option.Some) {
        var Some(v) = text else unsafe { unreachable }
        return string_of(v)
    }
    return string()
}

func has_text(haystack : &string, needle : string_view) : bool {
    return haystack.to_view().contains(&needle)
}

func check(name : string_view, ok : bool) {
    var n = name
    if(ok) {
        unsafe { printf("ok   %.*s\n", n.size() as int, n.data()) }
    } else {
        unsafe { printf("FAIL %.*s\n", n.size() as int, n.data()) }
    }
}

public func main() : int {
    var app = web::App.make("lang/compiled/spa_probe", "spa", "assets")
    app.set_minify(true)
    app.page("/", Shell)

    var built = app.build()
    var msg = app.error_message()
    unsafe { printf("build=%d msg=%.*s\n", built as int, msg.size() as int, msg.data()) }
    if(!built) { return 1 }

    var shell = http::Request()
    shell.method = string_of(string_view("GET"))
    shell.path = string_of(string_view("/"))
    var res = handle(&mut app, &mut shell)
    var body = body_text(&res)
    unsafe { printf("shell status=%d bytes=%zu\n", res.status as int, body.to_view().size()) }
    check(string_view("the shell renders"), res.status == 200u && has_text(&body, string_view("Task board")))
    check(string_view("the shell carries the client bundle"), has_text(&body, string_view("assets/spa.")))
    check(string_view("the shell ships a bootstrap payload"), has_text(&body, string_view("id=\"bootstrap\"")))
    check(string_view("the client mounts into #tasks"), has_text(&body, string_view("id=\"tasks\"")))

    var api_req = http::Request()
    api_req.method = string_of(string_view("GET"))
    api_req.path = string_of(string_view("/api/tasks"))
    var api = handle(&mut app, &mut api_req)
    var api_body = body_text(&api)
    unsafe { printf("api status=%d bytes=%zu\n", api.status as int, api_body.to_view().size()) }
    check(string_view("the JSON endpoint answers"), api.status == 200u && has_text(&api_body, string_view("\"open\":3")))
    check(string_view("the payload carries every task"), has_text(&api_body, string_view("write the shell")))

    var finished = app.finish()
    var finish_msg = app.error_message()
    unsafe {
        printf("finish=%d msg=%.*s\n", finished as int, finish_msg.size() as int, finish_msg.data())
    }
    return 0
}
