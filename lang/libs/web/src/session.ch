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
// Two stores, one API:
//
//   * `SessionStore.make()` — process memory. Correct for one process and for tests;
//     a restart forgets every session. `save_to`/`load_from` snapshot it by hand.
//   * `SessionStore.make_durable("sessions/")` — the records live in files under a
//     directory, one per session, written atomically (temp file + rename) on every
//     change. That survives a restart with no save call, and two processes on the
//     same host (or a shared volume) see each other's sessions: `load` re-reads the
//     record from disk on every request, and the memory map is only a cache between
//     a load and the reads that follow it.
//
// Both expire: `set_ttl(seconds)` drops a session whose record has not been written
// for that long (an idle timeout), and `sweep_expired()` deletes the files of the
// ones nobody will come back to, so an abandoned store does not grow forever.
//
// Durability is per session, not a snapshot: a crash loses at most the session being
// written, and a session is small enough that one file per session is cheap. Values
// are read and written through the store rather than copied into the handle, so a
// session never holds a stale snapshot.
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
    /// When this record was last written, as Unix seconds. A durable store uses it
    /// for the idle timeout; an in-memory store keeps it up to date for symmetry.
    public var last_seen : i64
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
    /// Where durable records live. Empty for an in-memory store.
    private var dir : string
    /// Idle timeout in seconds; `0` means a session never expires.
    private var ttl : i64

    @make
    public func make(cookie_name : string_view = "session", secure : bool = false) : SessionStore {
        var name = string()
        name.append_view(&cookie_name)
        return SessionStore {
            records : unordered_map<string, SessionRecord>(),
            cookie_name : name,
            secure : secure,
            dir : string(),
            ttl : 0
        }
    }

    /// A store whose records live as files under `directory` (created if missing).
    ///
    /// Two processes pointed at the same directory share sessions, and a restart does
    /// not lose them, because every change is written through instead of being held
    /// only in memory. The directory must be private to the app: a session record is
    /// user data and its CSRF token, not something another tenant may read.
    @make
    public func make_durable(directory : string_view, cookie_name : string_view = "session", secure : bool = false) : SessionStore {
        var store = SessionStore.make(cookie_name, secure)
        store.dir.append_view(&directory)
        store.ensure_dir()
        return store
    }

    public func set_secure(&mut self, on : bool) {
        secure = on
    }

    /// Expire a session this many seconds after its record was last written. `0`
    /// (the default) never expires, which is why a long-lived deployment wants a
    /// value here: a session id that never expires is a credential that never expires.
    public func set_ttl(&mut self, seconds : i64) {
        ttl = seconds
    }

    /// True when records are written through to `dir`.
    public func is_durable(&self) : bool {
        return !dir.empty()
    }

    /// The directory durable records live in, or `""`.
    public func directory(&self) : string_view {
        return dir.to_view()
    }

    /// The session this request carries, if its id is known to the store. An
    /// unknown or missing id is `None` — never a fresh empty session, so an
    /// anonymous request does not silently allocate server memory.
    public func load(&mut self, req : &http::Request) : Option<Session> {
        var jar = http::CookieJar.of(req)
        var name_view = cookie_name.to_view()
        var id_found = jar.get(name_view)
        if(id_found is Option.None) {
            return Option.None<Session>()
        }
        var Some(id_view) = id_found else unsafe { unreachable }
        // The id is user data that becomes a file name: only the hex ids `create`
        // generates are accepted, so a cookie cannot name `../../etc/passwd`.
        if(!session_id_is_safe(id_view)) {
            return Option.None<Session>()
        }
        if(is_durable()) {
            self.refresh(id_view)
        }
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
            if(is_durable()) { write_record_file(dir.to_view(), session.id.to_view(), rec) }
        }
    }

    public func remove(&mut self, session : &Session, name : string_view) {
        var rec = record_ptr(&records, session.id.to_view())
        if(rec == null) { return }
        unsafe {
            map_remove(&mut rec.values, name)
            if(is_durable()) { write_record_file(dir.to_view(), session.id.to_view(), rec) }
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
                if(is_durable()) { write_record_file(dir.to_view(), session.id.to_view(), rec) }
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
            if(is_durable()) { write_record_file(dir.to_view(), session.id.to_view(), rec) }
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
        unsafe {
            if(is_durable()) { write_record_file(dir.to_view(), id_view, rec) }
        }
        var opts = http::CookieOptions.make("/", "Lax", true, secure, 0)
        var name = cookie_name.to_view()
        res.cookie(name, id_view, opts)
    }

    /// Replace `session`'s id, carrying its values and CSRF token over, and forget
    /// the old id (memory record and durable file alike).
    ///
    /// Call this after a privilege change — a successful sign-in, a password or
    /// role change — so an id that was fixed on the client *before* that change is
    /// worthless afterwards. Without it an attacker who can plant a cookie (a
    /// subdomain, a shared kiosk, a link with `?session=`) reads every request of
    /// the session it chose: session fixation. Rotating also expires any copy of
    /// the old id that leaked, because the record behind it is gone.
    public func rotate(&mut self, session : &mut Session) {
        var old_id = string()
        old_id.append_view(&session.id.to_view())
        var fresh_id = random_hex(24u)
        var rec = record_ptr(&records, old_id.to_view())
        if(rec != null) {
            var carried = SessionRecord {
                values : unordered_map<string, string>(),
                csrf : string(),
                last_seen : 0
            }
            unsafe {
                carried.last_seen = rec.last_seen
                carried.csrf.append_view(&rec.csrf.to_view())
                var it = rec.values.iterator()
                while(it.valid()) {
                    var k = it.key() as *string
                    var v = it.value() as *string
                    map_set(&mut carried.values, k.to_view(), v.to_view())
                    it.next()
                }
            }
            var key = string()
            key.append_view(&fresh_id.to_view())
            records.insert(key, carried)
        }
        if(!old_id.empty()) {
            records.erase(&old_id)
            if(is_durable()) { remove_record_file(dir.to_view(), old_id.to_view()) }
        }
        session.id = fresh_id
        if(is_durable()) {
            var stored = record_mut(&mut records, session.id.to_view())
            if(stored != null) {
                unsafe { write_record_file(dir.to_view(), session.id.to_view(), stored) }
            }
        }
    }

    /// End the session: drop the server record and expire the cookie. A client that
    /// keeps the old id gets `None` from `load`, because the record is gone.
    public func destroy(&mut self, res : &mut http::Response, session : &Session) {
        var id = string()
        id.append_view(&session.id.to_view())
        if(!id.empty()) {
            records.erase(&id)
            if(is_durable()) {
                remove_record_file(dir.to_view(), id.to_view())
            }
        }
        var opts = http::CookieOptions.make("/", "Lax", true, secure, 0)
        var name = cookie_name.to_view()
        res.delete_cookie(name, opts)
    }

    public func count(&self) : ubigint {
        return records.size()
    }

    // ── Durable store ────────────────────────────────────────────────────────

    func ensure_dir(&mut self) {
        if(dir.empty()) { return }
        var view = dir.to_view()
        var p = path::Path.new(std::str.make_view(view))
        var _ = fs::create_dir_all(&p)
    }

    /// Re-read `id` from disk before it is used, so this request sees what another
    /// process wrote and the idle timer slides on a session that is still in use.
    ///
    /// A record that is missing, unreadable or expired drops the cached copy: a
    /// session another process ended must not come back to life here.
    func refresh(&mut self, id : string_view) {
        var fresh = read_record_file(dir.to_view(), id)
        var key = string()
        key.append_view(&id)
        if(fresh is Option.None) {
            records.erase(&key)
            return
        }
        var Some(rec) = fresh else unsafe { unreachable }
        if(record_is_expired(ttl, rec.last_seen)) {
            remove_record_file(dir.to_view(), id)
            records.erase(&key)
            return
        }
        var last_seen = rec.last_seen
        records.erase(&key)
        var inserted = string()
        inserted.append_view(&id)
        records.insert(inserted, rec)
        if(now_secs() - last_seen >= 60) {
            // Slide the idle timer at most once a minute: a request that only reads
            // should not rewrite the file.
            var stored = record_ptr(&records, id)
            if(stored != null) {
                unsafe { write_record_file(dir.to_view(), id, stored) }
            }
        }
    }

    /// Delete the records whose idle timeout has passed. Returns how many files were
    /// removed.
    ///
    /// Expiry is checked on every load, so this is housekeeping, not correctness: call
    /// it from a timer (once a minute is plenty) so abandoned sessions stop taking
    /// disk space. A process that restarts often may never need it.
    public func sweep_expired(&mut self) : usize {
        if(!is_durable() || ttl <= 0) { return 0 }
        var view = dir.to_view()
        var p = path::Path.new(std::str.make_view(view))
        var listed = fs::read_dir(&p)
        if(listed is std::Result.Err) { return 0 }
        var Ok(entries) = listed else unsafe { unreachable }
        var removed : usize = 0
        unsafe {
            var i : usize = 0
            while(i < entries.size()) {
                var entry = entries.get_ptr(i)
                var is_dir = entry.is_dir()
                if(!is_dir) {
                    var name = entry.file_name()
                    var removed_id = record_id_from_file_name(name.to_view())
                    if(removed_id is Option.Some) {
                        var Some(id) = removed_id else unsafe { unreachable }
                        var stored = read_record_file(dir.to_view(), id.to_view())
                        if(stored is Option.Some) {
                            var Some(rec) = stored else unsafe { unreachable }
                            if(record_is_expired(ttl, rec.last_seen)) {
                                remove_record_file(dir.to_view(), id.to_view())
                                var key = string()
                                key.append_view(&id.to_view())
                                records.erase(&key)
                                removed = removed + 1
                            }
                        }
                    }
                }
                i = i + 1
            }
        }
        return removed
    }

    // ── Durability ───────────────────────────────────────────────────────────
    /// Write the whole store to `file` so sessions survive a restart.
    ///
    /// The file is a snapshot, not a journal: one line per session,
    /// `<id> <csrf> <key>=<value>…`, every field hex-encoded. Hex is deliberate —
    /// session values are user data, and an escaping scheme is a parser bug
    /// waiting to happen; hex has no reserved characters.
    public func save_to(&self, file : string_view) : bool {
        var out = string()
        var it = records.iterator()
        while(it.valid()) {
            unsafe {
                var id = it.key() as *string
                var id_view = id.to_view()
                out.append_view(&id_view)
                var rec = it.value() as *SessionRecord
                out.append(' ')
                var csrf_view = rec.csrf.to_view()
                append_hex(&mut out, csrf_view)
                var vit = rec.values.iterator()
                while(vit.valid()) {
                    var key = vit.key() as *string
                    var key_view = key.to_view()
                    var value = vit.value() as *string
                    var value_view = value.to_view()
                    out.append(' ')
                    append_hex(&mut out, key_view)
                    out.append('=')
                    append_hex(&mut out, value_view)
                    vit.next()
                }
            }
            out.append('\n')
            it.next()
        }
        var p = path::Path.new(std::str.make_view(file))
        var written = fs::write(&p, out.as_bytes())
        return written is std::Result.Ok
    }

    /// Replace this store with the snapshot at `file`. A missing or unreadable
    /// file is an empty store (a first run), never an error; a malformed line is
    /// skipped, so a truncated snapshot cannot take the process down.
    public func load_from(&mut self, file : string_view) : bool {
        var p = path::Path.new(std::str.make_view(file))
        var read = fs::read_to_string(&p)
        if(read is std::Result.Err) { return false }
        var Ok(snapshot) = read else unsafe { unreachable }
        self.records.clear()
        var text_view = snapshot.to_view()
        var i : size_t = 0
        while(i < text_view.size()) {
            var line_end = i
            while(line_end < text_view.size() && text_view.at(line_end) != '\n') {
                line_end = line_end + 1
            }
            if(line_end > i) { self.restore_line(text_view, i, line_end) }
            i = line_end + 1
        }
        return true
    }

    /// Restore one snapshot line, through the same paths a live request uses, so a
    /// restored session behaves exactly like one that was never gone.
    func restore_line(&mut self, text : std::string_view, start : size_t, end : size_t) {
        if(!line_is_valid(text, start, end)) { return }
        var i = start
        var id = read_token(text, i, end)
        var Some(id_field) = id else unsafe { unreachable }
        var csrf = decode_hex_field(text, after_space(text, id_field.next, end), end)
        var Some(csrf_field) = csrf else unsafe { unreachable }
        var id_view = id_field.text.to_view()
        self.seed_csrf(id_view, csrf_field.text.to_view())
        var j = csrf_field.next
        while(j < end) {
            if(text.at(j) == ' ') { j = j + 1 }
            if(j >= end) { break }
            var key = decode_hex_field(text, j, end)
            var Some(key_field) = key else unsafe { unreachable }
            var value = decode_hex_field(text, key_field.next + 1, end)
            var Some(value_field) = value else unsafe { unreachable }
            self.seed_value(id_view, key_field.text.to_view(), value_field.text.to_view())
            j = value_field.next
        }
    }

    /// Set a session's CSRF token without generating one. Only the snapshot loader
    /// uses this: a live server must never accept a client-chosen token.
    func seed_csrf(&mut self, id : string_view, token : string_view) {
        var rec = record_mut(&mut self.records, id)
        if(rec == null) { return }
        unsafe { rec.csrf = copy_of(token) }
    }

    /// One restored value.
    func seed_value(&mut self, id : string_view, name : string_view, value : string_view) {
        var rec = record_mut(&mut self.records, id)
        if(rec == null) { return }
        unsafe { map_set(&mut rec.values, name, value) }
    }
}

