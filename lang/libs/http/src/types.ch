// ===== HTTP utilities and types =====
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
    Other(m : string)

    public func message(&self) : string_view {
        switch(self) {
            Io(e) => { return e.message() }
            Tls(e) => { return e.message() }
            InvalidUrl() => { return string_view("invalid URL") }
            Connect(m) => { return m.to_view() }
            Protocol(m) => { return m.to_view() }
            HeaderTooLarge() => { return string_view("response headers too large") }
            BodyTooLarge() => { return string_view("response body too large") }
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

// ---------------------------------------------------------------------------
// Transport
// ---------------------------------------------------------------------------
//
// A single owning connection handle. The async HTTP client keeps the legacy
// pointer-returning entry points under `http::raw`; the synchronous surface and
// the server use this enum, so no caller sees a descriptor or TLS context.

public struct Transport {
    internal var kind : int          // 0 plain, 1 TLS client, 2 TLS server, -1 closed
    internal var plain : *mut net::TcpStream
    internal var tls_client : *mut tls::TlsStream
    internal var tls_server : *mut tls::TlsServerStream

    @constructor
    func constructor() {
        return Transport {
            kind : -1,
            plain : null,
            tls_client : null,
            tls_server : null
        }
    }

    @delete
    func delete(&mut self) {
        if(self.plain != null) {
            unsafe { delete self.plain }
            self.plain = null
        }
        if(self.tls_client != null) {
            unsafe { delete self.tls_client }
            self.tls_client = null
        }
        if(self.tls_server != null) {
            unsafe { delete self.tls_server }
            self.tls_server = null
        }
        self.kind = -1
    }

    public func read_bytes(&mut self, buf : &mut [u8]) : Result<size_t, std::io::IoError> {
        if(self.kind == 0 && self.plain != null) {
            unsafe {
                var p = &mut *self.plain
                return p.read_bytes(buf)
            }
        }
        if(self.kind == 1 && self.tls_client != null) {
            unsafe {
                var p = &mut *self.tls_client
                return p.read_bytes(buf)
            }
        }
        if(self.kind == 2 && self.tls_server != null) {
            unsafe {
                var p = &mut *self.tls_server
                return p.read_bytes(buf)
            }
        }
        return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
    }

    public func write_bytes(&mut self, buf : &[u8]) : Result<size_t, std::io::IoError> {
        if(self.kind == 0 && self.plain != null) {
            unsafe {
                var p = &mut *self.plain
                return p.write_bytes(buf)
            }
        }
        if(self.kind == 1 && self.tls_client != null) {
            unsafe {
                var p = &mut *self.tls_client
                return p.write_bytes(buf)
            }
        }
        if(self.kind == 2 && self.tls_server != null) {
            unsafe {
                var p = &mut *self.tls_server
                return p.write_bytes(buf)
            }
        }
        return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
    }

    public func write_all_bytes(&mut self, buf : &[u8]) : Result<size_t, std::io::IoError> {
        var pos : size_t = 0
        while(pos < buf.size()) {
            var sub : [u8] = buf.subview(pos, buf.size())
            var r = self.write_bytes(sub)
            if(r is Result.Err) { return r }
            var Ok(n) = r else unsafe { unreachable }
            if(n == 0) { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::Other())) }
            pos += n
        }
        return Result.Ok(pos)
    }

    public func read_exact_bytes(&mut self, buf : &mut [u8]) : Result<size_t, std::io::IoError> {
        var pos : size_t = 0
        while(pos < buf.size()) {
            var sub = unsafe(std::mut_span<u8>(unsafe(buf.data() + pos), buf.size() - pos))
            var r = self.read_bytes(sub)
            if(r is Result.Err) { return r }
            var Ok(n) = r else unsafe { unreachable }
            if(n == 0) { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::UnexpectedEof())) }
            pos += n
        }
        return Result.Ok(pos)
    }

    public func try_read_to_end_bytes(&mut self, out : &mut vector<u8>) : Result<size_t, std::io::IoError> {
        var chunk : [8192]u8
        var total : size_t = 0
        while(true) {
            var view = unsafe(std::mut_span<u8>(&raw mut chunk[0], 8192 as size_t))
            var r = self.read_bytes(view)
            if(r is Result.Err) { return r }
            var Ok(n) = r else unsafe { unreachable }
            if(n == 0) { return Result.Ok(total) }
            var i : size_t = 0
            while(i < n) {
                if(!out.try_push(chunk[i])) {
                    return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::OutOfMemory()))
                }
                i += 1
            }
            total += n
        }
        return Result.Ok(total)
    }

    public func set_read_timeout(&mut self, ms : u64) : Result<std::Unit, std::io::IoError> {
        if(self.kind == 0 && self.plain != null) {
            unsafe {
                var p = &mut *self.plain
                return p.set_read_timeout(ms)
            }
        }
        if(self.kind == 1 && self.tls_client != null) {
            unsafe {
                var p = &mut *self.tls_client
                return p.set_read_timeout(ms)
            }
        }
        if(self.kind == 2 && self.tls_server != null) {
            unsafe {
                var p = &mut *self.tls_server
                return p.set_read_timeout(ms)
            }
        }
        return Result.Ok(std::Unit { })
    }

    public func shutdown(&mut self) : Result<std::Unit, std::io::IoError> {
        if(self.kind == 0 && self.plain != null) {
            unsafe {
                var p = &mut *self.plain
                return p.shutdown(net::Shutdown.Both())
            }
        }
        if(self.kind == 1 && self.tls_client != null) {
            unsafe {
                var p = &mut *self.tls_client
                return p.close_notify()
            }
        }
        if(self.kind == 2 && self.tls_server != null) {
            unsafe {
                var p = &mut *self.tls_server
                return p.close_notify()
            }
        }
        return Result.Ok(std::Unit { })
    }

    /// Detach the raw descriptor from a plain transport (Windows IOCP path).
    public func detach(&mut self) : net::raw::Socket {
        if(self.kind == 0 && self.plain != null) {
            return unsafe(net::raw::release_tcp(&mut *self.plain))
        }
        return 0 as net::raw::Socket
    }
}

