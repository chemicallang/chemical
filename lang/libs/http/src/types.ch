// ===== HTTP types =====
public namespace http {

using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;

// ---------------------------------------------------------------------------
// Errors
// ---------------------------------------------------------------------------

public variant HttpError {
    Io(e : std::io::IoError)
    Tls(e : tls::TlsError)
    InvalidUrl()
    Connect(m : string)
    Protocol(m : string)
    HeaderTooLarge()
    BodyTooLarge()
    Timeout()
    Other(m : string)

    public func message(&self) : string_view {
        switch(self) {
            Io(e) => { return e.message() }
            Tls(e) => { return e.message() }
            InvalidUrl() => { return string_view("invalid URL") }
            Connect(m) => { return m.to_view() }
            Protocol(m) => { return m.to_view() }
            HeaderTooLarge() => { return string_view("message head too large") }
            BodyTooLarge() => { return string_view("message body too large") }
            Timeout() => { return string_view("timed out") }
            Other(m) => { return m.to_view() }
            default => { return string_view("") }
        }
    }
}

func http_lit(value : *char) : string { unsafe { return string::make_no_len(value) } }

func http_ctor(data : *char, size : size_t) : string { unsafe { return string::constructor(data, size) } }

func http_make_string(value : &str) : string {
    var out = string()
    out.append_view(value.to_view())
    return out
}

func http_io_error() : std::io::IoError {
    return std::io::IoError.Kind(std::io::IoErrorKind.Other())
}

func header_name_equals(a : string_view, b : string_view) : bool {
    if(a.size() != b.size()) { return false }
    var i : size_t = 0
    while(i < a.size()) {
        var ca = a.at(i)
        var cb = b.at(i)
        if(ca >= 'A' && ca <= 'Z') { ca = (ca + 32) as char }
        if(cb >= 'A' && cb <= 'Z') { cb = (cb + 32) as char }
        if(ca != cb) { return false }
        i += 1
    }
    return true
}

/// Case-sensitive text equality over two views (no allocation, no `&`).
public func text_equals(a : string_view, b : string_view) : bool {
    if(a.size() != b.size()) { return false }
    var i : size_t = 0
    while(i < a.size()) {
        if(a.at(i) != b.at(i)) { return false }
        i += 1
    }
    return true
}

// ---------------------------------------------------------------------------
// Headers
// ---------------------------------------------------------------------------

// A header table. Insertion order is preserved (that is what peers see and what
// makes tests readable), lookup is case-insensitive, and `insert` *replaces* the
// existing value for the same name so a handler can override a default instead
// of emitting a duplicate. `append` is the explicit multi-value form.
public struct HeaderMap {

    internal var headers: vector<std::pair<string, string>>;

    @make
    func make() {
        return HeaderMap {
            headers : vector<std::pair<string, string>>()
        }
    }

    public func empty(&self) : bool {
        return headers.empty()
    }

    public func len(&self) : size_t {
        return headers.size()
    }

    /// Case-insensitive borrowed lookup. The view points into this map and is
    /// valid for as long as the map is not mutated.
    public func get(&self, name : &str) : Option<string_view> {
        var want = name.to_view()
        var i = 0u
        while(i < headers.size()) {
            var p = headers.get_ptr(i)
            if(header_name_equals(unsafe(p.first.to_view()), want)) {
                return Option.Some<string_view>(unsafe(p.second.to_view()))
            }
            i += 1u
        }
        return Option.None<string_view>()
    }

    /// Case-insensitive owned lookup: the returned string outlives the map.
    public func get_owned(&self, name : &str) : Option<string> {
        var got = self.get(name)
        if(got is Option.None) { return Option.None<string>() }
        var Some(v) = got else unsafe { unreachable }
        var out = string()
        out.append_view(&v)
        return Option.Some<string>(out)
    }

    public func contains(&self, name : &str) : bool {
        return self.get(name) is Option.Some
    }

    public func name_at(&self, index : size_t) : string_view {
        return self.headers.at_ref(index).first.to_view()
    }

    public func value_at(&self, index : size_t) : string_view {
        return self.headers.at_ref(index).second.to_view()
    }

    /// Set (insert or replace) an owned name/value pair.
    public func insert(&mut self, k : string, v : string) {
        var want = k.to_view()
        var i = 0u
        while(i < headers.size()) {
            var p = headers.get_ptr(i)
            if(header_name_equals(unsafe(p.first.to_view()), want)) {
                unsafe { p.second = std::replace(&mut v, string()) }
                return
            }
            i += 1u
        }
        headers.push_back(std::pair<string, string>{ first : k, second : v })
    }

