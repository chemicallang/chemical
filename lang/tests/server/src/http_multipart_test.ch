// ============================================================================
// multipart/form-data — file uploads
// ============================================================================
// A `<form enctype="multipart/form-data">` is the only way a browser uploads a
// file, and it is the one request-body encoding that can carry arbitrary bytes. So
// the tests that matter are the ones a urlencoded parser cannot pass:
//
//   * a part body with a NUL, a CR and a bare LF in it survives byte for byte;
//   * `name`/`filename` are read out of a quoted parameter, and a filename with a
//     space or a quote survives;
//   * a body that stops before its closing boundary is an error, not a form with
//     one part silently missing (a handler must never save half an upload);
//   * `max_parts` caps how many parts one body may split into.
//
// Run with ./scripts/test.sh --tcc --server
// ============================================================================
using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;

func mp_boundary_default() : string_view {
    return string_view("XyZ")
}

// `--<boundary>\r\n<headers>\r\n\r\n<data>` — the opening of one part. The closing
// delimiter is appended by `mp_close`.
func mp_open(body : &mut string, boundary : string_view, headers : string_view) {
    body.append_view("--")
    body.append_view(&boundary)
    body.append_view("\r\n")
    body.append_view(&headers)
    body.append_view("\r\n\r\n")
}

func mp_close(body : &mut string, boundary : string_view) {
    body.append_view("\r\n--")
    body.append_view(&boundary)
    body.append_view("--\r\n")
}

func mp_append_bytes(body : &mut string, bytes : &[u8]) {
    var i : usize = 0
    while(i < bytes.size()) {
        var b = unsafe(*bytes.at(i))
        body.append(b as char)
        i = i + 1
    }
}

func mp_content_type(boundary : string_view) : string {
    var out = string()
    out.append_view("multipart/form-data; boundary=")
    out.append_view(&boundary)
    return out
}

// Parse with the content type the request would carry, hoisted so no temporary
// view outlives the call.
func mp_parse(boundary : string_view, body : string_view, max_parts : usize = 1000) : Result<http::MultipartForm, http::MultipartError> {
    var ct = mp_content_type(boundary)
    var ct_view = ct.to_view()
    return http::parse_multipart(body.as_bytes(), ct_view, max_parts)
}

func mp_bytes_equal(data : &[u8], want : &[u8]) : bool {
    if(data.size() != want.size()) { return false }
    var i : usize = 0
    while(i < data.size()) {
        var have = unsafe(*data.at(i))
        var expect = unsafe(*want.at(i))
        if(have != expect) { return false }
        i = i + 1
    }
    return true
}

// A request carrying `body` as multipart with `boundary`.
func mp_request(boundary : string_view, body : string_view) : http::Request {
    var req = http::request(string_view("POST"), string_view("/upload"))
    var ct = mp_content_type(boundary)
    var ct_view = ct.to_view()
    req.with_header(string_view("Content-Type"), ct_view)
    req.set_body_text(body)
    return req
}

// Borrowed, so a test can still destructure the result afterwards (a by-value
// check would move it).
func mp_is_err(form : &Result<http::MultipartForm, http::MultipartError>) : bool {
    if(form is Result.Err) { return true }
    return false
}

@test
public func MP_text_field_and_binary_file_survive(env : &mut TestEnv) {
    var boundary = mp_boundary_default()
    var body = string()
    mp_open(&mut body, boundary, string_view("Content-Disposition: form-data; name=\"title\""))
    body.append_view("Hello upload")
    body.append_view("\r\n--")
    body.append_view(&boundary)
    body.append_view("\r\n")
    body.append_view("Content-Disposition: form-data; name=\"avatar\"; filename=\"a b.png\"\r\n")
    body.append_view("Content-Type: image/png")
    body.append_view("\r\n\r\n")
    // A PNG signature plus every byte a text-based parser tends to eat: NUL, CR,
    // a lone LF, and a high byte.
    var payload : [9]u8 = zeroed<[9]u8>()
    unsafe {
        payload[0] = 0x89u
        payload[1] = 'P' as u8
        payload[2] = 'N' as u8
        payload[3] = 'G' as u8
        payload[4] = 0u
        payload[5] = 13u
        payload[6] = 10u
        payload[7] = 13u
        payload[8] = 255u
    }
    var payload_span = unsafe(std::span<u8>(&raw mut payload[0], 9u))
    mp_append_bytes(&mut body, payload_span)
    mp_close(&mut body, boundary)

    var body_view = body.to_view()
    var req = mp_request(boundary, body_view)
    var parsed = req.multipart()
    if(mp_is_err(&parsed)) {
        env.error("a well-formed multipart body should parse")
        return
    }
    var Ok(form) = parsed else unsafe { unreachable }
    if(form.size() != 2u) {
        env.error("expected two parts")
        return
    }
    var title = form.field("title")
    if(title is Option.None) {
        env.error("the text field should be present")
        return
    }
    var Some(t) = title else unsafe { unreachable }
    if(!t.equals(&string_view("Hello upload"))) {
        env.error("the text field value should be the part body")
        return
    }
    var upload = form.file("avatar")
    if(upload == null) {
        env.error("the file part should be found by name")
        return
    }
    unsafe {
        var filename = upload.filename_or_empty()
        if(!filename.equals(&string_view("a b.png"))) {
            env.error("a quoted filename with a space should survive")
            return
        }
        var ct_view = upload.content_type_or_empty()
        if(!ct_view.equals(&string_view("image/png"))) {
            env.error("the part's Content-Type should be 'image/png'")
            return
        }
        if(upload.size() != 9u) {
            env.error("the upload should keep all nine bytes")
            return
        }
        var stored = upload.data.as_span()
        if(!mp_bytes_equal(stored, payload_span)) {
            env.error("the upload bytes should be unchanged (NUL, CR, LF, high byte)")
            return
        }
        if(!upload.is_file()) {
            env.error("a part with a filename is a file part")
            return
        }
    }
    if(form.file_count() != 1u) {
        env.error("exactly one part should be a file")
        return
    }
}

