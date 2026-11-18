// Cookies: the jar a request carries and the `Set-Cookie` a response sends.
//
// A cookie is a name/value pair with attributes; the browser stores it and sends it
// back on every matching request. Chemical has no global cookie state: a handler
// parses the request's jar, reads what it needs, and writes `Set-Cookie` headers on
// the response it returns. That is enough for sessions (`web::Session`, which builds
// on this) without the framework owning per-process state.
//
//     var jar = http.CookieJar.of(&req)
//     if(jar.get("theme") is Option.Some) { … }
//
//     res.cookie("theme", "dark", http.CookieOptions.make(secure: true))
//
// `set_cookie` is the plain public name (safe by construction: it serialises
// validated names and values); a raw header form is not needed and is not published.
public namespace http {

using std::Option;
using std::string;
using std::string_view;
using std::vector;

/// One cookie the client sent.
@direct_init
public struct Cookie {
    public var name : string
    public var value : string
}

/// The cookies of one request.
@direct_init
public struct CookieJar {
    public var cookies : vector<Cookie>

    /// Parse a `Cookie:` header (`a=1; b=2`). Empty segments and pairs without a
    /// name are skipped rather than rejected: a malformed cookie from another
    /// origin must not fail the request.
    public func parse(&mut self, header : string_view) {
        var i : ubigint = 0
        while(i < header.size()) {
            // One `name=value` pair, up to the next `;`.
            var name = string()
            var value = string()
            var in_value = false
            while(i < header.size()) {
                var c = header.at(i)
                if(c == ';') { break }
                if(c == '=' && !in_value) {
                    in_value = true
                } else if(c == ' ') {
                    // A space inside a pair is not significant around `=`; inside
                    // the value it is part of the value.
                    if(in_value && value.size() > 0) { value.append(c) }
                    if(!in_value && name.size() > 0) { /* trailing space */ }
                } else if(in_value) {
                    value.append(c)
                } else {
                    name.append(c)
                }
                i = i + 1
            }
            if(name.size() > 0) {
                cookies.push_back(Cookie { name : name, value : value })
            }
            // Skip the `;` and any spaces before the next pair.
            while(i < header.size()) {
                var c2 = header.at(i)
                if(c2 == ';' || c2 == ' ') { i = i + 1 } else { break }
            }
        }
    }

    public func get(&self, name : string_view) : Option<string_view> {
        var i : ubigint = 0
        while(i < cookies.size()) {
            var c = unsafe(&*cookies.get_ptr(i))
            var n = c.name.to_view()
            if(text_equals(n, name)) {
                return Option.Some<string_view>(c.value.to_view())
            }
            i = i + 1
        }
        return Option.None<string_view>()
    }

    public func has(&self, name : string_view) : bool {
        var found = get(name)
        if(found is Option.Some) { return true }
        return false
    }

    public func len(&self) : ubigint {
        return cookies.size()
    }

    /// The jar of a request: every `Cookie` header it carries, in order.
    public func of(req : &Request) : CookieJar {
        var jar = CookieJar { cookies : vector<Cookie>() }
        var i : size_t = 0
        while(i < req.headers.len()) {
            var name = req.headers.name_at(i)
            if(text_equals(name, "Cookie")) {
                var value = req.headers.value_at(i)
                jar.parse(value)
            }
            i = i + 1
        }
        return jar
    }
}

/// Attributes of a cookie the response sets. The defaults are the safe pair for a
/// session cookie: `HttpOnly` (no JavaScript access), `SameSite=Lax` (not sent on
/// cross-site POSTs), `Path=/`, no `Secure` (so `http://localhost` works — pass
/// `secure: true` in production), and no `Max-Age` (the cookie dies with the tab).
public struct CookieOptions {
    public var path : string
    public var domain : string
    /// `"Lax"`, `"Strict"`, `"None"` (case-insensitive). `"None"` requires
    /// `secure: true` for browsers to accept the cookie at all.
    public var same_site : string
    public var http_only : bool
    public var secure : bool
    /// Seconds until the cookie expires; 0 means "until the browser closes".
    public var max_age : i64