    public func insert_view(&mut self, k : &string_view, v : &string_view) {
        self.insert(unsafe(string(k.data(), k.size())), unsafe(string(v.data(), v.size())))
    }

    /// Add another value for an existing name (multi-value headers).
    public func append(&mut self, k : string, v : string) {
        headers.push_back(std::pair<string, string>{ first : k, second : v })
    }

    public func remove(&mut self, name : &str) {
        var want = name.to_view()
        var i = 0u
        while(i < headers.size()) {
            var p = headers.get_ptr(i)
            if(header_name_equals(unsafe(p.first.to_view()), want)) {
                headers.erase(i)
                return
            }
            i += 1u
        }
    }

    public func clear(&mut self) {
        headers.clear()
    }

    public func clone(&self) : HeaderMap {
        var out = HeaderMap()
        var i = 0u
        while(i < headers.size()) {
            var p = headers.get_ptr(i)
            unsafe { out.append(p.first.copy(), p.second.copy()) }
            i += 1u
        }
        return out
    }
}

/// Cursor over a header table, in insertion order.
public struct HeaderIter<'a> {
    internal var map : &'a HeaderMap;
    internal var index : size_t;

    public func valid(&self) : bool {
        return self.index < self.map.headers.size()
    }

    public func name(&self) : string_view {
        var p = self.map.headers.at_ref(self.index)
        return p.first.to_view()
    }

    public func value(&self) : string_view {
        var p = self.map.headers.at_ref(self.index)
        return p.second.to_view()
    }

    public func next(&mut self) {
        self.index += 1u
    }
}

public func (h: &HeaderMap) iter() : HeaderIter<'self> {
    var it : HeaderIter<'self> = HeaderIter<'self> { map : h, index : 0u }
    return it
}

// ---------------------------------------------------------------------------
// Query string
// ---------------------------------------------------------------------------

public struct QueryMap {
    var pairs : vector<std::pair<string, string>>

    @make
    func make() {
        return QueryMap {}
    }

    @make
    func make_from_qs(qs : &string_view) {
        return QueryMap {
            pairs : parse_query(qs)
        }
    }

    public func len(&self) : size_t {
        return pairs.size()
    }

    public func empty(&self) : bool {
        return pairs.empty()
    }

    /// The decoded value of `key`, or `None` when the key is absent. An empty
    /// value (`?a=`) is `Some("")` and is therefore distinguishable from a
    /// missing parameter.
    public func get(&self, key : &str) : Option<string_view> {
        var want = key.to_view()
        var i = 0u
        while(i < pairs.size()) {
            var p = pairs.get_ptr(i)
            unsafe {
                if(text_equals(p.first.to_view(), want)) {
                    return Option.Some<string_view>(p.second.to_view())
                }
            }
            i = i + 1u
        }
        return Option.None<string_view>()
    }

    /// The raw (owned) value of `key`.
    public func get_owned(&self, key : &str) : Option<string> {
        var got = self.get(key)
        if(got is Option.None) { return Option.None<string>() }
        var Some(v) = got else unsafe { unreachable }
        var out = string()
        out.append_view(&v)
        return Option.Some<string>(out)
    }
}

// ---------------------------------------------------------------------------
// Requests / responses
// ---------------------------------------------------------------------------

/// The peer's request. The body has already been framed and read by the
/// framework (bounded by `ServerConfig.max_body_bytes`), so a handler never
/// touches the connection.
public struct Request {
    var method: string
    var path: string
    var query: QueryMap
    var proto: string
    var headers: HeaderMap
    var remote: string
    var body: RequestBody
    var route_params: vector<std::pair<string, string>>

    @constructor func constructor() {
        return Request {
            method = string::empty_str(),
            path = string::empty_str(),
            query = QueryMap(),
            proto = string::empty_str(),
            headers = HeaderMap(),
            remote = string::empty_str(),
            body : RequestBody(),
            route_params : vector<std::pair<string, string>>()
        }
    }

    /// A route parameter (`/:id`), or `None`.
    public func param(&self, name : &str) : Option<string_view> {
        var want = name.to_view()
        var i = 0u
        while(i < route_params.size()) {
            var p = route_params.get_ptr(i)
            unsafe {
                if(text_equals(p.first.to_view(), want)) { return Option.Some<string_view>(p.second.to_view()) }
            }
            i = i + 1u
        }
        return Option.None<string_view>()
    }

    /// A header value, or `None`.
    public func header(&self, name : &str) : Option<string_view> {
        return self.headers.get(name)
    }

    /// A decoded query parameter, or `None`.
    public func query_param(&self, name : &str) : Option<string_view> {
        return self.query.get(name)
    }
}

