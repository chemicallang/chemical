// ===== multipart/form-data =====
//
// The encoding a `<form enctype="multipart/form-data">` produces, and the only way
// browsers upload a file. `application/x-www-form-urlencoded` (see `form.ch`) cannot
// carry binary: a file input turns the whole body into `%XX` text, three times the
// size and lossy for anything that is not valid UTF-8. So a handler that accepts a
// file needs this parser:
//
//   var form = req.multipart()
//   if(form is Result.Err) { return Result.Err(HttpError.BadRequest) }
//   var Ok(fields) = form else unsafe { unreachable }
//   var title = fields.field("title")              // a text input
//   var upload = fields.file("avatar")             // *mut MultipartPart, null if absent
//   if(upload != null) { save(upload.filename, upload.data) }
//
// A part is kept whole in memory: the server has already bounded the request with
// `ServerConfig.max_body_bytes`, so a part cannot be larger than that, and
// `max_parts` bounds how many of them a single body may split into. Streaming a part
// to disk as it arrives is a later addition — the API does not change for it, since
// a handler only ever sees `MultipartPart`.
//
// Deliberately not supported: `Content-Transfer-Encoding` (removed in RFC 7578 §4.7
// and never produced by a browser) and `filename*=UTF-8''…` (RFC 5987 extended
// parameters). A part that carries only the extended form gets `filename = None`, so
// a handler that must have a filename reports the upload as missing rather than
// silently saving it under a garbled name.
public namespace http {

using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;

/// Why a `multipart/form-data` body could not be parsed.
public variant MultipartError {
    /// No `Content-Type: multipart/form-data; boundary=…` on the request.
    MissingBoundary()
    /// The content type names a boundary that cannot be used (empty, or too long).
    InvalidBoundary()
    /// A part had no headers, no `name`, or no terminating boundary.
    MalformedPart()
    /// More parts than the caller allowed — a body trying to exhaust memory with
    /// thousands of tiny parts.
    TooManyParts(limit : usize)

    public func message(&self) : string {
        switch(self) {
            MissingBoundary() => return string("MultipartError: no boundary in Content-Type")
            InvalidBoundary() => return string("MultipartError: boundary is empty or too long")
            MalformedPart() => return string("MultipartError: malformed part")
            TooManyParts(limit) => {
                var s = string("MultipartError: more than ")
                s.append_integer(limit as int)
                s.append_view(" parts")
                return s
            }
        }
    }
}

/// One part of a multipart body: a text field or an uploaded file (`is_file`).
@direct_init
public struct MultipartPart {
    /// The form field name (`name="…"`), which is what the `<input>` was called.
    public var name : string
    /// The client's filename, when the part came from a file input. A browser sends
    /// `filename=""` for an empty file input, so an empty filename still means "a
    /// file part" — check `size()` if an empty upload must be refused.
    public var filename : Option<string>
    /// The part's own `Content-Type`, when it declared one.
    public var content_type : Option<string>
    /// The part body, exactly as sent (binary preserved).
    public var data : vector<u8>

    public func is_file(&self) : bool {
        return filename is Option.Some
    }

    public func size(&self) : usize {
        return data.size()
    }

    /// The part body as text. No UTF-8 validation and no copy: a text field is
    /// already the bytes the browser sent. Do not use it on a binary upload.
    public func text(&self) : string_view {
        if(data.size() == 0u) { return string_view("") }
        return unsafe(string_view(data.data() as *char, data.size()))
    }

    /// The filename, or `""` when the part is a plain text field.
    public func filename_or_empty(&self) : string_view {
        if(filename is Option.Some) {
            var Some(f) = filename else unsafe { unreachable }
            return f.to_view()
        }
        return string_view("")
    }

    /// The declared `Content-Type`, or `""`. A borrowed view: the part owns the
    /// `string`, and a part reached through a pointer cannot give that string away.
    public func content_type_or_empty(&self) : string_view {
        if(content_type is Option.Some) {
            var Some(ct) = content_type else unsafe { unreachable }
            return ct.to_view()
        }
        return string_view("")
    }
}

/// A parsed `multipart/form-data` body, in submission order.
@direct_init
public struct MultipartForm {
    private var parts : vector<MultipartPart>

    @constructor
    func constructor() {
        return MultipartForm {
            parts : vector<MultipartPart>()
        }
    }

    public func size(&self) : usize {
        return parts.size()
    }