// ── Snapshot parsing ────────────────────────────────────────────────────────
// ── Durable record files ───────────────────────────────────────────────────────
// One file per session, named `<id>.sess`, written to a temp file and renamed into
// place so a reader never sees half a record. The format is:
//
//     <last_seen_epoch> <csrf_hex>
//     <key_hex>=<value_hex>
//     …
//
// Hex for the same reason the snapshot uses it: session values are user data and an
// escaping scheme is a parser bug waiting to happen. A record is small, so hex is
// cheap here.

func now_secs() : i64 {
    var now = std::chrono::SystemTime::now()
    return now.as_unix_epoch_secs()
}

func record_is_expired(ttl : i64, last_seen : i64) : bool {
    if(ttl <= 0) { return false }
    return now_secs() - last_seen > ttl
}

// A session id becomes a file name, so it is validated as one: only the hex `create`
// produces is accepted, which also makes path traversal impossible.
func session_id_is_safe(id : std::string_view) : bool {
    if(id.size() == 0 || id.size() > 64u) { return false }
    var i : size_t = 0
    while(i < id.size()) {
        if(hex_value(id.at(i)) < 0) { return false }
        i = i + 1
    }
    return true
}

func record_file_text(dir : std::string_view, id : std::string_view) : string {
    var out = string()
    out.append_view(&dir)
    out.append('/')
    out.append_view(&id)
    out.append_view(".sess")
    return out
}

