// Probe 4: the pages of an authenticated app.
//
// The gap this app is built around: a `PageHandler` returns `void` into a 200 page.
// A page that has to answer "you are not signed in, go to /login" or "this todo is
// not yours, 404" cannot — only an endpoint can decide a status, and an endpoint
// cannot render a page with the site's bundle. So the protected area renders a
// sign-in prompt with status 200, and a mutation re-renders errors by redirecting
// to a query flag. Both are recorded in `lang/docs/web-stack-gaps.md`.
using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;

// ── Request/session helpers ───────────────────────────────────────────────
// `ctx.query` reads a parsed request; a request a test builds by hand has only a
// path, so this fallback decodes `?key=value` from it. An app would not write this
// if `http::Request.make(method, path, …)` existed (gap P1-11).
func query_text(ctx : &web::RequestContext, name : string_view) : string {
    var got = ctx.query(name)
    if(got is Option.Some) {
        var Some(v) = got else unsafe { unreachable }
        return string_of(v)
    }
    var path = ctx.path()
    var qmark = path.find(&string_view("?"))
    if(qmark is Option.None) { return string() }
    var Some(pos) = qmark else unsafe { unreachable }
    var query = path.subview(pos + 1, path.size())
    var pairs = query.split('&')
    var i : ubigint = 0
    while(i < pairs.size()) {
        var pair = unsafe(&*(pairs.get_ptr(i)))
        var parts = pair.split('=')
        if(parts.size() >= 2) {
            var key = unsafe(&*parts.get_ptr(0))
            if(key.to_view().equals(&name)) {
                var value = unsafe(&*parts.get_ptr(1))
                var value_view = value.to_view()
                return http::url_decode(&value_view)
            }
        }
        i = i + 1
    }
    return string()
}

func signed_in(ctx : &web::RequestContext) : bool {
    return ctx.has_session_value("uid")
}

func current_uid(ctx : &web::RequestContext) : bigint {
    var raw = ctx.session_text("uid")
    if(raw.empty()) { return 0 }
    var parsed = raw.to_view().to_int()
    if(parsed is Result.Ok) {
        var Ok(n) = parsed else unsafe { unreachable }
        return n as bigint
    }
    return 0
}

/// Read the one-shot flash message and clear it. There is no flash support, so an
/// app carries a session key and every page has to remember to take it.
func take_flash(ctx : &mut web::RequestContext) : string {
    var f = ctx.session_text("flash")
    if(!f.empty()) { ctx.remove_session("flash") }
    return f
}

func with_flash(ctx : &mut web::RequestContext, res : http::Response, to : string_view,
                 message : string_view) : http::Response {
    ctx.set_session("flash", message)
    return web::redirect(303u, to)
}

// ── Public pages ─────────────────────────────────────────────────────────
public func Landing(ctx : &mut web::RequestContext, page : &mut HtmlPage) {
    page.appendTitle("Tasks — a small shared list")
    #html {
        <main class="landing">
            <h1>Tiny tasks, one per line</h1>
            <p class="lede">A probe app: sign up, keep a list, and let the stack do the rest.</p>
            <p class="cta"><a href="/signup">Create an account</a> or <a href="/login">sign in</a>.</p>
        </main>
    }
}

func auth_form(page : &mut HtmlPage, action : string_view, submit : string_view,
               token : string_view, error : string_view) {
    var token_copy = escaped_token(token)
    var token_view = token_copy.to_view()
    var error_copy = escaped_token(error)
    var error_view = error_copy.to_view()
    #html {
        <form method="post" action={action} class="auth">
            <input type="hidden" name="csrf" value={token_view} />
            <p class="error">{error_view}</p>
            <label>Email <input type="email" name="email" required /></label>
            <label>Password <input type="password" name="password" required /></label>
            <button type="submit">{submit}</button>
        </form>
    }
}

/// `#html` does not escape interpolated values, so every user-visible string goes
/// through the escaper first. This helper is the workaround the P0-5 gap asks the
/// stack to remove.
func escaped_token(value : string_view) : string {
    return escape_html_view(value)
}

