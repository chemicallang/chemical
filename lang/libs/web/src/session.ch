// Sessions: a server-side record keyed by an opaque cookie value.
//
// A session is not data in the cookie — the cookie carries only a random id, and the
// values live in a store on the server. That keeps the cookie small and lets the
// server end a session unilaterally (`destroy`), which a value-in-cookie scheme
// cannot do.
//
//     var store = web::SessionStore.make()
//     var session = store.create()
//     var existing = store.load(&req)
//     if(existing is Option.Some) {
//         var Some(s) = existing else unsafe { unreachable }
//         session = s
//     }
//     store.set(&session, "user", "ada")
//     store.attach(&mut res, &session)          // Set-Cookie: session=<id>
//
// CSRF: `store.csrf_token(&session)` is a random token bound to the session. A
// state-changing endpoint compares the submitted field with
// `store.verify_csrf(&session, token)`; a cross-site form cannot read the token, so
// it cannot produce a valid one.
//
// The store here is in-memory (one per process): correct for a single process and
// for tests, and `load`/`create`/`get`/`set`/`remove`/`destroy` are what a persistent
// store has to implement. Values are read and written through the store rather than
// copied into the handle, so a session never holds a stale snapshot.
public namespace web {

using std::Option;
using std::string;
using std::string_view;
using std::unordered_map;

/// The cookie name a session id travels in.
public func session_cookie_name() : string_view {
    return string_view("session")
}

/// The hidden form field a CSRF token is submitted in.
public func csrf_field_name() : string_view {
    return string_view("_csrf")
}

/// A session handle: its id plus the CSRF token that was looked up with it. Values
/// live in the store and are read through it.
@direct_init
public struct Session {
    public var id : string

    public func valid(&self) : bool {
        return !id.empty()
    }
}

/// A stored session record.
@direct_init
public struct SessionRecord {
    public var values : unordered_map<string, string>
    public var csrf : string
}

/// The session store: cookie parsing, ids, and the server-side records.
@direct_init
public struct SessionStore {
    private var records : unordered_map<string, SessionRecord>
    /// Cookie name used by `load`/`attach`/`destroy`.
    private var cookie_name : string
    /// `Secure` on the session cookie. Keep `false` for `http://localhost`; set it
    /// in production.
    private var secure : bool

    @make
    public func make(cookie_name : string_view = "session", secure : bool = false) : SessionStore {
        var name = string()
        name.append_view(&cookie_name)
        return SessionStore {
            records : unordered_map<string, SessionRecord>(),
            cookie_name : name,
            secure : secure
        }
    }

    public func set_secure(&mut self, on : bool) {
        secure = on
    }

    /// The session this request carries, if its id is known to the store. An
    /// unknown or missing id is `None` — never a fresh empty session, so an
    /// anonymous request does not silently allocate server memory.
    public func load(&self, req : &http::Request) : Option<Session> {
        var jar = http::CookieJar.of(req)
        var name_view = cookie_name.to_view()
        var id_found = jar.get(name_view)
        if(id_found is Option.None) {
            return Option.None<Session>()
        }
        var Some(id_view) = id_found else unsafe { unreachable }
        var rec = record_ptr(&records, id_view)
        if(rec == null) {
            return Option.None<Session>()
        }
        var id = string()
        id.append_view(&id_view)
        return Option.Some<Session>(Session { id : id })
    }

    /// A new session with a fresh random id. Its record exists from the first
    /// `set`, `csrf_token` or `attach`.
    public func create(&self) : Session {
        return Session { id : random_hex(24u) }
    }

    public func get(&self, session : &Session, name : string_view) : Option<string_view> {
        var rec = record_ptr(&records, session.id.to_view())
        if(rec == null) {
            return Option.None<string_view>()
        }
        var out = Option.None<string_view>()
        unsafe {
            out = map_get(&rec.values, name)
        }
        return out
    }

    public func has(&self, session : &Session, name : string_view) : bool {
        var found = get(session, name)
        if(found is Option.Some) { return true }
        return false
    }

    public func set(&mut self, session : &Session, name : string_view, value : string_view) {
        var rec = record_mut(&mut records, session.id.to_view())
        if(rec == null) { return }
        unsafe {
            map_set(&mut rec.values, name, value)
        }
    }

    public func remove(&mut self, session : &Session, name : string_view) {
        var rec = record_ptr(&records, session.id.to_view())
        if(rec == null) { return }
        unsafe {
            map_remove(&mut rec.values, name)
        }
    }

    /// The session's CSRF token, generated on first use and stable afterwards (so
    /// several forms on one page carry the same token).
    public func csrf_token(&mut self, session : &Session) : string_view {
        var rec = record_mut(&mut records, session.id.to_view())
        if(rec == null) {
            return string_view("")
        }
        unsafe {
            if(rec.csrf.empty()) {
                rec.csrf = random_hex(24u)
            }
            return rec.csrf.to_view()
        }
    }