@test
public func MP_unquoted_and_escaped_parameters_are_read(env : &mut TestEnv) {
    var boundary = mp_boundary_default()
    var body = string()
    // Unquoted name, and a filename whose value contains an escaped quote.
    mp_open(&mut body, boundary,
        string_view("Content-Disposition: form-data; name=q; filename=\"it\\\"s.txt\""))
    body.append_view("value")
    mp_close(&mut body, boundary)

    var body_view = body.to_view()
    var parsed = mp_parse(boundary, body_view)
    if(mp_is_err(&parsed)) {
        env.error("unquoted parameters should parse")
        return
    }
    var Ok(form) = parsed else unsafe { unreachable }
    var upload = form.part("q")
    if(upload == null) {
        env.error("an unquoted name should be readable")
        return
    }
    unsafe {
        var value = upload.text()
        if(!value.equals(&string_view("value"))) {
            env.error("the part body should be the value")
            return
        }
        var filename = upload.filename_or_empty()
        if(!filename.equals(&string_view("it\"s.txt"))) {
            env.error("a backslash-escaped quote should unescape to a quote")
            return
        }
    }
}

@test
public func MP_missing_boundary_is_reported(env : &mut TestEnv) {
    var body = string()
    body.append_view("not multipart at all")
    var ct = string_view("application/x-www-form-urlencoded")
    var parsed = http::parse_multipart(body.to_view().as_bytes(), ct)
    if(!mp_is_err(&parsed)) {
        env.error("a content type without a boundary must be an error, not an empty form")
        return
    }
    var Err(reason) = parsed else unsafe { unreachable }
    var kind = reason is http::MultipartError.MissingBoundary
    if(!kind) {
        env.error("the error should say the boundary is missing")
    }
}

@test
public func MP_truncated_body_does_not_yield_a_partial_form(env : &mut TestEnv) {
    var boundary = mp_boundary_default()
    var body = string()
    mp_open(&mut body, boundary, string_view("Content-Disposition: form-data; name=\"a\""))
    body.append_view("first")
    // The closing delimiter never arrives: a client that died mid-upload.
    var body_view = body.to_view()
    var parsed = mp_parse(boundary, body_view)
    if(!mp_is_err(&parsed)) {
        env.error("a body that stops before the closing boundary is malformed")
    }
}

@test
public func MP_max_parts_bounds_the_form(env : &mut TestEnv) {
    var boundary = mp_boundary_default()
    var body = string()
    var i : int = 0
    while(i < 3) {
        mp_open(&mut body, boundary, string_view("Content-Disposition: form-data; name=\"n\""))
        body.append_view("x")
        if(i < 2) {
            body.append_view("\r\n--")
            body.append_view(&boundary)
            body.append_view("\r\n")
        }
        i = i + 1
    }
    mp_close(&mut body, boundary)
    var body_view = body.to_view()
    var parsed = mp_parse(boundary, body_view, 2u)
    if(!mp_is_err(&parsed)) {
        env.error("three parts with a limit of two must be refused")
        return
    }
    var Err(reason) = parsed else unsafe { unreachable }
    var limited = reason is http::MultipartError.TooManyParts
    if(!limited) {
        env.error("the error should be TooManyParts")
    }
}

@test
public func MP_fields_read_as_a_plain_form(env : &mut TestEnv) {
    var boundary = mp_boundary_default()
    var body = string()
    mp_open(&mut body, boundary, string_view("Content-Disposition: form-data; name=\"email\""))
    body.append_view("ada@example.com")
    body.append_view("\r\n--")
    body.append_view(&boundary)
    body.append_view("\r\n")
    body.append_view("Content-Disposition: form-data; name=\"file\"; filename=\"\"\r\n\r\n")
    body.append_view("")
    mp_close(&mut body, boundary)

    var body_view = body.to_view()
    var parsed = mp_parse(boundary, body_view)
    if(mp_is_err(&parsed)) {
        env.error("the body should parse")
        return
    }
    var Ok(form) = parsed else unsafe { unreachable }
    // An empty file input (`filename=""`) is a file part of size zero, so it must not
    // show up as a text field with an empty value.
    if(form.file_count() != 1u) {
        env.error("a filename=\"\" part is still a file part")
        return
    }
    var text_fields = form.fields()
    var email = text_fields.get("email")
    if(email is Option.None) {
        env.error("a text field should be reachable through Form.get")
        return
    }
    var Some(value) = email else unsafe { unreachable }
    if(!value.equals(&string_view("ada@example.com"))) {
        env.error("the text field value should be decoded unchanged")
        return
    }
    if(text_fields.has("file")) {
        env.error("a file part must not appear as a text field")
    }
}