/// The peer's response. A handler builds one and returns it; the framework owns
/// the connection and encodes it (status line, headers, body, framing).
public struct Response {
    var status: u16
    var status_text: string
    var proto: string
    var headers: HeaderMap
    var body: ResponseBody

    @constructor func constructor() {
        return Response {
            status = 200u,
            status_text = string(),
            proto = string(),
            headers = HeaderMap(),
            body : ResponseBody.Empty()
        }
    }

    public func set_header_view(&mut self, k : &string_view, v : &string_view) {
        self.headers.insert_view(k, v)
    }

    public func set_header(&mut self, k : string, v : string) {
        self.headers.insert(k, v)
    }

    public func set_body(&mut self, body : ResponseBody) {
        self.body = body
    }

    /// `Content-Type: text/plain; charset=utf-8` (unless the handler already
    /// chose a content type).
    public func text(&mut self, v : &string_view) {
        if(!self.headers.contains("Content-Type")) {
            self.set_header_view("Content-Type", "text/plain; charset=utf-8")
        }
        var out = string()
        out.append_view(v)
        self.body = ResponseBody.Text(out)
    }

    /// `Content-Type: text/html; charset=utf-8` (unless already set).
    public func html(&mut self, v : &string_view) {
        if(!self.headers.contains("Content-Type")) {
            self.set_header_view("Content-Type", "text/html; charset=utf-8")
        }
        var out = string()
        out.append_view(v)
        self.body = ResponseBody.Text(out)
    }

    public func bytes(&mut self, v : &[u8]) {
        var out = vector<u8>()
        out.reserve(v.size())
        var i : size_t = 0
        while(i < v.size()) {
            out.push(unsafe(*v.at(i)))
            i += 1
        }
        self.body = ResponseBody.Bytes(out)
    }
}

// ---------------------------------------------------------------------------
// Status text
// ---------------------------------------------------------------------------

public func status_text(code : u16) : string_view {
    switch(code) {
        100 => { return string_view("Continue") }
        101 => { return string_view("Switching Protocols") }
        200 => { return string_view("OK") }
        201 => { return string_view("Created") }
        202 => { return string_view("Accepted") }
        204 => { return string_view("No Content") }
        206 => { return string_view("Partial Content") }
        301 => { return string_view("Moved Permanently") }
        302 => { return string_view("Found") }
        303 => { return string_view("See Other") }
        304 => { return string_view("Not Modified") }
        307 => { return string_view("Temporary Redirect") }
        308 => { return string_view("Permanent Redirect") }
        400 => { return string_view("Bad Request") }
        401 => { return string_view("Unauthorized") }
        403 => { return string_view("Forbidden") }
        404 => { return string_view("Not Found") }
        405 => { return string_view("Method Not Allowed") }
        408 => { return string_view("Request Timeout") }
        409 => { return string_view("Conflict") }
        411 => { return string_view("Length Required") }
        413 => { return string_view("Content Too Large") }
        414 => { return string_view("URI Too Long") }
        415 => { return string_view("Unsupported Media Type") }
        429 => { return string_view("Too Many Requests") }
        431 => { return string_view("Request Header Fields Too Large") }
        500 => { return string_view("Internal Server Error") }
        501 => { return string_view("Not Implemented") }
        502 => { return string_view("Bad Gateway") }
        503 => { return string_view("Service Unavailable") }
        504 => { return string_view("Gateway Timeout") }
        505 => { return string_view("HTTP Version Not Supported") }
        default => { return string_view("") }
    }
}

// ---------------------------------------------------------------------------
// Response builders (keep handler bodies one line)
// ---------------------------------------------------------------------------

public func response_empty(status : u16) : Response {
    var r = Response()
    r.status = status
    return r
}

public func response_text(status : u16, v : &string_view) : Response {
    var r = Response()
    r.status = status
    r.text(v)
    return r
}

public func response_html(status : u16, v : &string_view) : Response {
    var r = Response()
    r.status = status
    r.html(v)
    return r
}

public func response_bytes(status : u16, v : &[u8]) : Response {
    var r = Response()
    r.status = status
    r.bytes(v)
    return r
}

/// A JSON response. The body must already be serialized (`json::stringify`),
/// which keeps `http` independent of the `json` library.
public func response_json_text(status : u16, serialized : &string_view) : Response {
    var r = Response()
    r.status = status
    r.set_header_view("Content-Type", "application/json")
    var out = string()
    out.append_view(serialized)
    r.body = ResponseBody.Text(out)
    return r
}

public func response_not_found() : Response {
    return response_text(404u, "Not Found\n")
}

} // namespace http
