// ===== Byte ranges (`Range: bytes=…`) =====
//
// A resumable download, a video seek and a browser's partial fetch all send
// `Range: bytes=<first>-<last>`. Without support the server answers 200 with the
// whole body: a seek re-downloads the file from byte 0 and an interrupted download
// cannot resume. That is a production gap, not a nicety.
//
// Scope, stated so it cannot surprise anyone:
//
//   * `bytes` units only, and a **single** range. The multi-range form needs
//     `multipart/byteranges`, which nothing in this stack emits; a request for two
//     ranges gets the whole representation (200), which RFC 9110 permits for a
//     server that does not support it.
//   * in-memory bodies (`Text`, `Bytes`, `View`). A `View` is sliced without a
//     copy because it borrows a longer-lived buffer; an owned body is copied. A
//     `File` body keeps streaming the whole file (200).
//   * `If-Range` is honoured when it is an `ETag`: a client holding a stale ETag
//     gets the whole current representation instead of a slice of a different one.
public namespace http {

using std::Option;
using std::string;
using std::string_view;

/// What a response should do about the request's `Range` header.
public variant RangeDecision {
    /// No range was asked for (or it cannot be honoured): answer the whole body.
    /// `Accept-Ranges: bytes` is still advertised when the body is in memory.
    None()
    /// Bytes `first .. last` (both inclusive) of the representation.
    Slice(first : usize, last : usize)
    /// A syntactically valid range that the representation cannot satisfy: `416`.
    Unsatisfiable()

    public func is_slice(&self) : bool {
        switch(self) {
            Slice(_) => { return true }
            default => { return false }
        }
    }
}

func range_digits(view : string_view, from : usize, to : usize, out : *mut usize) : bool {
    if(from >= to) { return false }
    var i = from
    var value : usize = 0
    while(i < to) {
        var c = view.at(i)
        if(c < '0' || c > '9') { return false }
        value = value * 10u + (c as usize - 48u)
        i = i + 1u
    }
    unsafe { *out = value }
    return true
}

/// Resolve a `Range` header value against `length` bytes of representation.
///
/// Handles `first-last`, `first-` and `-suffix`, ignores any other unit, and
/// reports `Unsatisfiable` for a range that starts past the end (RFC 9110 §14.1.2:
/// that is a `416`, not a `200`). A `last` past the end is clamped.
public func parse_byte_range(value : string_view, length : usize) : RangeDecision {
    var prefix = string_view("bytes=")
    if(value.size() <= prefix.size() || !text_equals(value.subview(0u, prefix.size()), prefix)) {
        return RangeDecision.None()
    }
    var spec = value.subview(prefix.size(), value.size())
    // A comma means multiple ranges; serve the whole representation instead.
    var comma = spec.find(std::string_view(","))
    if(comma is Option.Some) { return RangeDecision.None() }
    var dash_at : usize = 0
    var found_dash = false
    var scan : usize = 0
    while(scan < spec.size()) {
        if(spec.at(scan) == '-') {
            dash_at = scan
            found_dash = true
            break
        }
        scan = scan + 1u
    }
    if(!found_dash) { return RangeDecision.None() }

    var first : usize = 0
    var last : usize = 0
    var has_first = range_digits(spec, 0u, dash_at, &raw mut first)
    var has_last = false
    if(dash_at + 1u < spec.size()) {
        has_last = range_digits(spec, dash_at + 1u, spec.size(), &raw mut last)
        if(!has_last) { return RangeDecision.None() }
    }
    if(!has_first && !has_last) { return RangeDecision.None() }

    if(length == 0u) { return RangeDecision.Unsatisfiable() }

    if(!has_first) {
        // `-N`: the last N bytes. A suffix larger than the body is the whole body.
        if(last == 0u) { return RangeDecision.Unsatisfiable() }
        if(last >= length) { return RangeDecision.Slice(0u, length - 1u) }
        return RangeDecision.Slice(length - last, length - 1u)
    }
    if(first >= length) { return RangeDecision.Unsatisfiable() }
    var end = length - 1u
    if(has_last) {
        if(last < first) { return RangeDecision.None() }
        if(last < end) { end = last }
    }
    return RangeDecision.Slice(first, end)
}

/// True when the body can be served in pieces without touching the disk.
func body_is_rangeable(body : &ResponseBody) : bool {
    switch(body) {
        Text(_) => { return true }
        Bytes(_) => { return true }
        View(_) => { return true }
        default => { return false }
    }
}

/// `body` cut to `first .. last` (inclusive). A `View` is sliced in place — it
/// already borrows bytes that outlive the response — while an owned body is copied,
/// because a view into it would point at storage the response replaces.
func body_slice(body : &ResponseBody, first : usize, last : usize) : ResponseBody {
    if(last < first) { return ResponseBody.Empty() }
    switch(body) {
        Text(t) => {
            var out = string()
            var view = unsafe(t.to_view())
            out.reserve(last + 1u - first)
            out.append_view(&unsafe(view.subview(first, last + 1u)))
            return ResponseBody.Text(out)
        }
        Bytes(d) => {
            var out = std::vector<u8>()
            out.reserve(last + 1u - first)
            var base = unsafe(d.data() as *u8)
            var i = first
            while(i <= last) {
                out.push_back(unsafe(base[i]))
                i = i + 1u
            }
            return ResponseBody.Bytes(out)
        }
        View(v) => {
            return ResponseBody.View(unsafe(v.subview(first, last + 1u)))
        }
        default => { return ResponseBody.Empty() }
    }
}

/// Rewrite `res` for `decision`: `206` + `Content-Range`, `416` + `Content-Range:
/// bytes *​/<len>`, or just advertise `Accept-Ranges: bytes`.
///
/// Called by the server immediately before writing, so a handler never has to think
/// about ranges and an artifact gets them for free.
public func apply_byte_range(res : &mut Response, decision : RangeDecision) {
    if(res.headers.contains("Content-Encoding")) {
        // A range applies to the representation that was sent, and a compressed one
        // has no byte offsets the client knows about. Serve the whole thing.
        return
    }
    var rangeable = body_is_rangeable(&res.body)
    switch(decision) {
        None() => {
            if(rangeable && res.status == 200u && !res.headers.contains("Accept-Ranges")) {
                res.set_header_view("Accept-Ranges", "bytes")
            }
            return
        }
        Unsatisfiable() => {
            if(res.status != 200u) { return }
            var length = res.body.size()
            var content_range = string()
            content_range.append_view("bytes */")
            content_range.append_uinteger(length)
            var header = content_range.to_view()
            res.set_header_view("Content-Range", &header)
            res.set_header_view("Accept-Ranges", "bytes")
            res.status = 416u
            res.body = ResponseBody.Empty()
            return
        }
        Slice(first, last) => {
            if(res.status != 200u || !rangeable) { return }
            var length = res.body.size()
            var content_range = string()
            content_range.append_view("bytes ")
            content_range.append_uinteger(first as ubigint)
            content_range.append('-')
            content_range.append_uinteger(last as ubigint)
            content_range.append('/')
            content_range.append_uinteger(length)
            var header = content_range.to_view()
            res.set_header_view("Content-Range", &header)
            res.set_header_view("Accept-Ranges", "bytes")
            res.status = 206u
            res.body = body_slice(&res.body, first, last)
            return
        }
    }
}

/// `decision` for a request: `None` when `If-Range` names a different `ETag` than
/// the response carries (the client's copy is stale, so it needs the whole thing).
public func range_decision_for(range : string_view, if_range : Option<string_view>,
                               etag : Option<string_view>, length : usize) : RangeDecision {
    if(if_range is Option.None) {
        return parse_byte_range(range, length)
    }
    var Some(want) = if_range else unsafe { unreachable }
    if(etag is Option.None) {
        return parse_byte_range(range, length)
    }
    var Some(current) = etag else unsafe { unreachable }
    if(!text_equals(want, current)) { return RangeDecision.None() }
    return parse_byte_range(range, length)
}

} // namespace http