impl std::io::Read for Transport {
    func read(&mut self, buf : &mut [u8]) : Result<size_t, std::io::IoError> {
        return self.read_bytes(buf)
    }
    func read_exact(&mut self, buf : &mut [u8]) : Result<size_t, std::io::IoError> {
        return self.read_exact_bytes(buf)
    }
    func try_read_to_end(&mut self, out : &mut vector<u8>) : Result<size_t, std::io::IoError> {
        return self.try_read_to_end_bytes(out)
    }
}

impl std::io::Write for Transport {
    func write(&mut self, buf : &[u8]) : Result<size_t, std::io::IoError> {
        return self.write_bytes(buf)
    }
    func write_all(&mut self, buf : &[u8]) : Result<size_t, std::io::IoError> {
        return self.write_all_bytes(buf)
    }
    func flush(&mut self) : Result<size_t, std::io::IoError> {
        return Result.Ok(0 as size_t)
    }
}

// Owning constructors. Streams live behind heap pointers so copying the
// `Transport` value can never duplicate a descriptor or TLS context.
func transport_box_plain(fd : net::raw::Socket) : *mut Transport {
    var t = unsafe(malloc(sizeof(Transport))) as *mut Transport
    if(t == null) { return null }
    unsafe {
        *t = Transport()
        t.kind = 0
        t.plain = net::raw::stream_box_from_fd(fd)
    }
    return t
}

func transport_box_tls(box : *mut tls::TlsStream) : *mut Transport {
    var t = unsafe(malloc(sizeof(Transport))) as *mut Transport
    if(t == null) { return null }
    unsafe {
        *t = Transport()
        t.kind = 1
        t.tls_client = box
    }
    return t
}

func transport_box_tls_server(box : *mut tls::TlsServerStream) : *mut Transport {
    var t = unsafe(malloc(sizeof(Transport))) as *mut Transport
    if(t == null) { return null }
    unsafe {
        *t = Transport()
        t.kind = 2
        t.tls_server = box
    }
    return t
}

