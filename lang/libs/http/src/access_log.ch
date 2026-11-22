// ===== Access log =====
//
// An operator's first debugging tool is one line per request: who asked, for what,
// what came back, how big it was, how long it took. Without it a server is a black
// box — the framework's diagnostics (`status_for_error`, refused framing) explain a
// single connection, but nothing says which request was slow at 3 a.m.
//
// The line is built here and handed to a sink the app owns, so the destination and
// the format stay the app's decision: `AccessLogger.combined()` gives the Common Log
// Format plus a duration, `AccessLogger.make(sink)` takes any callback (a file, a
// metrics counter, a structured JSON line), and `AccessLogger.off()` is the default
// and costs one branch per message.
public namespace http {

using std::string;
using std::string_view;

/// One served message, as an access logger sees it.
///
/// The views borrow the request/response buffers and are valid only for the
/// duration of the call: a sink that stores anything must copy it.
@direct_init
public struct AccessEntry {
    /// `a.b.c.d:port` of the peer. Empty when the driver cannot name it.
    var remote : string_view
    var method : string_view
    /// The request target as it arrived: path plus query.
    var target : string_view
    var status : u16
    /// Bytes of the encoded body written (the gzip length when the body was
    /// compressed, `0` for `HEAD` — nothing was sent).
    var bytes : usize
    /// Wall time from the first read of this message to the last write.
    var duration_us : i64
    /// When the message finished, as Unix seconds.
    var epoch_secs : i64
    var keep_alive : bool
}

/// Where access lines go. A `std.send_sync_function` so the logger can live in a
/// `Server` that moves between threads.
public type AccessSink = std.send_sync_function<(entry : &AccessEntry) => void>

func access_sink_quiet(entry : &AccessEntry) { }

/// `22/Nov/2026:10:07:02 +0000` — the Common Log Format's timestamp (always UTC,
/// so a log line is comparable across servers in different zones).
public func format_access_date(epoch_secs : i64) : string {
    var days = epoch_secs
    if(days < 0) { days = days - 86399 }
    days = days / 86400
    var secs_of_day = epoch_secs - days * 86400

    var year : i64 = 1970
    var month : i64 = 1
    var day : i64 = 1
    http_date_civil_from_days(days, &raw mut year, &raw mut month, &raw mut day)

    var out = string()
    out.reserve(26)
    http_date_two_digits(&mut out, day)
    out.append('/')
    out.append_char_ptr(http_date_month(month))
    out.append('/')
    out.append_integer(year)
    out.append(':')
    http_date_two_digits(&mut out, secs_of_day / 3600)
    out.append(':')
    http_date_two_digits(&mut out, (secs_of_day % 3600) / 60)
    out.append(':')
    http_date_two_digits(&mut out, secs_of_day % 60)
    out.append_view(" +0000")
    return out
}

func clf_hex_digit(value : u8) : char {
    if(value < 10u) { return (48u + value as u32) as char }
    return (87u + value as u32) as char
}

/// Append `value` as one quoted-field body.
///
/// The request target is attacker-controlled: it may carry a `"` (which would close
/// the field and forge the rest of the line), a backslash, or control bytes that a
/// terminal prints as escapes or that split a line-oriented reader. The line is the
/// operator's record of what happened, so a field is escaped rather than copied:
/// `\"`, `\\` and `\xHH` for anything else below 0x20 and for 0x7f.
func append_clf_field(out : &mut string, value : string_view) {
    var i : ubigint = 0
    while(i < value.size()) {
        var c = value.at(i)
        var byte = c as u8
        if(c == '"') {
            out.append_view("\\\"")
        } else if(c == '\\') {
            out.append_view("\\\\")
        } else if(byte < 32u || byte == 127u) {
            out.append_view("\\x")
            out.append(clf_hex_digit((byte >> 4u) & 15u))
            out.append(clf_hex_digit(byte & 15u))
        } else {
            out.append(c)
        }
        i = i + 1
    }
}

/// One line: the Common Log Format followed by the duration in microseconds.
///
/// ```
/// 127.0.0.1:51234 - - [22/Nov/2026:10:07:02 +0000] "GET /pricing HTTP/1.1" 200 2565 38us
/// ```
///
/// `-` stands in for a field the entry does not have (a peer the driver could not
/// name), which is what log parsers expect.
public func format_access_line(entry : &AccessEntry) : string {
    var out = string()
    out.reserve(120)
    if(entry.remote.empty()) {
        out.append('-')
    } else {
        out.append_view(&entry.remote)
    }
    out.append_view(" - - [")
    var stamp = format_access_date(entry.epoch_secs)
    out.append_string(&stamp)
    out.append_view("] \"")
    if(entry.method.empty()) { out.append('-') } else { append_clf_field(&mut out, entry.method) }
    out.append(' ')
    // Both are request data: the method is whatever token arrived and the target is
    // path + query, so neither may be trusted to be a single unquoted field.
    if(entry.target.empty()) { out.append('-') } else { append_clf_field(&mut out, entry.target) }
    out.append_view(" HTTP/1.1\" ")
    out.append_uinteger(entry.status as ubigint)
    out.append(' ')
    out.append_uinteger(entry.bytes as ubigint)
    out.append(' ')
    out.append_integer(entry.duration_us)
    out.append_view("us\n")
    return out
}

/// Print `entry` in the Common Log Format on standard error.
public func access_sink_stderr(entry : &AccessEntry) {
    var line = format_access_line(entry)
    unsafe {
        // One `fwrite` + `fflush` per line: the line is under `PIPE_BUF`, so two
        // threads cannot interleave inside one entry, and a long-running server
        // shows its log immediately.
        var file = get_stderr()
        var _ = fwrite(line.data() as *void, 1 as u64, line.size() as u64, file)
        fflush(file)
    }
}

/// Per-request logging for a `Server` (see `ServerConfig.access_log`).
public struct AccessLogger {
    internal var sink : AccessSink
    internal var on : bool

    /// Log through `sink`. The server calls it on the thread that served the
    /// request, so a sink that writes shared state must do its own locking.
    @make
    public func make(sink : AccessSink) : AccessLogger {
        return AccessLogger { sink : sink, on : true }
    }

    /// Common Log Format lines on standard error.
    @make
    public func combined() : AccessLogger {
        // A lambda, not a bare function reference: a `Send` callback needs its
        // capture witness, which only a lambda literal carries (`SAFE-SEND-002`).
        var sink : AccessSink = ||(entry : &AccessEntry) => { access_sink_stderr(entry) }
        return AccessLogger { sink : sink, on : true }
    }

    /// No logging (the default).
    @make
    public func off() : AccessLogger {
        var sink : AccessSink = ||(entry : &AccessEntry) => { access_sink_quiet(entry) }
        return AccessLogger { sink : sink, on : false }
    }

    public func is_enabled(&self) : bool {
        return self.on
    }

    func emit(&self, entry : &AccessEntry) {
        // Called on the serving thread, so no locking here: the sink owns whatever
        // synchronisation its destination needs.
        self.sink(entry)
    }
}

} // namespace http
