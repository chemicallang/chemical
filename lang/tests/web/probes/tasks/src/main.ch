// Probe 4 driver: register the authenticated app's routes, then run one request
// suite in-process and print machine-checkable lines.
//
// Gaps this app is built to expose (recorded in `lang/docs/web-stack-gaps.md`):
//   * the application's own state has nowhere to live — `web::App` owns only the
//     session store, so the probe reaches an `@never_destructed` global through one
//     `unsafe` accessor;
//   * a `PageHandler` cannot answer a 303/404/401, so a guarded page renders its
//     "please sign in" body with status 200;
//   * an endpoint cannot render a page, so a failed form POST redirects to a query
//     flag (`/login?e=bad`) instead of re-rendering the form with 422;
//   * a shell route's fills receive the request but not the session;
//   * CSRF verification is a five-line dance repeated in every mutating endpoint;
//   * a request a test builds by hand has no `query`, so query reading needs a
//     fallback that re-parses the path.
using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;

// ── Endpoints ─────────────────────────────────────────────────────────────
func csrf_ok(ctx : &web::RequestContext, form : &http::Form) : bool {
    var token = form.get("csrf")
    if(token is Option.None) { return false }
    var Some(t) = token else unsafe { unreachable }
    return ctx.verify_csrf(t)
}

func text_field(form : &http::Form, name : string_view) : string {
    var found = form.get(name)
    if(found is Option.None) { return string() }
    var Some(value) = found else unsafe { unreachable }
    return string_of(value)
}

public func LoginPost(ctx : &mut web::RequestContext) : http::Response {
    var form = ctx.form()
    if(!csrf_ok(ctx, &form)) { return web::text(403u, &string_view("bad csrf")) }
    var email = text_field(&form, string_view("email"))
    var password = text_field(&form, string_view("password"))
    var email_view = email.to_view()
    var password_view = password.to_view()
    var user = db_find_user(email_view)
    if(user is Option.None) { return web::redirect(303u, string_view("/login?e=bad")) }
    var Some(uid) = user else unsafe { unreachable }
    if(!db_password_ok(uid, password_view)) { return web::redirect(303u, string_view("/login?e=bad")) }
    var uid_text = string()
    uid_text.append_integer(uid)
    ctx.set_session("uid", uid_text.to_view())
    ctx.set_session("email", email_view)
    // A real app rotates the CSRF token on privilege change.
    var hello = string("Welcome back, ")
    hello.append_view(&email_view)
    ctx.set_session("flash", hello.to_view())
    return web::redirect(303u, string_view("/todos"))
}

public func SignupPost(ctx : &mut web::RequestContext) : http::Response {
    var form = ctx.form()
    if(!csrf_ok(ctx, &form)) { return web::text(403u, &string_view("bad csrf")) }
    var email = text_field(&form, string_view("email"))
    var password = text_field(&form, string_view("password"))
    var email_copy = email.to_view()
    var password_copy = password.to_view()
    if(email_copy.find(&string_view("@")) is Option.None) {
        return web::redirect(303u, string_view("/signup?e=email"))
    }
    if(password_copy.size() < 8u) { return web::redirect(303u, string_view("/signup?e=weak")) }
    if(db_find_user(email_copy).is_some()) { return web::redirect(303u, string_view("/signup?e=taken")) }
    var salt : bigint = 0x51a7
    var hash = hash_password(salt, password_copy)
    var uid = db_create_user(email_copy, hash, salt)
    var uid_text = string()
    uid_text.append_integer(uid)
    ctx.set_session("uid", uid_text.to_view())
    ctx.set_session("email", email_copy)
    ctx.set_session("flash", string_view("Account created."))
    return web::redirect(303u, string_view("/todos"))
}

public func LogoutPost(ctx : &mut web::RequestContext) : http::Response {
    var form = ctx.form()
    if(!csrf_ok(ctx, &form)) { return web::text(403u, &string_view("bad csrf")) }
    var res = web::redirect(303u, string_view("/"))
    ctx.destroy_session(&mut res)
    return res
}

public func TodoCreate(ctx : &mut web::RequestContext) : http::Response {
    var form = ctx.form()
    if(!csrf_ok(ctx, &form)) { return web::text(403u, &string_view("bad csrf")) }
    var uid = current_uid(ctx)
    if(uid == 0) { return web::redirect(303u, string_view("/login")) }
    var title = text_field(&form, string_view("title"))
    var title_view = title.to_view()
    if(title_view.empty()) { return web::redirect(303u, string_view("/todos")) }
    db_add_todo(uid, title_view)
    return web::redirect(303u, string_view("/todos"))
}