// ---------------------------------------------------------------------------
// Headers / query
// ---------------------------------------------------------------------------

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

    public func print(&self) {
        var i = 0u;
        unsafe { printf("headers:\n") };
        while(i < headers.size()) {
            var p = headers.get_ptr(i);
            unsafe { printf("\theader '%s' : '%s'\n", p.first.data(), p.second.data()) };
            i = i + 1u;
        }
    }

    public func insert(&mut self, k : string, v : string) {
        headers.push_back(std::pair<string, string>{ first : k, second : v })
    }

    public func insert_view(&mut self, k : &string_view, v : &string_view) {
        headers.push_back(std::pair<string, string>{
            first : unsafe(string(k.data(), k.size())),
            second : unsafe(string(v.data(), v.size()))
        })
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
        var want = name.to_view()
        var i = 0u
        while(i < headers.size()) {
            var p = headers.get_ptr(i)
            if(header_name_equals(unsafe(p.first.to_view()), want)) {
                return Option.Some<string>(unsafe(p.second.copy()))
            }
            i += 1u
        }
        return Option.None<string>()
    }

    // Compatibility alias for the pre-canonical owned lookup.
    public func get_str(&self, name : &str) : Option<string> {
        return self.get_owned(name)
    }

    public func get_view(&self, name : &str) : Option<string_view> {
        return self.get(name)
    }
}

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

    public func get(&self, key : &string_view) : string_view {
        var i = 0u;
        while(i < pairs.size()) {
            var p = pairs.get_ptr(i);
            unsafe {
                if(p.first.equals_view(key)) { return p.second.to_view() }
            }
            i = i + 1u;
        }
        return string_view()
    }
}

// helper: simple ASCII case-insensitive strcmp. Raw C-string arguments
// cannot be length-validated here, so the operation is an unsafe boundary.
public unsafe func strcasecmp(a:*char, b:*char) : int {
    unsafe {
    if(a == null && b == null) { return 0 }
    if(a == null || b == null) { return 1 }

    var i = 0;
    loop {
        var ca = a[i]; var cb = b[i];
        if(ca == '\0' && cb == '\0') { return 0 }
        var la = ca; var lb = cb;
        if(la >= 'A' && la <= 'Z') { la = (la + 32) as char }
        if(lb >= 'A' && lb <= 'Z') { lb = (lb + 32) as char }
        if(la != lb) { return (la as int) - (lb as int) }
        i = i + 1;
    }
    }
}

// ---------------------------------------------------------------------------
// Body
// ---------------------------------------------------------------------------

// Body: lazy reader for a request/response body. The connection is owned by
// `owned_transport` (client responses) or borrowed through `transport`
// (server requests) and is never exposed as a public raw field.
@direct_init
public struct Body {

    internal var buf : *mut net::Buffer
    internal var remaining : isize
    internal var chunked : bool
    internal var timeout_secs : long
    internal var max_body : usize
    internal var closed : bool
    internal var cur_chunk_left : usize
    internal var seen_total : usize
    internal var owns_buf : bool
    internal var transport : *mut Transport
    internal var owns_transport : bool

    @constructor
    func constructor() {
        return Body {
            buf : null,
            remaining : -1,
            chunked : false,
            timeout_secs : 0,
            max_body : 0,
            closed : false,
            cur_chunk_left : 0,
            seen_total : 0,
            owns_buf : false,
            transport : null,
            owns_transport : false
        }
    }

    @delete
    func destruct(&mut self) {
        if(self.owns_buf && self.buf != null) {
            unsafe { delete self.buf }
            self.buf = null
            self.owns_buf = false
        }
        if(self.owns_transport && self.transport != null) {
            unsafe { delete self.transport }
            self.transport = null
            self.owns_transport = false
        }
    }

    /// Create a body that owns `transport`. The caller's already-read bytes in
    /// `buf_ptr` are copied into a Body-owned buffer.
    func make_owned(transport : *mut Transport,
                    buf_ptr : *mut net::Buffer,
                    content_len : isize,
                    chunked : bool,
                    timeout_secs : long,
                    max_body : usize) : Body {
        var body = Body()
        body.transport = transport
        body.owns_transport = true
        body.remaining = content_len
        body.chunked = chunked
        body.timeout_secs = timeout_secs
        body.max_body = max_body
        body.buf = new net::Buffer()
        body.owns_buf = true
        unsafe {
            if(buf_ptr != null && buf_ptr.len() > 0u) {
                body.buf.append_bytes(buf_ptr.as_ptr(), buf_ptr.len())
            }
        }
        return body
    }

    /// Create a body borrowing a server-owned transport box. The box must stay
    /// alive until the body is closed.
    func make_borrowed(transport : *mut Transport,
                       buf_ptr : *mut net::Buffer,
                       content_len : isize,
                       chunked : bool,
                       timeout_secs : long,
                       max_body : usize) : Body {
        var body = Body()
        body.transport = transport
        body.owns_transport = false
        body.remaining = content_len
        body.chunked = chunked
        body.timeout_secs = timeout_secs
        body.max_body = max_body
        body.buf = new net::Buffer()
        body.owns_buf = true
        unsafe {
            if(buf_ptr != null && buf_ptr.len() > 0u) {
                body.buf.append_bytes(buf_ptr.as_ptr(), buf_ptr.len())
            }
        }
        return body
    }

