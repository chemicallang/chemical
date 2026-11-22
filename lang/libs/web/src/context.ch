// What a route gets: the request, plus the session the app already resolved.
//
// Before this, a page handler got `(page, req)` and nothing else, so every app that
// wanted "the current session" published it through a top-level pointer (a mutable
// global is `SAFE-GLOBAL-001`, so that is a pointer to a request-local and every
// read is `unsafe`) and reimplemented CSRF lookups in each handler.
//
//     public func Cart(ctx : &mut web::RequestContext, page : &mut HtmlPage) {
//         var items = ctx.session_text("cart").to_view()   // "" when anonymous
//         var token = ctx.csrf_token()                     // starts a session if needed
//         ...
//     }
//
// A session is *loaded* once per request by `App.handle` and only *created* when a
// route asks for one (`ensure_session`, `csrf_token`, `set`), so an anonymous request
// allocates nothing.
public namespace web {

using std::Option;
using std::string;
using std::string_view;

@direct_init
public struct RequestContext {
    /// The request being answered: `ctx.path()`, `ctx.header("…")`, `ctx.param("id")`.
    /// A pointer, not a reference, because the context outlives the route's borrow of
    /// it; it is the caller's request and never null.
    private var request : *mut http::Request
    /// The session this request carries. Invalid until a route asks for one.
    public var session : Session
    /// True when this request started the session, which is when the response has to
    /// carry `Set-Cookie` (done by `App.handle` after the route returns).
    private var created : bool
    /// The app's store. Null during the build-time renders (`build()`/`finish()`
    /// render every pre-rendered route once, with no session and nothing to create).
    private var store : *mut SessionStore
    /// Whatever the app registered with `App.set_state` (its database, its config).
    /// Type-erased here and recovered by `ctx.state<T>()`; null when the app has none.
    private var state_ptr : *mut void
    /// The app's frozen site, so a handler can render a page through `ctx.render_page`
    /// without holding the app. Null outside an app (a bare `StaticSite`).
    private var site : *mut StaticSite

    @make
    func make(request_ : *mut http::Request, store_ : *mut SessionStore,
              state_ : *mut void = null, site_ : *mut StaticSite = null) : RequestContext {
        // Load the session the request carries, once. An unknown or missing id
        // stays invalid: a route decides whether this request deserves a session
        // (`ensure_session`), so an anonymous visit allocates nothing.
        var found : Option<Session> = Option.None<Session>()
        if(store_ != null) {
            var st = unsafe(&mut *store_)
            found = st.load(unsafe(&*request_))
        }
        var session_ = Session { id : string() }
        if(found is Option.Some) {
            var Some(loaded) = found else unsafe { unreachable }
            session_ = loaded
        }
        return RequestContext {
            request : request_,
            session : session_,
            created : false,
            store : store_,
            state_ptr : state_,
            site : site_
        }
    }

    // ── The app's own state ──────────────────────────────────────────────────

    /// The app state registered with `App.set_state`, as the type it was registered
    /// as: `var db = ctx.state<Db>()`. Null when the app registered none.
    ///
    /// This is the answer to "where does a handler keep its database": the app owns
    /// the value (usually a stack local in `main`), hands it over once, and every
    /// route reads it through the context — the same context that carries the
    /// session. Before this, the only public answer was a top-level mutable global
    /// reached through an `unsafe` accessor, because `SAFE-GLOBAL-001` forbids the
    /// direct form.
    public func <T> state(&self) : *mut T {
        return state_ptr as *mut T
    }

    // ── The request ──────────────────────────────────────────────────────────

    /// The raw request, for code whose type is spelled in terms of it
    /// (`SlotValues`, `ShellPredicate`, `web::form`).
    public func req(&self) : *mut http::Request {
        return request
    }

    /// The path this app matched (the raw request path, before the mount prefix is
    /// stripped).
    public func path(&self) : string_view {
        return unsafe(request.path.to_view())
    }

    public func method(&self) : string_view {
        return unsafe(request.method.to_view())
    }

    /// A request header, or `None`.
    public func header(&self, name : &str) : Option<string_view> {
        return unsafe(request.header(name))
    }

    /// A route parameter (`/invoices/{id}` → `ctx.param("id")`), or `None`.
    public func param(&self, name : &str) : Option<string_view> {
        return unsafe(request.param(name))
    }

    /// A query parameter (`?q=…`), or `None`.
    public func query(&self, name : &str) : Option<string_view> {
        return unsafe(request.query_param(name))
    }

    /// The body as text (empty when the request had none).
    public func body_text(&self) : string_view {
        return unsafe(request.body_text())
    }

    /// Whether the body is of this media type (`ctx.is_form_post()` covers the
    /// common case).
    public func is_content_type(&self, want : &str) : bool {
        return unsafe(request.is_content_type(want))
    }

    /// The urlencoded form body; empty for another media type, exactly as
    /// `web::form(req)` behaves.
    public func form(&self) : http::Form {
        return parse_request_form(unsafe(&*request))
    }

    /// True for a `POST` that carries an urlencoded form body — the request a
    /// `<form method="post">` sends, and the one `ctx.form()` can read.
    public func is_form_post(&self) : bool {
        return unsafe(request.is_form_post())
    }