    public func is_empty(&self) : bool {
        return parts.size() == 0u
    }

    public func part_at(&self, index : usize) : *mut MultipartPart {
        return parts.get_ptr(index)
    }

    /// The first value of a text field (a part without a filename), or `None`.
    public func field(&self, name : &str) : Option<string_view> {
        var want = name.to_view()
        unsafe {
            var i : usize = 0
            while(i < parts.size()) {
                var part = parts.get_ptr(i)
                var is_upload = part.is_file()
                if(!is_upload) {
                    var part_name = part.name.to_view()
                    if(part_name.equals(&want)) {
                        return Option.Some<string_view>(part.text())
                    }
                }
                i = i + 1
            }
        }
        return Option.None<string_view>()
    }

    /// The first part with field name `name`, whatever it is, or `null`.
    public func part(&self, name : &str) : *mut MultipartPart {
        var want = name.to_view()
        unsafe {
            var i : usize = 0
            while(i < parts.size()) {
                var part = parts.get_ptr(i)
                var part_name = part.name.to_view()
                if(part_name.equals(&want)) {
                    return part
                }
                i = i + 1
            }
        }
        return null
    }

    /// The first uploaded file with field name `name`, or `null`. A pointer into the
    /// form, so it stays valid while the form lives and may be mutated in place.
    public func file(&self, name : &str) : *mut MultipartPart {
        var found = part(name)
        if(found == null) { return null }
        var filename_present = unsafe(found.is_file())
        if(!filename_present) { return null }
        return found
    }

    /// How many parts came from a file input.
    public func file_count(&self) : usize {
        var n : usize = 0
        unsafe {
            var i : usize = 0
            while(i < parts.size()) {
                var part = parts.get_ptr(i)
                if(part.is_file()) { n = n + 1 }
                i = i + 1
            }
        }
        return n
    }

    /// The text fields as an ordinary `Form`, so a handler written for urlencoded
    /// input (`form.get("q")`) accepts a multipart one unchanged. Files are left out.
    public func fields(&self) : Form {
        var out = Form()
        unsafe {
            var i : usize = 0
            while(i < parts.size()) {
                var part = parts.get_ptr(i)
                var is_upload = part.is_file()
                if(!is_upload) {
                    out.push(part.name.to_view(), part.text())
                }
                i = i + 1
            }
        }
        return out
    }