func param_id(ctx : &web::RequestContext) : bigint {
    var raw = ctx.param("id")
    if(raw is Option.None) { return 0 }
    var Some(value) = raw else unsafe { unreachable }
    var parsed = value.to_int()
    if(parsed is Result.Ok) {
        var Ok(n) = parsed else unsafe { unreachable }
        return n as bigint
    }
    return 0
}

public func TodoToggle(ctx : &mut web::RequestContext) : http::Response {
    var form = ctx.form()
    if(!csrf_ok(ctx, &form)) { return web::text(403u, &string_view("bad csrf")) }
    var uid = current_uid(ctx)
    var id = param_id(ctx)
    // Ownership is part of the lookup: another user's todo is a 404, never a 403
    // that would confirm it exists.
    if(db_find_owned_todo(id, uid) is Option.None) { return web::text(404u, &string_view("no such todo")) }
    db_toggle_todo(id)
    return web::redirect(303u, string_view("/todos"))
}

public func TodoDelete(ctx : &mut web::RequestContext) : http::Response {
    var form = ctx.form()
    if(!csrf_ok(ctx, &form)) { return web::text(403u, &string_view("bad csrf")) }
    var uid = current_uid(ctx)
    var id = param_id(ctx)
    if(db_find_owned_todo(id, uid) is Option.None) { return web::text(404u, &string_view("no such todo")) }
    db_delete_todo(id)
    return web::redirect(303u, string_view("/todos"))
}

func json_escape(value : string_view) : string {
    var out = string()
    var i : ubigint = 0
    while(i < value.size()) {
        var c = value.at(i)
        if(c == '"') { out.append_view("\\\"") }
        else if(c == '\\') { out.append_view("\\\\") }
        else if(c == '\n') { out.append_view("\\n") }
        else { out.append(c) }
        i = i + 1
    }
    return out
}

/// The JSON API the browser polls. A typed response (`#json` + `std::Serializer`)
/// would be the stack's answer; the probe hand-writes it to keep the measurement
/// about routing, not codegen.
public func TodosJson(ctx : &mut web::RequestContext) : http::Response {
    var uid = current_uid(ctx)
    var ids = db_todos_of(uid, true)
    var out = string("[")
    var i : ubigint = 0
    while(i < ids.size()) {
        var id = unsafe(*ids.get_ptr(i))
        if(i > 0u) { out.append(',') }
        out.append_view("{\"id\":")
        out.append_integer(id)
        out.append_view(",\"title\":\"")
        var title = json_escape(db_todo_title(id))
        out.append_string(&title)
        out.append_view("\",\"done\":")
        if(db_todo_done(id)) { out.append_view("true") } else { out.append_view("false") }
        out.append('}')
        i = i + 1
    }
    out.append(']')
    var body = out.to_view()
    return web::json(200u, &body)
}

// ── Suite plumbing ────────────────────────────────────────────────────────
func request_with(app : &mut web::App, method : string_view, path : string_view,
                   cookie : string_view, body : string_view) : http::Response {
    var req = http::Request()
    req.method = string_of(method)
    req.path = string_of(path)
    if(!cookie.empty()) {
        var k = string_view("Cookie")
        var v = string_of(cookie)
        var v_view = v.to_view()
        req.headers.insert_view(&k, &v_view)
    }
    if(!body.empty()) {
        var k = string_view("Content-Type")
        var v = string_view("application/x-www-form-urlencoded")
        req.headers.insert_view(&k, &v)
        req.set_body_text(body)
    }
    return app.handle(&raw mut req)
}

