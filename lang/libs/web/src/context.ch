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

    @make
    func make(request_ : *mut http::Request, store_ : *mut SessionStore) : RequestContext {
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
            store : store_
        }
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