    public func remaining_bytes(&self) : isize {
        return self.remaining
    }

    public func is_closed(&self) : bool {
        return self.closed
    }

    func recv_raw(&mut self, dst : *mut u8, cap : usize) : int {
        if(cap == 0u) { return 0 }
        if(cap > 2147483647 as usize) { return -1 }
        if(self.transport == null) { return -1 }
        var view = unsafe(std::mut_span<u8>(dst, cap))
        var r = unsafe(self.transport.read_bytes(view))
        if(r is Result.Err) { return -1 }
        var Ok(n) = r else unsafe { unreachable }
        return n as int
    }
}

// low-level recv helper that honors timeout and returns <=0 on error/timeout
func body_recv(b:*mut Body, dst:*mut u8, cap: usize) : int {
    unsafe {
    if(b.transport != null) {
        var _ = b.transport.set_read_timeout((b.timeout_secs * 1000) as u64)
    }
    return b.recv_raw(dst, cap)
    }
}

// Helper: copy up to `want` bytes from the already-read buffer into dst, returning copied count.
func copy_from_buffer(b:*mut Body, dst:*mut u8, want: usize) : usize {
    unsafe {
    if(b.buf == null) { return 0u }
    var bufref = b.buf;
    var avail = bufref.len();
    var take = if(avail < want) avail else want;
    if(take == 0u) { return 0u }
    var i = 0u;
    var srcp = bufref.as_ptr();
    while(i < take) {
        dst[i] = srcp[i];
        i = i + 1u;
    }
    bufref.consume(take);
    return take;
    }
}