// The id a record file name carries, or `None` for any other file in the directory.
func record_id_from_file_name(name : std::string_view) : Option<string> {
    var suffix = std::string_view(".sess")
    if(name.size() <= suffix.size()) { return Option.None<string>() }
    var tail = name.subview(name.size() - suffix.size(), name.size())
    if(!tail.equals(&suffix)) { return Option.None<string>() }
    var id = name.subview(0, name.size() - suffix.size())
    if(!session_id_is_safe(id)) { return Option.None<string>() }
    return Option.Some<string>(copy_of(id))
}

func write_record_file(dir : std::string_view, id : std::string_view, rec : *mut SessionRecord) : bool {
    unsafe {
        rec.last_seen = now_secs()
    }
    var text = string()
    serialize_record(rec, &mut text)
    var file = record_file_text(dir, id)
    // A per-write temp name, so two processes saving the same session at the same
    // moment do not truncate each other's half-written temp file.
    var tmp = string()
    tmp.append_view(&file.to_view())
    tmp.append('.')
    var tag = random_hex(4u)
    tmp.append_view(&tag.to_view())
    tmp.append_view(".tmp")
    var tmp_view = tmp.to_view()
    var tmp_path = path::Path.new(std::str.make_view(tmp_view))
    var wrote = fs::write(&tmp_path, text.as_bytes())
    if(wrote is std::Result.Err) { return false }
    var file_view = file.to_view()
    var target = path::Path.new(std::str.make_view(file_view))
    var moved = fs::move_path(&tmp_path, &target)
    return moved is std::Result.Ok
}