func auth_error(code : string_view) : string {
    if(code.equals(&string_view("bad"))) { return string("That email and password do not match.") }
    if(code.equals(&string_view("weak"))) { return string("Use a password of at least 8 characters.") }
    if(code.equals(&string_view("taken"))) { return string("That email already has an account.") }
    if(code.equals(&string_view("email"))) { return string("Enter an email address.") }
    return string()
}

public func LoginPage(ctx : &mut web::RequestContext, page : &mut HtmlPage) {
    var code = query_text(ctx, string_view("e"))
    var message = auth_error(code.to_view())
    var token = ctx.csrf_token()
    page.appendTitle("Tasks — sign in")
    #html {
        <main class="auth-page">
            <h1>Sign in</h1>
        </main>
    }
    auth_form(page, string_view("/login"), string_view("Sign in"), token, message.to_view())
    #html {
        <p class="alt">No account? <a href="/signup">Create one</a>.</p>
    }
}

public func SignupPage(ctx : &mut web::RequestContext, page : &mut HtmlPage) {
    var code = query_text(ctx, string_view("e"))
    var message = auth_error(code.to_view())
    var token = ctx.csrf_token()
    page.appendTitle("Tasks — create an account")
    #html {
        <main class="auth-page">
            <h1>Create an account</h1>
        </main>
    }
    auth_form(page, string_view("/signup"), string_view("Sign up"), token, message.to_view())
    #html {
        <p class="alt">Already have one? <a href="/login">Sign in</a>.</p>
    }
}

// ── The protected area ───────────────────────────────────────────────────
// A page handler cannot redirect, so an anonymous visitor gets this page with
// status 200 instead of a 303 to /login. The user-visible cost is a wasted render
// and a wrong status; the API cost is that every protected route repeats this.
public func TodosPage(ctx : &mut web::RequestContext, page : &mut HtmlPage) {
    page.appendTitle("Tasks — your list")
    if(!signed_in(ctx)) {
        #html {
            <main class="gate">
                <h1>Your list</h1>
                <p>You are not signed in. <a href="/login">Sign in</a> to see your tasks.</p>
            </main>
        }
        return
    }
    // Counted so the driver can separate "rendered per request" from "frozen".
    var handled = db_note_handled()
    var uid = current_uid(ctx)
    var status_text = query_text(ctx, string_view("status"))
    var show_done = status_text.to_view().equals(&string_view("done"))
    var ids = db_todos_of(uid, show_done)
    var flash = take_flash(ctx)
    var flash_view = flash.to_view()
    var token = ctx.csrf_token()
    var token_copy = string()
    token_copy.append_view(&token)
    var token_view = token_copy.to_view()
    var open_count = db_todos_of(uid, false).size()
    var counts_text = string()
    counts_text.append_integer(open_count as bigint)
    counts_text.append_view(" open of ")
    counts_text.append_integer(handled)
    counts_text.append_view(" rendered")
    var counts_view = counts_text.to_view()
    var index : ubigint = 0
    #html {
        <main class="todos">
            <header class="bar">
                <h1>Your list</h1>
                <p class="flash">{flash_view}</p>
                <p class="counts">{counts_view}</p>
                <a href="/account">Account</a>
                <a href="/stats">Site stats</a>
            </header>
            <form method="post" action="/todos" class="new">
                <input type="hidden" name="csrf" value={token_view} />
                <input type="text" name="title" placeholder="What needs doing?" required />
                <button type="submit">Add</button>
            </form>
            <nav class="filters">
                <a href="/todos" class={filter_class(show_done, false)}>Open</a>
                <a href="/todos?status=done" class={filter_class(show_done, true)}>Done</a>
            </nav>
            <ul class="list">
                @{
                    while(index < ids.size()) {
                        var todo_id = unsafe(*ids.get_ptr(index))
                        var title = db_todo_title(todo_id)
                        var done = db_todo_done(todo_id)
                        var id_text = string()
                        id_text.append_integer(todo_id as bigint)
                        var id_view = id_text.to_view()
                        var slug = string()
                        slug.append_view("/todos/")
                        slug.append_view(&id_view)
                        var slug_view = slug.to_view()
                        var title_copy = escape_html_view(title)
                        var title_view = title_copy.to_view()
                        var state = item_class(done)
                        #html {
                            <li class={state}>
                                <span class="title">{title_view}</span>
                                <form method="post" action={slug_view}>
                                    <input type="hidden" name="csrf" value={token_view} />
                                    <button type="submit">{toggle_label(done)}</button>
                                </form>
                                <form method="post" action={delete_action(slug_view)}>
                                    <input type="hidden" name="csrf" value={token_view} />
                                    <button type="submit">Delete</button>
                                </form>
                            </li>
                        }
                        index = index + 1
                    }
                }
            </ul>
            <p class="empty">{empty_note(ids.size())}</p>
        </main>
    }
}