    func push(&mut self, part : MultipartPart) {
        parts.push_back(part)
    }
}

/// Parse a `multipart/form-data` body. `content_type` is the request's full
/// `Content-Type` value (the boundary lives in its parameters).
///
/// Fails rather than guessing: an unparsable part never yields a partial form, so a
/// handler cannot save half of an upload and report success.
public func parse_multipart(body : &[u8], content_type : string_view, max_parts : usize = 1000) : Result<MultipartForm, MultipartError> {
    var boundary_found = multipart_boundary(content_type)
    if(boundary_found is Option.None) {
        return Result.Err(MultipartError.MissingBoundary())
    }
    var Some(boundary) = boundary_found else unsafe { unreachable }
    if(boundary.empty() || boundary.size() > 70u) {
        // RFC 2046 §5.1.1: a boundary is at most 70 characters. A longer one cannot
        // have come from a conforming client, and unbounded length would let a
        // request make the parser allocate arbitrarily.
        return Result.Err(MultipartError.InvalidBoundary())
    }

    var delimiter = string()
    delimiter.append_view("--")
    delimiter.append_string(&boundary)
    var delimiter_bytes = delimiter.as_bytes()
    var closing = string_view("--").as_bytes()
    var crlf = string_view("\r\n").as_bytes()
    var crlf_crlf = string_view("\r\n\r\n").as_bytes()
    // `CRLF--boundary` ends a part; built once, since every part scans for it.
    var part_end = mp_concat(crlf, delimiter_bytes)

    var form = MultipartForm()
    var first = mp_find(body, 0u, delimiter_bytes)
    if(first is Option.None) {
        return Result.Err(MultipartError.MissingBoundary())
    }
    var Some(found_at) = first else unsafe { unreachable }
    // A boundary at position 0 is the opening one; a client that sent a preamble
    // before it would have it skipped, which RFC 2046 allows.
    var cursor = found_at + delimiter_bytes.size()

    while(true) {
        // `--` immediately after the delimiter is the closing boundary.
        if(mp_starts_at(body, cursor, closing)) {
            return Result.Ok(form)
        }
        if(form.size() >= max_parts) {
            return Result.Err(MultipartError.TooManyParts(max_parts))
        }
        // Transport padding (spaces/tabs) may precede the CRLF; skip both.
        cursor = mp_skip_to_line_end(body, cursor)
        var header_end = mp_find(body, cursor, crlf_crlf)
        if(header_end is Option.None) {
            return Result.Err(MultipartError.MalformedPart())
        }
        var Some(headers_end) = header_end else unsafe { unreachable }
        var headers = mp_to_string(body, cursor, headers_end)
        var head = mp_parse_headers(headers)
        if(head.name.empty()) {
            return Result.Err(MultipartError.MalformedPart())
        }
        var data_start = headers_end + crlf_crlf.size()
        var next = mp_find(body, data_start, part_end.as_span())
        if(next is Option.None) {
            return Result.Err(MultipartError.MalformedPart())
        }
        var Some(data_end) = next else unsafe { unreachable }
        var part = MultipartPart {
            name : head.name,
            filename : head.filename,
            content_type : head.content_type,
            data : vector<u8>()
        }
        mp_copy_range(body, data_start, data_end, &mut part.data)
        form.push(part)
        cursor = data_end + crlf.size() + delimiter_bytes.size()
    }
    // The loop only leaves through a `return`; this keeps the return path explicit
    // for a reader (and for a checker that does not prove `while(true)` diverges).
    return Result.Ok(form)
}

/// The boundary named in a `multipart/form-data` content type, if any.
public func multipart_boundary(content_type : string_view) : Option<string> {
    return mp_param(content_type, string_view("boundary"))
}

/// A `multipart/form-data` body from a request, using its own `Content-Type`.
public func (req : &Request) multipart(max_parts : usize = 1000) : Result<MultipartForm, MultipartError> {
    var ct = req.headers.get("Content-Type")
    if(ct is Option.None) {
        return Result.Err(MultipartError.MissingBoundary())
    }
    var Some(value) = ct else unsafe { unreachable }
    return parse_multipart(req.body_view(), value, max_parts)
}

// ── Byte scanning ───────────────────────────────────────────────────────────
// A file upload is binary, so the body is walked as bytes: decoding it to text
// first would corrupt a payload that is not UTF-8.

func mp_starts_at(data : &[u8], at : usize, needle : &[u8]) : bool {
    if(at + needle.size() > data.size()) { return false }
    var i : usize = 0
    while(i < needle.size()) {
        var have = unsafe(*data.at(at + i))
        var want = unsafe(*needle.at(i))
        if(have != want) { return false }
        i = i + 1
    }
    return true
}

func mp_find(data : &[u8], from : usize, needle : &[u8]) : Option<usize> {
    var i = from
    var limit = data.size()
    while(i + needle.size() <= limit) {
        if(mp_starts_at(data, i, needle)) { return Option.Some<usize>(i) }
        i = i + 1
    }
    return Option.None<usize>()
}

// The index after the CRLF at `at` (or after the transport padding before it).
// A body that does not have a CRLF there is malformed, but this is called only
// right after a delimiter, so it returns `at` unchanged and the header search
// reports the failure.
func mp_skip_to_line_end(data : &[u8], at : usize) : usize {
    var i = at
    while(i < data.size()) {
        var c = unsafe(*data.at(i))
        if(c == ' ' || c == '\t' || c == '\r') {
            i = i + 1
            continue
        }
        if(c == '\n') { return i + 1 }
        return at
    }
    return at
}

func mp_to_string(data : &[u8], start : usize, end : usize) : string {
    var out = string()
    if(end <= start) { return out }
    out.reserve(end - start)
    var i = start
    while(i < end) {
        unsafe { out.append(unsafe(*data.at(i)) as char) }
        i = i + 1
    }
    return out
}

func mp_copy_range(data : &[u8], start : usize, end : usize, out : &mut vector<u8>) {
    if(end <= start) { return }
    out.reserve(end - start)
    var i = start
    while(i < end) {
        out.push_back(unsafe(*data.at(i)))
        i = i + 1
    }
}

// `a` followed by `b`, for matching `CRLF--boundary` in one scan.
func mp_concat(a : &[u8], b : &[u8]) : vector<u8> {
    var out = vector<u8>()
    out.reserve(a.size() + b.size())
    var i : usize = 0
    while(i < a.size()) {
        out.push_back(unsafe(*a.at(i)))
        i = i + 1
    }
    var j : usize = 0
    while(j < b.size()) {
        out.push_back(unsafe(*b.at(j)))
        j = j + 1
    }
    return out
}

// ── Part headers ────────────────────────────────────────────────────────────

@direct_init
struct PartHeaders {
    var name : string
    var filename : Option<string>
    var content_type : Option<string>
}

// `Content-Disposition: form-data; name="title"; filename="a b.png"` and
// `Content-Type: image/png`, in either order and in any case.
func mp_parse_headers(block : string) : PartHeaders {
    var out = PartHeaders {
        name : string(),
        filename : Option.None<string>(),
        content_type : Option.None<string>()
    }
    var disposition = string()
    var declared_type = string()
    var view = block.to_view()
    var lines = unsafe(view.split_views('\n'))
    var i : usize = 0
    while(i < lines.size()) {
        var line = unsafe(*lines.get_ptr(i))
        var end = line.size()
        while(end > 0u && line.at(end - 1u) == '\r') { end = end - 1u }
        line = line.subview(0u, end)
        var colon = line.find(string_view(":"))
        if(colon is Option.Some) {
            var Some(at) = colon else unsafe { unreachable }
            var key = line.subview(0u, at).trim()
            var value = line.subview(at + 1u, line.size()).trim()
            if(mp_iequals(key, string_view("content-disposition"))) {
                disposition.append_view(&value)
            } else if(mp_iequals(key, string_view("content-type"))) {
                declared_type.append_view(&value)
            }
        }
        i = i + 1
    }

    var disp = disposition.to_view()
    if(!disp.empty()) {
        var name = mp_param(disp, string_view("name"))
        if(name is Option.Some) {
            var Some(n) = name else unsafe { unreachable }
            out.name = n
        }
        var filename = mp_param(disp, string_view("filename"))
        if(filename is Option.Some) {
            var Some(f) = filename else unsafe { unreachable }
            out.filename = Option.Some<string>(f)
        }
    }
    var typed = declared_type.to_view()
    if(!typed.empty()) {
        var copied = string()
        copied.append_view(&typed)
        out.content_type = Option.Some<string>(copied)
    }
    return out
}

func mp_iequals(a : string_view, b : string_view) : bool {
    if(a.size() != b.size()) { return false }
    var i : usize = 0
    while(i < a.size()) {
        if(mp_lower(a.at(i)) != mp_lower(b.at(i))) { return false }
        i = i + 1
    }
    return true
}

func mp_lower(c : char) : char {
    if(c >= 'A' && c <= 'Z') { return (c as int + 32) as char }
    return c
}

// The value of parameter `key` in a header value such as
// `form-data; name="title"; filename=photo.png`.
//
// The value is unquoted and `\"` inside quotes is unescaped, because a filename is
// user data and a client is free to quote it either way. Parameters after the first
// match win, which is what a browser's ordering gives for `filename` (it comes last).
func mp_param(value : string_view, key : string_view) : Option<string> {
    var segments = unsafe(value.split_views(';'))
    var i : usize = 0
    // The first segment is the disposition type / media type, not a parameter.
    while(i < segments.size()) {
        var raw = unsafe(*segments.get_ptr(i))
        var segment = raw.trim()
        var is_first = i == 0u
        i = i + 1
        if(is_first) { continue }
        if(segment.size() <= key.size()) { continue }
        var head = segment.subview(0u, key.size())
        if(!mp_iequals(head, key)) { continue }
        if(segment.at(key.size()) != '=') { continue }
        var rest = segment.subview(key.size() + 1u, segment.size()).trim()
        var out = string()
        if(!rest.empty() && rest.at(0u) == '"') {
            var j : usize = 1u
            var closed = false
            while(j < rest.size()) {
                var c = rest.at(j)
                if(c == '\\' && j + 1u < rest.size()) {
                    out.append(rest.at(j + 1u))
                    j = j + 2u
                    continue
                }
                if(c == '"') {
                    closed = true
                    break
                }
                out.append(c)
                j = j + 1u
            }
            if(!closed) { return Option.None<string>() }
            return Option.Some<string>(out)
        }
        out.append_view(&rest)
        return Option.Some<string>(out)
    }
    return Option.None<string>()
}

} // namespace http