    /// True when `submitted` is this session's CSRF token. Compared in constant time
    /// so a wrong token leaks nothing about how much of it was right.
    public func verify_csrf(&self, session : &Session, submitted : string_view) : bool {
        var rec = record_ptr(&records, session.id.to_view())
        if(rec == null) { return false }
        var token = string_view("")
        unsafe { token = rec.csrf.to_view() }
        if(token.empty() || submitted.size() != token.size()) { return false }
        var diff : u8 = 0
        var i : ubigint = 0
        while(i < token.size()) {
            var a = token.at(i) as u8
            var b = submitted.at(i) as u8
            diff = diff | (a ^ b)
            i = i + 1
        }
        return diff == 0u
    }

    /// Replace the token (call after a privilege change — sign-in, password change —
    /// so a form rendered before it cannot be replayed).
    public func rotate_csrf(&mut self, session : &Session) {
        var rec = record_mut(&mut records, session.id.to_view())
        if(rec == null) { return }
        unsafe {
            rec.csrf = random_hex(24u)
        }
    }

    /// Send the session cookie. The id is the only thing the client holds; the values
    /// stay on the server. Creates the record if this is a new session, so an
    /// attached-but-empty session still loads back.
    public func attach(&mut self, res : &mut http::Response, session : &Session) {
        var id_view = session.id.to_view()
        if(id_view.empty()) { return }
        var rec = record_mut(&mut records, id_view)
        if(rec == null) { return }
        var opts = http::CookieOptions.make("/", "Lax", true, secure, 0)
        var name = cookie_name.to_view()
        res.cookie(name, id_view, opts)
    }

    /// End the session: drop the server record and expire the cookie. A client that
    /// keeps the old id gets `None` from `load`, because the record is gone.
    public func destroy(&mut self, res : &mut http::Response, session : &Session) {
        var id = string()
        id.append_view(&session.id.to_view())
        if(!id.empty()) {
            records.erase(&id)
        }
        var opts = http::CookieOptions.make("/", "Lax", true, secure, 0)
        var name = cookie_name.to_view()
        res.delete_cookie(name, opts)
    }

    public func count(&self) : ubigint {
        return records.size()
    }
}

// ── Store internals ─────────────────────────────────────────────────────────
func record_ptr(m : &unordered_map<string, SessionRecord>, id : string_view) : *mut SessionRecord {
    var k = string()
    k.append_view(&id)
    return m.get_ptr(&k)
}

// The record for `id`, created (with an empty CSRF token) when absent.
func record_mut(m : &mut unordered_map<string, SessionRecord>, id : string_view) : *mut SessionRecord {
    var k = string()
    k.append_view(&id)
    var existing = m.get_ptr(&k)
    if(existing != null) { return existing }
    var rec = SessionRecord {
        values : unordered_map<string, string>(),
        csrf : string()
    }
    m.insert(k, rec)
    var key = string()
    key.append_view(&id)
    return m.get_ptr(&key)
}

// A view of the stored value, borrowed from the map (not from the local key, which
// would dangle).
func map_get(m : &unordered_map<string, string>, name : string_view) : Option<string_view> {
    var k = string()
    k.append_view(&name)
    var p = m.get_ptr(&k)
    if(p == null) {
        return Option.None<string_view>()
    }
    var view = string_view("")
    unsafe { view = p.to_view() }
    return Option.Some<string_view>(view)
}

func map_set(m : &mut unordered_map<string, string>, name : string_view, value : string_view) {
    var k = string()
    k.append_view(&name)
    var v = string()
    v.append_view(&value)
    m.insert(k, v)
}

func map_remove(m : &mut unordered_map<string, string>, name : string_view) {
    var k = string()
    k.append_view(&name)
    m.erase(&k)
}

// `bytes` cryptographically random bytes as lowercase hex. An OS failure would leave
// the session id predictable, so it aborts instead: a server that cannot generate an
// id must not hand out a guessable one.
func random_hex(bytes : ubigint) : string {
    var out = string()
    var result = osrand::random_bytes(bytes as size_t)
    if(result is std::Result.Err) {
        unsafe { unreachable }
    }
    var Ok(buf) = result else unsafe { unreachable }
    var i : ubigint = 0
    while(i < buf.size()) {
        var b = buf.at(i)
        out.append(hex_digit((b >> 4) & 0x0f))
        out.append(hex_digit(b & 0x0f))
        i = i + 1
    }
    return out
}

func hex_digit(v : u8) : char {
    if(v < 10u) {
        return (v + 48u) as char
    }
    return (v + 87u) as char
}

} // namespace web
