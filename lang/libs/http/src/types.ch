// ===== HTTP utilities and types =====
public namespace http {

    public struct HeaderMap {

        var headers: std::vector<std::pair<std::string,std::string>>;

        @make
        func make() {
            return HeaderMap {
                headers : std::vector<std::pair<std::string,std::string>>()
            }
        }

        func empty(&self) : bool {
            return headers.empty()
        }

        func print(&self) {
            var i = 0u;
            printf("headers:\n");
            while(i < headers.size()) {
                var p = headers.get_ptr(i);
                printf("\theader '%s' : '%s'\n", p.first.data(), p.second.data());
                i = i + 1u;
            }
        }

        func insert(&mut self, k: std::string, v: std::string) {
            headers.push_back(std::pair<std::string,std::string>{ first : k, second : v })
        }

        func insert_view(&mut self, k: &std::string_view, v: &std::string_view) {
            headers.push_back(std::pair<std::string,std::string>{ first : std::string(k.data(), k.size()), second : std::string(v.data(), v.size()) })
        }

        func get(&self, name: *char) : std::Option<std::string> {
            var i = 0u;
            while(i < headers.size()) {
                var p = headers.get_ptr(i);
                // case-insensitive compare using manual loop
                if(strcasecmp(p.first.data(), name) == 0) { return std::Option.Some<std::string>(p.second.copy()) }
                i = i + 1u;
            }
            return std::Option.None<std::string>();
        }

        public func get_view(&self, name : &std::string_view) : std::string_view {
            var i = 0u;
            while(i < headers.size()) {
                var p = headers.get_ptr(i);
                if(strcasecmp(p.first.data(), name.data()) == 0) { return p.second.to_view() }
                i = i + 1u;
            }
            return std::string_view()
        }

    }

    public struct QueryMap {
        var pairs : std::vector<std::pair<std::string, std::string>>

        @make
        func make() {
            return QueryMap {}
        }

        @make
        func make(qs : &std::string_view) {
            return QueryMap {
                pairs : parse_query(qs)
            }
        }

        public func get(&self, key : &std::string_view) : std::string_view {
            var i = 0u;
            while(i < pairs.size()) {
                var p = pairs.get_ptr(i);
                if(p.first.equals_view(key)) { return p.second.to_view() }
                i = i + 1u;
            }
            return std::string_view()
        }
    }

    // helper: simple ASCII case-insensitive strcmp
    public func strcasecmp(a:*char, b:*char) : int {
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

    // Body: lazy reader for request body. Does NOT close the socket by itself.
    @direct_init
    public struct Body {

        var sock: net::Socket;               // underlying socket (do not close here)
        var buf: *mut net::Buffer;       // pointer to buffer that holds bytes already read
        var remaining: isize;                // -1 meaning unknown (no Content-Length), otherwise bytes left
        var chunked: bool;                   // transfer-encoding: chunked
        var timeout_secs: long;              // timeout for subsequent recv calls
        var max_body: usize;                 // configured max body size (for limits)
        var closed: bool;                    // has the handler closed this Body?
        // chunked state
        var cur_chunk_left: usize;           // bytes left in current chunk when chunked==true
        var seen_total: usize;               // how many body bytes have been delivered to user (enforce max_body)
        var owns_buf: bool;                  // whether this Body owns the buf pointer (must free it)

        @make
        func empty_make() {
            return Body {
                sock = 0
                buf = null
                remaining = -1
                chunked = false
                timeout_secs = 0
                max_body = 0
                closed = false
                cur_chunk_left = 0u
                seen_total = 0u
                owns_buf = false
            }
        }

        @delete
        func destruct(&self) {
            if(self.owns_buf && self.buf != null) {
                delete self.buf;
            }
        }

        // construct body from request metadata
        public func make_body(
            sock: net::Socket,
            buf_ptr: *mut net::Buffer,
            content_len: isize,
            chunked: bool,
            timeout_secs: long,
            max_body: usize,
            owns: bool = false
        ) : Body {
            return Body{
                sock: sock,
                buf: buf_ptr,
                remaining: content_len,
                chunked: chunked,
                timeout_secs: timeout_secs,
                max_body: max_body,
                closed: false,
                cur_chunk_left: 0u,
                seen_total: 0u,
                owns_buf: owns
            };
        }
    }

    // low-level recv helper that honors timeout and returns <=0 on error/timeout
    func body_recv(b:*mut Body, dst:*mut u8, cap: usize) : int {
        // set timeout on socket
        net::set_recv_timeout(b.sock, b.timeout_secs, 0);
        return net::recv_all(b.sock, dst, cap);
    }

    // Helper: copy up to `want` bytes from the already-read buffer into dst, returning copied count.
    func copy_from_buffer(b:*mut Body, dst:*mut u8, want: usize) : usize {
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

    // PUBLIC: read up to `cap` bytes into dst
    func (b: &mut Body) read(dst:*mut u8, cap: usize) : int {
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
                        if(n <= 0) { return -1 }
                    }
                }
                return (want as int);
            }
            var linebuf = std::string::empty_str();
            if(b.buf != null && b.buf.len() > 0u) {
                var i = 0u;
                while(i + 1 < b.buf.len()) {
                    var a = b.buf.get_byte(i);
                    var bb = b.buf.get_byte(i + 1u);
                    if(a == '\r' as u8 && bb == '\n' as u8) {
                        var nline = i;
                        var ptr = b.buf.as_ptr();
                        var tmpline = std::string::constructor(ptr as *char, nline as size_t);
                        b.buf.consume(nline + 2u);
                        linebuf = tmpline;
                        break;
                    }
                    i = i + 1u;
                }
            }
            var found_crlf = false;
            while(!found_crlf) {
                var tmp : [64]u8;
                var n = body_recv(&raw mut b, &raw mut tmp[0], 64);
                if(n <= 0) { return -1 }
                var i = 0;
                while(i < n) {
                    var ch = tmp[i];
                    linebuf.append(ch as char);
                    var L = linebuf.size();
                    if(L >= 2 && linebuf.get(L-2u) == '\r' && linebuf.get(L-1u) == '\n') {
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
                var c = linebuf.get(ii);
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
                        if(L >= 4u) {
                            var i = 0u;
                            while(i + 3u < L) {
                                if(b.buf.get_byte(i) == '\r' as u8 && b.buf.get_byte(i+1u) == '\n' as u8 &&
                                   b.buf.get_byte(i+2u) == '\r' as u8 && b.buf.get_byte(i+3u) == '\n' as u8) {
                                    b.buf.consume(i + 4u);
                                    found = true;
                                    break;
                                }
                                i = i + 1u;
                            }
                        }
                    }
                    if(!found) {
                        var tmp : [8192u]u8;
                        var n = body_recv(&raw mut b, &raw mut tmp[0], 8192u);
                        if(n <= 0) { return -1 }
                        if(b.buf != null) { b.buf.append_bytes(&raw mut tmp[0], n as usize) }
                    }
                }
                b.closed = true;
                return 0;
            }
            b.cur_chunk_left = hex_val;
            return b.read(dst, cap);
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