// Internal raw read: `dst`/`cap` are caller-validated by the safe wrappers.
func (b: &mut Body) read_raw(dst:*mut u8, cap: usize) : int {
    unsafe {
    if(cap > 2147483647 as usize) { return -1 }
    if(cap > 0u && dst == null) { return -1 }
    if(cap == 0u) { return 0 }
    if(b.closed) { return 0 }
    if(b.max_body > 0u && b.seen_total >= b.max_body) { return -1 }

    if(b.chunked) {
        if(b.cur_chunk_left > 0u) {
            var want = if(cap < b.cur_chunk_left) cap else b.cur_chunk_left;
            var copied = copy_from_buffer(&raw mut b, dst, want);
            var total_read = copied;
            while(total_read < want) {
                var n = body_recv(&raw mut b, &raw mut dst[total_read], want - total_read);
                if(n <= 0) { return -1 }
                total_read = total_read + (n as usize);
            }
            b.cur_chunk_left = b.cur_chunk_left - want;
            b.seen_total = b.seen_total + want;
            if(b.max_body > 0u && b.seen_total > b.max_body) { return -1 }
            if(b.cur_chunk_left == 0u) {
                var tmp : [2]u8;
                var c = copy_from_buffer(&raw mut b, &raw mut tmp[0], 2);
                if(c < 2) {
                    var rem = 2 - c;
                    var n = body_recv(&raw mut b, &raw mut tmp[c], rem);
                    if(n < 0) { b.closed = true; return 0 }
                    if(n == 0) {
                        if(c == 0u) { b.closed = true; return 0 }
                        b.closed = true
                        return 0
                    }
                }
            }
            return (want as int);
        }
        var linebuf = string::empty_str();
        var found_crlf = false;
        if(b.buf != null && b.buf.len() > 0u) {
            var i = 0u;
            while(i + 1 < b.buf.len()) {
                var a = b.buf.get_byte(i);
                var bb = b.buf.get_byte(i + 1u);
                if(a == '\r' as u8 && bb == '\n' as u8) {
                    var nline = i;
                    var ptr = b.buf.as_ptr();
                    var tmpline = http_ctor(ptr as *char, nline as size_t);
                    b.buf.consume(nline + 2u);
                    linebuf = tmpline;
                    found_crlf = true;
                    break;
                }
                i = i + 1u;
            }
        }
        while(!found_crlf) {
            var tmp : [64]u8;
            var n = body_recv(&raw mut b, &raw mut tmp[0], 64);
            if(n < 0) { b.closed = true; return 0 }
            if(n == 0) { b.closed = true; return 0 }
            var i = 0;
            while(i < n) {
                var ch = tmp[i];
                linebuf.append(ch as char);
                var L = linebuf.size();
                if(L >= 2 && linebuf.at(L-2u) == '\r' && linebuf.at(L-1u) == '\n') {
                    linebuf = linebuf.substring(0u, L-2u);
                    found_crlf = true;
                    if((i as usize) + 1u < (n as usize)) {
                        var rem_n = (n as usize) - ((i as usize) + 1u);
                        if(b.buf != null) { b.buf.append_bytes(&raw mut tmp[i + 1], rem_n) }
                    }
                    break;
                }
                i = i + 1;
            }
        }
        var hex_ok = true;
        var hex_val : usize = 0u;
        var ii = 0u;
        while(ii < linebuf.size()) {
            var c = linebuf.at(ii);
            if(c == ';' as char) { break }
            var v = hex_value(c);
            if(v < 0) { hex_ok = false; break }
            hex_val = (hex_val * 16u) + (v as usize);
            ii = ii + 1u;
        }
        if(!hex_ok) { return -1 }
        if(hex_val == 0u) {
            var found = false;
            while(!found) {
                if(b.buf != null) {
                    var L = b.buf.len();
                    var i = 0u;
                    while(i + 1u < L) {
                        if(b.buf.get_byte(i) == '\r' as u8 && b.buf.get_byte(i + 1u) == '\n' as u8) {
                            if(i == 0u || (i >= 2u && b.buf.get_byte(i - 2u) == '\r' as u8 && b.buf.get_byte(i - 1u) == '\n' as u8)) {
                                b.buf.consume(i + 2u);
                                found = true;
                                break;
                            }
                        }
                        i = i + 1u;
                    }
                }
                if(!found) {
                    var tmp : [8192u]u8;
                    var n = body_recv(&raw mut b, &raw mut tmp[0], 8192u);
                    if(n <= 0) { b.closed = true; return 0 }
                    if(b.buf != null) { b.buf.append_bytes(&raw mut tmp[0], n as usize) }
                }
            }
            b.closed = true;
            return 0;
        }
        b.cur_chunk_left = hex_val;
        return b.read_raw(dst, cap);
    }

    if(b.remaining >= 0) {
        var want = if((b.remaining as usize) < cap) (b.remaining as usize) else cap;
        var copied = copy_from_buffer(&raw mut b, dst, want);
        var total = copied;
        while(total < want) {
            var n = body_recv(&raw mut b, &raw mut dst[total], want - total);
            if(n <= 0) { return -1 }
            total = total + (n as usize);
        }
        b.remaining = b.remaining - (total as isize);
        b.seen_total = b.seen_total + total;
        if(b.max_body > 0u && b.seen_total > b.max_body) { return -1 }
        return (total as int);
    } else {
        var copied = copy_from_buffer(&raw mut b, dst, cap);
        var total = copied;
        if(total > 0u) {
            b.seen_total = b.seen_total + total;
            if(b.max_body > 0u && b.seen_total > b.max_body) { return -1 }
            return (total as int);
        }
        var n = body_recv(&raw mut b, dst, cap);
        if(n <= 0) { return 0 }
        b.seen_total = b.seen_total + (n as usize);
        if(b.max_body > 0u && b.seen_total > b.max_body) { return -1 }
        return n;
    }
    }
}

// Safe body reads: no raw pointer on the hot path.
public func (b: &mut Body) read(buf : &mut [u8]) : Result<size_t, std::io::IoError> {
    if(buf.size() > 2147483647 as usize) {
        return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind.InvalidInput()))
    }
    var n = b.read_raw(buf.data(), buf.size())
    if(n < 0) {
        return Result.Err(http_io_error())
    }
    return Result.Ok(n as size_t)
}

/// Fill `buf` completely; an EOF before the buffer is full is `UnexpectedEof`.
public func (b: &mut Body) read_exact(buf : &mut [u8]) : Result<std::Unit, std::io::IoError> {
    var pos : size_t = 0
    while(pos < buf.size()) {
        var sub = unsafe(std::mut_span<u8>(unsafe(buf.data() + pos), buf.size() - pos))
        var r = b.read(sub)
        if(r is Result.Err) { return Result.Err(http_io_error()) }
        var Ok(n) = r else unsafe { unreachable }
        if(n == 0) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::UnexpectedEof()))
        }
        pos += n
    }
    return Result.Ok(std::Unit { })
}