func read_record_file(dir : std::string_view, id : std::string_view) : Option<SessionRecord> {
    var file = record_file_text(dir, id)
    var file_view = file.to_view()
    var p = path::Path.new(std::str.make_view(file_view))
    var read = fs::read_to_string(&p)
    if(read is std::Result.Err) { return Option.None<SessionRecord>() }
    var Ok(contents) = read else unsafe { unreachable }
    var rec = SessionRecord {
        values : unordered_map<string, string>(),
        csrf : string(),
        last_seen : 0
    }
    var ptr = &raw mut rec
    if(!parse_record_file(contents.to_view(), ptr)) {
        return Option.None<SessionRecord>()
    }
    return Option.Some<SessionRecord>(rec)
}

func remove_record_file(dir : std::string_view, id : std::string_view) {
    var file = record_file_text(dir, id)
    var file_view = file.to_view()
    var p = path::Path.new(std::str.make_view(file_view))
    var _ = fs::remove_file(&p)
}

func serialize_record(rec : *mut SessionRecord, out : &mut string) {
    unsafe {
        out.append_integer(rec.last_seen)
        out.append(' ')
        append_hex(out, rec.csrf.to_view())
        out.append('\n')
        var it = rec.values.iterator()
        while(it.valid()) {
            var key = it.key() as *string
            var value = it.value() as *string
            append_hex(out, key.to_view())
            out.append('=')
            append_hex(out, value.to_view())
            out.append('\n')
            it.next()
        }
    }
}

