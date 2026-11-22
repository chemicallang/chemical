// Probe 3 driver: build the SPA shell, then answer the two requests the browser
// makes (the shell, and the JSON it fetches). Prints machine-checkable lines.
//
// The shell is a pre-rendered route and the JSON the client fetches is an
// `app.endpoint`, so the app answers both: nothing here dispatches by hand.
using std::Option;
using std::string;
using std::string_view;

func string_of(v : string_view) : string {
    var out = string()
    out.append_view(&v)
    return out
}

public func Tasks(ctx : &mut web::RequestContext) : http::Response {
    var body = tasks_json()
    var body_view = body.to_view()
    return web::json(200u, &body_view)
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
    app.endpoint("GET", "/api/tasks", Tasks)

    var built = app.build()
    var msg = app.error_message()
    unsafe { printf("build=%d msg=%.*s\n", built as int, msg.size() as int, msg.data()) }
    if(!built) { return 1 }

    var shell = http::request(string_view("GET"), string_view("/"))
    var res = app.handle(&raw mut shell)
    var body = body_text(&res)
    unsafe { printf("shell status=%d bytes=%zu\n", res.status as int, body.to_view().size()) }
    check(string_view("the shell renders"), res.status == 200u && has_text(&body, string_view("Task board")))
    // The shell renders no universal component, so it must not ship the hydration
    // runtime it never runs. What it *should* carry is the typed client module
    // (`client/*.ch` -> `web-client.mjs`), which nothing connects to the page yet
    // (`web-stack-gaps.md` P0-2).
    check(string_view("a client-only shell carries no hydration runtime"),
        !has_text(&body, string_view("assets/spa.")))
    check(string_view("the shell ships a bootstrap payload"), has_text(&body, string_view("id=\"bootstrap\"")))
    check(string_view("the client mounts into #tasks"), has_text(&body, string_view("id=\"tasks\"")))

    var api_req = http::request(string_view("GET"), string_view("/api/tasks"))
    var api = app.handle(&raw mut api_req)
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