/// Read the rest of the body, appending into `out`. Returns bytes appended.
public func (b: &mut Body) read_to_end(out : &mut vector<u8>) : Result<size_t, std::io::IoError> {
    var tmp : [8192]u8
    var total : size_t = 0
    while(true) {
        var view = unsafe(std::mut_span<u8>(&raw mut tmp[0], 8192 as size_t))
        var r = b.read(view)
        if(r is Result.Err) { return r }
        var Ok(n) = r else unsafe { unreachable }
        if(n == 0) { return Result.Ok(total) }
        var i : size_t = 0
        while(i < n) {
            if(!out.try_push(tmp[i])) {
                return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::OutOfMemory()))
            }
            i += 1
        }
        total += n
    }
    return Result.Ok(total)
}

// Read the rest of the body into an owned byte vector.
public func (b: &mut Body) read_all() : Result<vector<u8>, std::io::IoError> {
    var out = vector<u8>()
    var r = b.read_to_end(&mut out)
    if(r is Result.Err) { return Result.Err(http_io_error()) }
    return Result.Ok(out)
}

/// Read the rest of the body as a UTF-8 string.
public func (b: &mut Body) read_to_string() : Result<string, std::io::IoError> {
    var out = string()
    var tmp : [8192u]u8;
    while(true) {
        var n = b.read_raw(&raw mut tmp[0], 8192u);
        if(n < 0) { return Result.Err(http_io_error()) }
        if(n == 0) { break }
        var part = unsafe(string_view(&tmp[0] as *char, n as size_t))
        out.append_view(&part)
    }
    return Result.Ok(out)
}

/// Compatibility helper for the pre-canonical `Option<string>` surface.
public func (b: &mut Body) read_to_string_opt() : Option<string> {
    var r = b.read_to_string()
    if(r is Result.Err) { return Option.None<string>() }
    var Ok(v) = r else unsafe { unreachable }
    return Option.Some<string>(v)
}

/// Compatibility helper: read exactly `n` bytes as an owned string.
public func (b: &mut Body) read_exact_string(n : usize) : Option<string> {
    var out = string()
    out.reserve(n)
    var rem = n
    var tmp : [8192u]u8
    while(rem > 0u) {
        var want = if(rem < 8192u) rem else 8192u
        var r = b.read_raw(&raw mut tmp[0], want)
        if(r <= 0) { return Option.None<string>() }
        var part = http_ctor(&tmp[0] as *char, r as size_t)
        out.append_string(&part)
        rem = rem - (r as usize)
    }
    return Option.Some<string>(out)
}

// drain remaining body
public func (b: &mut Body) drain() : bool {
    var tmp : [8192u]u8;
    while(true) {
        var n = b.read_raw(&raw mut tmp[0], 8192u);
        if(n < 0) { return false }
        if(n == 0) { break }
    }
    return true;
}

public func (b: &mut Body) close() {
    b.closed = true;
}

public func (b: &Body) is_chunked() : bool {
    return b.chunked;
}

public func (b: &Body) content_length() : isize {
    return b.remaining
}

// Owned async-client result. The legacy pointer-returning entry points live in
// `http::raw`; this value is safe to move and drop.
public struct HttpResult {
    var ok : bool
    var status : uint
    var status_text : string
    var headers : HeaderMap
    var body : string
    var error : string

    public func is_ok(&self) : bool { return self.ok }
    public func status_code(&self) : uint { return self.status }
    public func body_view(&self) : string_view { return self.body.to_view() }
    public func error_view(&self) : string_view { return self.error.to_view() }
}

// ---------------------------------------------------------------------------
// Requests / responses
// ---------------------------------------------------------------------------

public struct Request {
    var method: string;
    var path: string;
    var query : QueryMap;
    var proto: string;
    var headers: HeaderMap;
    var body_len: usize;
    var remote: string;
    var body : Body;
    var route_params : vector<std::pair<string, string>>;

    @constructor func constructor() {
        return Request {
            method = string::empty_str();
            path = string::empty_str();
            query = QueryMap();
            proto = string::empty_str();
            headers = HeaderMap();
            body_len = 0u;
            remote = string::empty_str();
            body : Body();
            route_params : vector<std::pair<string, string>>()
        }
    }
}

public struct Response {
    var status: uint;
    var status_text: string;
    var proto: string;
    var headers: HeaderMap;
    var body_len: usize;
    var body: Body;

    @constructor func constructor() {
        return Response {
            status = 200u;
            status_text = string();
            proto = string();
            headers = HeaderMap();
            body_len = 0u;
            body : Body()
        }
    }
}

} // namespace http