// Fill `out` from one record file. All or nothing: a line that does not parse rejects
// the whole record, so a damaged file cannot resurrect a session with half its values
// and a CSRF token that no longer matches.
func parse_record_file(text : std::string_view, out : *mut SessionRecord) : bool {
    var first_end = index_of_newline(text, 0)
    var space = index_of_char(text, 0, first_end, ' ')
    if(space >= first_end) { return false }
    var epoch = parse_epoch(text, 0, space)
    if(epoch < 0) { return false }
    var csrf = decode_hex_field(text, space + 1, first_end)
    if(csrf is Option.None) { return false }
    var Some(csrf_field) = csrf else unsafe { unreachable }
    unsafe {
        out.last_seen = epoch
        // A copy, not `csrf_field.text`: `csrf_field` is a payload bound by
        // `var Some(csrf_field) = csrf`, so its `text` aliases the variant's payload.
        // Moving the field out clears this binding's drop flag while `csrf` still
        // owns the string, and both free it (see destructure_move.ch).
        out.csrf = copy_of(csrf_field.text.to_view())
    }
    var i = first_end
    if(i < text.size()) { i = i + 1 }
    while(i < text.size()) {
        var line_end = index_of_newline(text, i)
        if(line_end > i) {
            var eq = index_of_char(text, i, line_end, '=')
            if(eq >= line_end) { return false }
            var key = decode_hex_field(text, i, eq)
            var value = decode_hex_field(text, eq + 1, line_end)
            if(key is Option.None || value is Option.None) { return false }
            var Some(key_field) = key else unsafe { unreachable }
            var Some(value_field) = value else unsafe { unreachable }
            unsafe {
                map_set(&mut out.values, key_field.text.to_view(), value_field.text.to_view())
            }
        }
        i = line_end
        if(i < text.size()) { i = i + 1 }
    }
    return true
}

func index_of_newline(text : std::string_view, from : size_t) : size_t {
    return index_of_char(text, from, text.size(), '\n')
}

func index_of_char(text : std::string_view, from : size_t, end : size_t, needle : char) : size_t {
    var i = from
    while(i < end) {
        if(text.at(i) == needle) { return i }
        i = i + 1
    }
    return end
}

// A non-negative decimal epoch, or -1.
func parse_epoch(text : std::string_view, start : size_t, end : size_t) : i64 {
    if(start >= end) { return -1 }
    var value : i64 = 0
    var i = start
    while(i < end) {
        var c = text.at(i)
        if(c < '0' || c > '9') { return -1 }
        value = value * 10 + (c as i64 - 48)
        i = i + 1
    }
    return value
}