    // ── Rendering a page from an endpoint ─────────────────────────────────────────

    /// Render `handler` for this request and answer with it — the escape hatch an
    /// **endpoint** needs when it should show a page instead of redirecting:
    ///
    /// ```chemical
    /// public func LoginPost(ctx : &mut web::RequestContext) : http::Response {
    ///     if(!password_ok(ctx)) {
    ///         return ctx.render_page(LoginPage, 422u)   // re-render the form
    ///     }
    ///     return web::redirect(303u, "/todos")
    /// }
    /// ```
    ///
    /// Redirect-after-post is still the default worth reaching for (a reload then
    /// does not resubmit); this is for the case it cannot express, a form that has to
    /// come back with the errors in it. The page is rendered against the app's frozen
    /// bundle, exactly like a `live` route, so a universal component hydrates.
    ///
    /// `cache_control` defaults to `no-store`: a response that depended on a
    /// submission must not be cached. Fails with 503 when the bundle lacks a
    /// component the page renders (the same failure a `live` route reports).
    public func render_page(&mut self, handler : PageHandler, status : u16 = 200u,
                            cache_control : string_view = "no-store") : http::Response {
        if(site == null) {
            var reason = string_view("render_page: this context has no site (a bare StaticSite cannot render a live page)")
            return error_response(503u, &reason)
        }
        var st = unsafe(&mut *site)
        var page = HtmlPage()
        if(!st.attach_dynamic(&mut page)) {
            var reason = st.dynamic_error_message()
            return error_response(500u, &reason)
        }
        handler(self, &mut page)
        if(page.used_universal()) { page.defaultUniversalSetup() }
        var html = st.render_dynamic(&mut page)
        if(html.empty()) {
            var reason = st.dynamic_error_message()
            return error_response(503u, &reason)
        }
        var out = st.dynamic_response(&html, cache_control)
        var none = string_view("")
        var res = response_of(&out, none)
        res.status = status
        return res
    }

    // ── The session ──────────────────────────────────────────────────────────

    /// Start a session for this request if it has none. Idempotent. A pre-rendered
    /// build pass has no store, so there it does nothing (`session.valid()` stays
    /// false) — a per-user page must be a `live`/`endpoint` route.
    public func ensure_session(&mut self) {
        if(session.valid()) { return }
        if(store == null) { return }
        var st = unsafe(&mut *store)
        session = st.create()
        created = true
    }

    /// A session value, or `None`. Reads the store, so it never holds a snapshot.
    public func session_value(&self, name : &str) : Option<string_view> {
        if(!session.valid() || store == null) { return Option.None<string_view>() }
        var st = unsafe(&mut *store)
        return st.get(&session, name.to_view())
    }

    /// A session value as an owned string (`""` when unset) — the form that survives
    /// past the next store write.
    public func session_text(&self, name : &str) : string {
        var found = session_value(name)
        var out = string()
        if(found is Option.Some) {
            var Some(value) = found else unsafe { unreachable }
            out.append_view(&value)
        }
        return out
    }

    public func has_session_value(&self, name : &str) : bool {
        return session_value(name) is Option.Some
    }

    /// Write a session value, starting the session if needed.
    public func set_session(&mut self, name : &str, value : string_view) {
        ensure_session()
        if(!session.valid() || store == null) { return }
        var st = unsafe(&mut *store)
        st.set(&session, name.to_view(), value)
    }

    public func remove_session(&mut self, name : &str) {
        if(!session.valid() || store == null) { return }
        var st = unsafe(&mut *store)
        st.remove(&session, name.to_view())
    }

    /// The session's CSRF token, generating it (and the session, on a first visit) as
    /// needed. A page that renders a form calls this; a mutating endpoint compares
    /// the submitted field with `ctx.verify_csrf(...)`.
    public func csrf_token(&mut self) : string_view {
        ensure_session()
        if(!session.valid() || store == null) { return string_view("") }
        var st = unsafe(&mut *store)
        return st.csrf_token(&session)
    }

    /// True when `submitted` is this session's token (constant-time compare).
    public func verify_csrf(&self, submitted : string_view) : bool {
        if(!session.valid() || store == null) { return false }
        var st = unsafe(&mut *store)
        return st.verify_csrf(&session, submitted)
    }

    /// Destroy the session (sign-out): the record is gone and `res` carries the
    /// cookie-clearing header. Nothing else does that for you, which is why this
    /// takes the response.
    public func destroy_session(&mut self, res : &mut http::Response) {
        if(!session.valid() || store == null) { return }
        var st = unsafe(&mut *store)
        st.destroy(res, &session)
        session = Session { id : string() }
        created = false
    }

    // ── Framework side ───────────────────────────────────────────────────────

    /// Attach the session cookie when this request started the session. Called by
    /// `App.handle`; a custom dispatcher that owns its responses calls it too.
    public func finish(&mut self, res : &mut http::Response) {
        if(!created || !session.valid() || store == null) { return }
        var st = unsafe(&mut *store)
        st.attach(res, &session)
    }
}

} // namespace web