func filter_class(show_done : bool, this_tab : bool) : string_view {
    if(show_done == this_tab) { return string_view("active") }
    return string_view("")
}

func item_class(done : bool) : string_view {
    if(done) { return string_view("item done") }
    return string_view("item")
}

func toggle_label(done : bool) : string_view {
    if(done) { return string_view("Reopen") }
    return string_view("Done")
}

func empty_note(count : ubigint) : string {
    if(count == 0u) { return string("Nothing here yet.") }
    return string()
}

// `/todos/7` + `/delete` → `/todos/7/delete`.
func delete_action(base : string_view) : string {
    var out = string()
    out.append_view(&base)
    out.append_view("/delete")
    return out
}

public func AccountPage(ctx : &mut web::RequestContext, page : &mut HtmlPage) {
    page.appendTitle("Tasks — account")
    var uid = current_uid(ctx)
    var email = db_user_email(uid)
    var email_copy = escape_html_view(email)
    var email_view = email_copy.to_view()
    var token = ctx.csrf_token()
    var token_copy = string()
    token_copy.append_view(&token)
    var token_view = token_copy.to_view()
    if(!signed_in(ctx)) {
        #html {
            <main class="gate"><h1>Account</h1><p><a href="/login">Sign in</a> first.</p></main>
        }
        return
    }
    var todo_count = string()
    todo_count.append_integer(db_todos_of(uid, true).size() as bigint)
    var count_view = todo_count.to_view()
    #html {
        <main class="account">
            <h1>Account</h1>
            <dl>
                <dt>Email</dt><dd>{email_view}</dd>
                <dt>Tasks</dt><dd>{count_view}</dd>
            </dl>
            <form method="post" action="/logout">
                <input type="hidden" name="csrf" value={token_view} />
                <button type="submit">Sign out</button>
            </form>
        </main>
    }
}

// A frozen-chrome route: the markup below is produced once at build, and each
// request only splices the three slot values. `StatsFills` gets the request but no
// session — a shell route whose chrome depends on the user is not expressible
// (gap: `SlotValues = (req) => SlotFills`).
public func StatsPage(ctx : &mut web::RequestContext, page : &mut HtmlPage) {
    page.appendTitle("Tasks — site stats")
    #html {
        <main class="stats">
            <h1>Site stats</h1>
            <dl>
                <dt>Accounts</dt><dd>@{page.slot("users")}</dd>
                <dt>Tasks</dt><dd>@{page.slot("todos")}</dd>
                <dt>Requests handled</dt><dd>@{page.slot("handled")}</dd>
            </dl>
            <p><a href="/todos">Back to your list</a></p>
        </main>
    }
}

public func StatsFills(req : &http::Request) : web::SlotFills {
    var fills = web::SlotFills()
    var users = string()
    users.append_integer(db_user_count())
    var todos = string()
    todos.append_integer(db_todo_count())
    var handled = string()
    handled.append_integer(db_note_handled())
    fills.set_text("users", users.to_view())
    fills.set_text("todos", todos.to_view())
    fills.set_text("handled", handled.to_view())
    return fills
}

public func Missing(ctx : &mut web::RequestContext, page : &mut HtmlPage) {
    page.appendTitle("Tasks — not found")
    #html {
        <main class="missing">
            <h1>Not found</h1>
            <p>That page does not exist. <a href="/">Go home</a>.</p>
        </main>
    }
}

func string_of(v : string_view) : string {
    var out = string()
    out.append_view(&v)
    return out
}