    // read all remaining body into a string
    public func (b: &mut Body) read_to_string() : std::Option<std::string> {
        var out = std::string::empty_str();
        var tmp : [8192u]u8;
        while(true) {
            var n = b.read(&raw mut tmp[0], 8192u);
            if(n < 0) { return std::Option.None<std::string>() }
            if(n == 0) { break }
            out.append_view(std::string_view(&tmp[0] as *char, n as size_t))
        }
        return std::Option.Some<std::string>(out);
    }

    // read exactly `n` bytes
    public func (b: &mut Body) read_exact(n: usize) : std::Option<std::string> {
        var out = std::string::empty_str();
        out.reserve(n);
        var rem = n;
        var tmp : [8192u]u8;
        while(rem > 0u) {
            var want = if(rem < 8192u) rem else 8192u;
            var r = b.read(&raw mut tmp[0], want);
            if(r <= 0) { return std::Option.None<std::string>() }
            var part = std::string::constructor(&tmp[0] as *char, r as size_t);
            out.append_string(&part);
            rem = rem - (r as usize);
        }
        return std::Option.Some<std::string>(out);
    }

    // drain remaining body
    public func (b: &mut Body) drain() : bool {
        var tmp : [8192u]u8;
        while(true) {
            var n = b.read(&raw mut tmp[0], 8192u);
            if(n < 0) { return false }
            if(n == 0) { break }
        }
        return true;
    }

    // close resources
    public func (b: &mut Body) close() {
        b.closed = true;
    }

    public struct Request {
        var method: std::string;
        var path: std::string;
        var query : QueryMap;
        var proto: std::string;
        var headers: HeaderMap;
        var body_len: usize;
        var remote: std::string;
        var body : Body

        @constructor func constructor() {
            return Request {
                method = std::string::empty_str();
                path = std::string::empty_str();
                query = QueryMap();
                proto = std::string::empty_str();
                headers = HeaderMap();
                body_len = 0u;
                remote = std::string::empty_str();
                body : Body()
            }
        }
    }

    public struct Response {
        var status: uint;
        var status_text: std::string;
        var proto: std::string;
        var headers: HeaderMap;
        var body_len: usize;
        var body: Body;

        @constructor func constructor() {
            return Response {
                status = 200u;
                status_text = std::string();
                proto = std::string();
                headers = HeaderMap();
                body_len = 0u;
                body : Body()
            }
        }
    }
}
