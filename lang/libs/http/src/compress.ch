// ===== Content-Encoding negotiation =====
//
// The artifact sink negotiates its own gzip variants at build time. A rendered
// response has no frozen variant, so this asks the same question of the request's
// `Accept-Encoding`: may the server compress, and is gzip the form to use?
public namespace http {

using std::string_view;

// True when `header[start..end]` is exactly `lit`.
func accept_token_is(header : string_view, start : size_t, end : size_t, lit : string_view) : bool {
    if(end < start) { return false }
    if(end - start != lit.size()) { return false }
    var i : size_t = 0
    while(i < lit.size()) {
        if(header.at(start + i) != lit.at(i)) { return false }
        i = i + 1
    }
    return true
}

// The `q=` value in `header[start..end]` is zero (`q=0`, `q=0.0`, `q=0.00`).
// A missing `q` is not zero: it means 1.
func accept_q_is_zero(header : string_view, start : size_t, end : size_t) : bool {
    var i = start
    var eq : size_t = end
    while(i < end) {
        if(header.at(i) == '=') { eq = i + 1; break }
        i = i + 1
    }
    if(eq >= end) { return false }
    var saw_digit = false
    var non_zero = false
    var k = eq
    while(k < end) {
        var c = header.at(k)
        if(c == '.') {
            k = k + 1
            continue
        }
        if(c < '0' || c > '9') { return false }
        saw_digit = true
        if(c != '0') { non_zero = true }
        k = k + 1
    }
    if(!saw_digit) { return false }
    return !non_zero
}

/// Whether an `Accept-Encoding` header accepts gzip: it names `gzip` with a
/// non-zero quality, or names `*` with a non-zero quality and does not exclude
/// gzip. `gzip;q=0` is an explicit refusal, which outranks a `*` wildcard.
public func accepts_gzip(header : string_view) : bool {
    if(header.size() == 0) { return false }
    var i : size_t = 0
    var wildcard_ok = false
    var gzip_seen = false
    while(i < header.size()) {
        // One comma-separated token, trimmed of surrounding spaces.
        var start = i
        while(i < header.size() && header.at(i) != ',') { i = i + 1 }
        var end = i
        while(start < end && header.at(start) == ' ') { start = start + 1 }
        while(end > start && header.at(end - 1) == ' ') { end = end - 1 }
        // `name` runs to the first `;`, the parameters follow it.
        var name_end = start
        while(name_end < end && header.at(name_end) != ';') { name_end = name_end + 1 }
        var params_start = name_end
        if(params_start < end && header.at(params_start) == ';') { params_start = params_start + 1 }
        var q_is_zero = accept_q_is_zero(header, params_start, end)
        if(accept_token_is(header, start, name_end, string_view("gzip"))) {
            gzip_seen = true
            if(!q_is_zero) { return true }
        } else if(accept_token_is(header, start, name_end, string_view("*"))) {
            if(!q_is_zero) { wildcard_ok = true }
        }
        i = i + 1
    }
    if(gzip_seen) { return false }
    return wildcard_ok
}

} // namespace http