// A decoded field plus where the next one starts.
@direct_init
struct HexField {
    var text : string
    var next : size_t
}

// `next` after a field points at the separator space; the next field starts
// after it.
func after_space(text : std::string_view, next : size_t, end : size_t) : size_t {
    if(next < end && text.at(next) == ' ') { return next + 1 }
    return next
}

// The raw (not hex) token at `start`: the session id is written as its own text,
// because an id is opaque already and hex-encoding it only makes the file harder
// to read.
func read_token(text : std::string_view, start : size_t, end : size_t) : Option<HexField> {
    var out = string()
    var i = start
    while(i < end) {
        var c = text.at(i)
        if(c == ' ' || c == '=') { break }
        unsafe { out.append(c) }
        i = i + 1
    }
    if(out.size() == 0) { return Option.None<HexField>() }
    return Option.Some<HexField>(HexField { text : out, next : i })
}

// The hex field at `start`, stopping at a space, `=` or the line end.
func decode_hex_field(text : std::string_view, start : size_t, end : size_t) : Option<HexField> {
    var out = string()
    var digits : size_t = 0
    var i = start
    while(i < end) {
        var c = text.at(i)
        if(c == ' ' || c == '=') { break }
        var nibble = hex_value(c)
        if(nibble < 0) { return Option.None<HexField>() }
        var byte : u8 = nibble as u8
        if(digits % 2 == 0) {
            var high : u8 = byte * 16u
            unsafe { out.append(high as char) }
        } else {
            var last = out.size() - 1
            var low : u8 = unsafe(out.at(last) as u8)
            unsafe { out.set(last, (low + byte) as char) }
        }
        digits = digits + 1
        i = i + 1
    }
    if(digits % 2 != 0) { return Option.None<HexField>() }
    return Option.Some<HexField>(HexField { text : out, next : i })
}

func hex_value(c : char) : int {
    if(c >= '0' && c <= '9') { return (c as int) - 48 }
    if(c >= 'a' && c <= 'f') { return (c as int) - 87 }
    if(c >= 'A' && c <= 'F') { return (c as int) - 55 }
    return -1
}

func append_hex(out : &mut string, value : std::string_view) {
    var i : size_t = 0
    while(i < value.size()) {
        var b = value.at(i) as u8
        out.append(hex_digit(b >> 4))
        out.append(hex_digit(b & 0x0f))
        i = i + 1
    }
}

// Is `<id> <csrf> <key>=<value>…` well formed between `start` and `end`? Checked
// before anything is stored, so a truncated line leaves no session behind.
func line_is_valid(text : std::string_view, start : size_t, end : size_t) : bool {
    var i = start
    var id = read_token(text, i, end)
    if(id is Option.None) { return false }
    var Some(id_field) = id else unsafe { unreachable }
    // A line that stops right after the id has lost its CSRF field: that is a
    // truncated write, not a session with an empty token (which still has the
    // separator space before the first value).
    if(id_field.next >= end) { return false }
    var csrf = decode_hex_field(text, after_space(text, id_field.next, end), end)
    if(csrf is Option.None) { return false }
    var Some(csrf_field) = csrf else unsafe { unreachable }
    var j = csrf_field.next
    while(j < end) {
        if(text.at(j) == ' ') { j = j + 1 }
        if(j >= end) { break }
        var key = decode_hex_field(text, j, end)
        if(key is Option.None) { return false }
        var Some(key_field) = key else unsafe { unreachable }
        if(key_field.next >= end || text.at(key_field.next) != '=') { return false }
        var value = decode_hex_field(text, key_field.next + 1, end)
        if(value is Option.None) { return false }
        var Some(value_field) = value else unsafe { unreachable }
        j = value_field.next
    }
    return true
}

// A fresh owned copy of a view. Returned by value, so the store's field owns the
// bytes it holds.
func copy_of(view : string_view) : string {
    var s = string()
    s.append_view(&view)
    return s
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
        csrf : string(),
        last_seen : now_secs()
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