func header_text(res : &http::Response, name : &string_view) : string {
    var i : size_t = 0
    while(i < res.headers.len()) {
        var n = res.headers.name_at(i)
        if(n.equals(name)) {
            return string_of(res.headers.value_at(i))
        }
        i = i + 1
    }
    return string()
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

/// `Set-Cookie: session=abc; Path=/; …` → `session=abc`.
func cookie_pair(cookie : &string) : string_view {
    var view = cookie.to_view()
    var semi = view.find(&string_view(";"))
    if(semi.is_some()) {
        var Some(pos) = semi else unsafe { unreachable }
        return view.subview(0, pos)
    }
    return view
}

func csrf_of(res : &http::Response) : string {
    var body = body_text(res)
    var view = body.to_view()
    var needle = string_view("name=\"csrf\" value=\"")
    var at = view.find(&needle)
    if(at is Option.None) { return string() }
    var Some(pos) = at else unsafe { unreachable }
    var start = pos + needle.size()
    var end = start
    while(end < view.size() && view.at(end) != '"') { end = end + 1 }
    return string_of(view.subview(start, end))
}

func form_text(name : string_view, value : string_view) : string {
    var out = string()
    out.append_view(&name)
    out.append('=')
    out.append_view(&value)
    return out
}

func form2(n1 : string_view, v1 : string_view, n2 : string_view, v2 : string_view) : string {
    var out = form_text(n1, v1)
    out.append('&')
    out.append_view(&n2)
    out.append('=')
    out.append_view(&v2)
    return out
}

func check(name : string_view, ok : bool) {
    var n = name
    if(ok) {
        unsafe { printf("ok   %.*s\n", n.size() as int, n.data()) }
    } else {
        unsafe { printf("FAIL %.*s\n", n.size() as int, n.data()) }
    }
}

/// `/todos/7` out of whatever the rendered delete form asked for.
func todo_id_from(body : &string, marker : string_view) : string {
    var view = body.to_view()
    var at = view.find(&marker)
    if(at is Option.None) { return string() }
    var Some(pos) = at else unsafe { unreachable }
    var start = pos + marker.size()
    var end = start
    while(end < view.size() && view.at(end) >= '0' && view.at(end) <= '9') { end = end + 1 }
    return string_of(view.subview(start, end))
}

public func main() : int {
    db_init()

    var app = web::App.make("lang/compiled/tasks_probe", "tasks", "assets")
    app.set_minify(true)
    app.set_gzip(true)
    app.page("/", Landing)
    app.live("/login", LoginPage)
    app.live("/signup", SignupPage)
    app.live("/todos", TodosPage)
    app.live("/account", AccountPage)
    app.shell("/stats", StatsPage, StatsFills)
    app.endpoint("POST", "/login", LoginPost)
    app.endpoint("POST", "/signup", SignupPost)
    app.endpoint("POST", "/logout", LogoutPost)
    app.endpoint("POST", "/todos", TodoCreate)
    app.endpoint("POST", "/todos/{id}", TodoToggle)
    app.endpoint("POST", "/todos/{id}/delete", TodoDelete)
    app.endpoint("GET", "/api/todos", TodosJson)
    app.not_found(Missing)
    app.export_sitemap("https://tasks.example", true)

    var built = app.build()
    var msg = app.error_message()
    unsafe { printf("build=%d msg=%.*s\n", built as int, msg.size() as int, msg.data()) }
    if(!built) { return 1 }

    var artifacts = app.static_site().artifact_count()
    var shell_bytes = app.shell_frozen_bytes()
    var bundle = string_of(app.static_site().bundle_js_url())
    var bundle_view = bundle.to_view()
    unsafe {
        printf("site artifacts=%zu shell_bytes=%zu bundle=%.*s\n",
            artifacts, shell_bytes, bundle_view.size() as int, bundle_view.data())
    }

    // 1. The pre-rendered landing page: no component, so no JS at all.
    var landing = request_with(&mut app, string_view("GET"), string_view("/"), string_view(""), string_view(""))
    var landing_body = body_text(&landing)
    check(string_view("landing is 200"), landing.status == 200u)
    check(string_view("landing ships no script"), !has_text(&landing_body, string_view("<script")))

    // 2. The guarded page answers 200, not 303 — the handler cannot decide.
    var gate = request_with(&mut app, string_view("GET"), string_view("/todos"), string_view(""), string_view(""))
    var gate_body = body_text(&gate)
    check(string_view("anonymous /todos is a 200 sign-in gate (gap)"),
        gate.status == 200u && has_text(&gate_body, string_view("not signed in")))

    // 3. The login form carries a CSRF token and a session cookie.
    var login = request_with(&mut app, string_view("GET"), string_view("/login"), string_view(""), string_view(""))
    var login_body = body_text(&login)
    var login_token = csrf_of(&login)
    var login_cookie = header_text(&login, &string_view("Set-Cookie"))
    check(string_view("login form has a CSRF field"), login_token.size() > 0)
    check(string_view("login form starts a session"), login_cookie.size() > 0)
    var anon = cookie_pair(&login_cookie)

    // 4. Wrong credentials: a redirect to the query-flag error (an endpoint cannot
    //    re-render the page with 422).
    var bad_body = form2(string_view("csrf"), login_token.to_view(),
        string_view("email"), string_view("ada@example.com"))
    var bad_body2 = string()
    bad_body2.append_string(&bad_body)
    bad_body2.append_view("&password=wrong")
    var bad = request_with(&mut app, string_view("POST"), string_view("/login"), anon, bad_body2.to_view())
    var bad_location = header_text(&bad, &string_view("Location"))
    check(string_view("wrong password is a 303 to /login?e=bad (gap)"),
        bad.status == 303u && has_text(&bad_location, string_view("/login?e=bad")))

    // 5. Correct credentials: 303 into the app, with the session cookie set.
    var good_body = string()
    good_body.append_string(&bad_body)
    good_body.append_view("&password=correct+horse")
    var good = request_with(&mut app, string_view("POST"), string_view("/login"), anon, good_body.to_view())
    var good_location = header_text(&good, &string_view("Location"))
    var session_cookie = header_text(&good, &string_view("Set-Cookie"))
    check(string_view("sign-in is a 303 to /todos"),
        good.status == 303u && has_text(&good_location, string_view("/todos")))
    // The sign-in landed in the session the form's GET already created, so no new
    // cookie is due: `Set-Cookie` appears only when the session is *created*.
    var ada = anon
    if(session_cookie.size() > 0) { ada = cookie_pair(&session_cookie) }

    // 6. The list renders per request, with the flash from the sign-in.
    var list = request_with(&mut app, string_view("GET"), string_view("/todos"), ada, string_view(""))
    var list_body = body_text(&list)
    check(string_view("the list greets the signed-in user"), has_text(&list_body, string_view("Welcome back")))
    check(string_view("the seeded done task is filtered out of the open list"),
        !has_text(&list_body, string_view("Read the web stack survey")))
    check(string_view("the list is no-store"),
        has_text(&header_text(&list, &string_view("Cache-Control")), string_view("no-store")))
    var ada_token = csrf_of(&list)
    check(string_view("the list carries a CSRF token"), ada_token.size() > 0)

    // 7. Create a task through the form.
    var create_body = form2(string_view("csrf"), ada_token.to_view(),
        string_view("title"), string_view("Ship%20the%20probe"))
    var created = request_with(&mut app, string_view("POST"), string_view("/todos"), ada, create_body.to_view())
    check(string_view("creating a task answers 303"), created.status == 303u)
    list = request_with(&mut app, string_view("GET"), string_view("/todos"), ada, string_view(""))
    list_body = body_text(&list)
    check(string_view("the new task is listed"), has_text(&list_body, string_view("Ship the probe")))
    var todo_id = todo_id_from(&list_body, string_view("action=\"/todos/"))
    check(string_view("the row exposes its todo id"), todo_id.size() > 0)

    // 8. Toggle it done, then read both filters.
    var toggle_path = string("/todos/")
    toggle_path.append_string(&todo_id)
    var toggle_path_view = toggle_path.to_view()
    var toggle_body = form_text(string_view("csrf"), ada_token.to_view())
    var toggled = request_with(&mut app, string_view("POST"), toggle_path_view, ada, toggle_body.to_view())
    check(string_view("toggling answers 303"), toggled.status == 303u)
    var done_view = string_view("/todos?status=done")
    var done = request_with(&mut app, string_view("GET"), done_view, ada, string_view(""))
    var done_body = body_text(&done)
    check(string_view("the done filter lists the finished task"),
        has_text(&done_body, string_view("Ship the probe")))
    var open_again = request_with(&mut app, string_view("GET"), string_view("/todos"), ada, string_view(""))
    check(string_view("the open filter hides it"),
        !has_text(&body_text(&open_again), string_view("Ship the probe")))

    // 9. A second account sees only its own list (isolation by lookup).
    var signup = request_with(&mut app, string_view("GET"), string_view("/signup"), string_view(""), string_view(""))
    var signup_body = body_text(&signup)
    var signup_token = csrf_of(&signup)
    var signup_cookie = header_text(&signup, &string_view("Set-Cookie"))
    var grace_anon = cookie_pair(&signup_cookie)
    var weak = form2(string_view("csrf"), signup_token.to_view(),
        string_view("email"), string_view("grace@example.com"))
    var weak2 = string()
    weak2.append_string(&weak)
    weak2.append_view("&password=short")
    var weak_res = request_with(&mut app, string_view("POST"), string_view("/signup"), grace_anon, weak2.to_view())
    check(string_view("a short password is refused"),
        has_text(&header_text(&weak_res, &string_view("Location")), string_view("e=weak")))
    var grace_body = string()
    grace_body.append_string(&weak)
    grace_body.append_view("&password=longenough")
    var grace_res = request_with(&mut app, string_view("POST"), string_view("/signup"), grace_anon, grace_body.to_view())
    var grace_cookie = header_text(&grace_res, &string_view("Set-Cookie"))
    var grace = grace_anon
    if(grace_cookie.size() > 0) { grace = cookie_pair(&grace_cookie) }
    check(string_view("sign-up is a 303 into the app"),
        grace_res.status == 303u && grace.size() > 0)
    var grace_list = request_with(&mut app, string_view("GET"), string_view("/todos"), grace, string_view(""))
    var grace_body_text = body_text(&grace_list)
    check(string_view("a second account has an empty list"),
        !has_text(&grace_body_text, string_view("Read the web stack survey")))
    var grace_token = csrf_of(&grace_list)
    var steal_body = form_text(string_view("csrf"), grace_token.to_view())
    var steal_path = string("/todos/")
    steal_path.append_string(&todo_id)
    steal_path.append_view("/delete")
    var steal_path_view = steal_path.to_view()
    var steal = request_with(&mut app, string_view("POST"), steal_path_view, grace, steal_body.to_view())
    check(string_view("another user's task is a 404"), steal.status == 404u)

    // 10. CSRF is required on every mutation.
    var no_csrf_body = form_text(string_view("title"), string_view("nope"))
    var no_csrf = request_with(&mut app, string_view("POST"), string_view("/todos"), ada, no_csrf_body.to_view())
    check(string_view("a body without a CSRF token is 403"), no_csrf.status == 403u)

    // 11. The JSON API.
    var api = request_with(&mut app, string_view("GET"), string_view("/api/todos"), ada, string_view(""))
    var api_body = body_text(&api)
    var api_type = header_text(&api, &string_view("Content-Type"))
    check(string_view("the JSON API answers the user's tasks"),
        api.status == 200u && has_text(&api_body, string_view("\"title\":\"Ship the probe\"")))
    check(string_view("the JSON API sets application/json"),
        api_type.to_view().equals(&string_view("application/json")))

    // 12. The account page and the frozen-chrome stats route.
    var account = request_with(&mut app, string_view("GET"), string_view("/account"), ada, string_view(""))
    var account_body = body_text(&account)
    check(string_view("the account page shows the email"),
        has_text(&account_body, string_view("ada@example.com")))
    var stats1 = request_with(&mut app, string_view("GET"), string_view("/stats"), ada, string_view(""))
    var stats1_body = body_text(&stats1)
    check(string_view("the shell route fills its slots"),
        has_text(&stats1_body, string_view("Accounts")) && has_text(&stats1_body, string_view("2")))
    var stats2 = request_with(&mut app, string_view("GET"), string_view("/stats"), ada, string_view(""))
    var stats2_body = body_text(&stats2)
    check(string_view("the shell route runs no component work (only fills)"),
        stats2_body.size() > 0u && stats1_body.size() == stats2_body.size())
    check(string_view("the shell was compiled at build"), shell_bytes > 0u)

    // 13. Sign-out destroys the session record.
    var logout_token = csrf_of(&account)
    var logout_body = form_text(string_view("csrf"), logout_token.to_view())
    var logout = request_with(&mut app, string_view("POST"), string_view("/logout"), ada, logout_body.to_view())
    check(string_view("sign-out answers 303"), logout.status == 303u)
    var cleared = request_with(&mut app, string_view("GET"), string_view("/todos"), ada, string_view(""))
    check(string_view("after sign-out the guarded page gates again"),
        has_text(&body_text(&cleared), string_view("not signed in")))

    // 14. Unknown path.
    var missing = request_with(&mut app, string_view("GET"), string_view("/nope"), string_view(""), string_view(""))
    check(string_view("an unknown path answers the 404 route"), missing.status == 404u)

    // 15. Per-request render cost (the number the gaps doc quotes). The clock is
    //     `std::chrono::Instant` — the gaps doc used to claim there was none.
    var started = std::chrono::Instant::now()
    var i = 0
    var renders = 400
    while(i < renders) {
        var r = request_with(&mut app, string_view("GET"), string_view("/todos"), grace, string_view(""))
        i = i + 1
    }
    var elapsed = started.elapsed()
    var micros = elapsed.as_micros()
    unsafe { printf("bench renders=%d micros=%lld per_render_us=%lld\n", renders,
        micros, micros / (renders as i64)) }

    // 16. The artifact set the static half exports (pre-rendered routes only).
    var finished = app.finish()
    unsafe {
        printf("finish=%d artifacts=%zu users=%d todos=%d\n", finished as int,
            artifacts, db_user_count() as int, db_todo_count() as int)
    }
    return 0
}