    @make
    public func make(path : string_view = "/", same_site : string_view = "Lax",
                     http_only : bool = true, secure : bool = false,
                     max_age : i64 = 0, domain : string_view = "") : CookieOptions {
        var p = string()
        p.append_view(&path)
        var ss = string()
        ss.append_view(&same_site)
        var d = string()
        d.append_view(&domain)
        return CookieOptions {
            path : p,
            domain : d,
            same_site : ss,
            http_only : http_only,
            secure : secure,
            max_age : max_age
        }
    }

    /// The same attributes with different values (a copy, not a mutation, so a
    /// shared default is safe).
    public func with(&self, path : string_view = "", same_site : string_view = "",
                     http_only_off : bool = false, secure_on : bool = false,
                     max_age : i64 = -1) : CookieOptions {
        var out = CookieOptions.make(
            self.path.to_view(),
            self.same_site.to_view(),
            self.http_only,
            self.secure,
            self.max_age,
            self.domain.to_view())
        if(!path.empty()) {
            out.path.clear()
            out.path.append_view(&path)
        }
        if(!same_site.empty()) {
            out.same_site.clear()
            out.same_site.append_view(&same_site)
        }
        if(http_only_off) { out.http_only = false }
        if(secure_on) { out.secure = true }
        if(max_age >= 0) { out.max_age = max_age }
        return out
    }
}

/// Serialise one `Set-Cookie` header value. A name or value containing a control
/// character, `;`, `,`, or a space is not serialisable and yields an empty string
/// (the caller's `cookie()` then skips the header instead of emitting a broken one).
public func set_cookie_value(name : string_view, value : string_view, opts : &CookieOptions) : string {
    var out = string()
    if(!cookie_token_ok(name) || !cookie_value_ok(value)) {
        return out
    }
    out.append_view(&name)
    out.append('=')
    out.append_view(&value)

    var path = opts.path.to_view()
    if(!path.empty()) {
        out.append_view("; Path=")
        out.append_view(&path)
    }
    var domain = opts.domain.to_view()
    if(!domain.empty()) {
        out.append_view("; Domain=")
        out.append_view(&domain)
    }
    if(opts.max_age > 0) {
        out.append_view("; Max-Age=")
        append_i64(&mut out, opts.max_age)
    } else if(opts.max_age < 0) {
        // A negative Max-Age is "delete now" (RFC 6265 §4.1.2.2).
        out.append_view("; Max-Age=0")
    }
    var same_site = opts.same_site.to_view()
    if(!same_site.empty()) {
        out.append_view("; SameSite=")
        out.append_view(&same_site)
    }
    if(opts.secure) {
        out.append_view("; Secure")
    }
    if(opts.http_only) {
        out.append_view("; HttpOnly")
    }
    return out
}

// RFC 6265 `token`: no separators, no control characters, no whitespace.
func cookie_token_ok(s : string_view) : bool {
    if(s.empty()) { return false }
    var i : ubigint = 0
    while(i < s.size()) {
        var c = s.at(i)
        if(c <= ' ' || c == ';' || c == ',' || c == '=' || c == '"' || c == '\\') { return false }
        i = i + 1
    }
    return true
}

// Cookie values may not contain `;`, `,`, `"`, `\`, or whitespace (the safe subset
// of RFC 6265's cookie-octet). Anything else must be encoded by the caller.
func cookie_value_ok(s : string_view) : bool {
    var i : ubigint = 0
    while(i < s.size()) {
        var c = s.at(i)
        if(c < ' ' || c == ';' || c == ',' || c == '"' || c == '\\') { return false }
        i = i + 1
    }
    return true
}

func append_i64(out : &mut string, v : i64) {
    var n = v
    if(n < 0) {
        out.append('-')
        n = -n
    }
    if(n == 0) {
        out.append('0')
        return
    }
    var digits = string()
    while(n > 0) {
        var d = (n % 10) as u8
        digits.append((d + 48u) as char)
        n = n / 10
    }
    var i = digits.size()
    while(i > 0) {
        i = i - 1
        out.append(digits.at(i))
    }
}

} // namespace http
