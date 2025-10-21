public namespace net {
    public type Socket = usize;

    // Common constants
    comptime const AF_INET = 2
    comptime const AF_INET6 = 10
    comptime const SOCK_STREAM = 1
    comptime const IPPROTO_TCP = 6
    comptime const AI_PASSIVE = 1

    // timeval struct for setsockopt SO_RCVTIMEO
    public struct timeval { var tv_sec: long; var tv_usec: long }

    if(def.windows) {
        // WinSock2 minimal declarations. You might need to link -lws2_32 when building.
        @dllimport @stdcall @extern public func WSAStartup(wVersionRequested: ushort, lpWSAData: *mut char): int;
        @dllimport @stdcall @extern public func WSACleanup(): int;
        @dllimport @stdcall @extern public func WSAGetLastError(): int;
        // @dllimport @stdcall @extern public func FormatMessageA(dwFlags: uint, lpSource:*void, dwMessageId:uint, dwLanguageId:uint, lpBuffer:*mut char, nSize:uint, Arguments:*void): uint;
        @dllimport @stdcall @extern public func socket(af:int, typ:int, protocol:int): uintptr_t;
        @dllimport @stdcall @extern public func bind(s: uintptr_t, name:*char, namelen:int): int;
        @dllimport @stdcall @extern public func listen(s: uintptr_t, backlog:int): int;
        @dllimport @stdcall @extern public func accept(s: uintptr_t, addr:*mut char, addrlen:*mut int): uintptr_t;
        @dllimport @stdcall @extern public func recv(s: uintptr_t, buf:*mut char, len:int, flags:int): int;
        @dllimport @stdcall @extern public func send(s: uintptr_t, buf:*char, len:int, flags:int): int;
        @dllimport @stdcall @extern public func closesocket(s: uintptr_t): int;
        @dllimport @stdcall @extern public func setsockopt(s: uintptr_t, level:int, optname:int, optval:*char, optlen:int): int;
        @dllimport @stdcall @extern public func getaddrinfo(node:*char, service:*char, hints:*mut char, res:*mut *mut char): int;
        @dllimport @stdcall @extern public func freeaddrinfo(res:*mut char): void;

        // BOOL TransmitFile(SOCKET hSocket, HANDLE hFile, DWORD nNumberOfBytesToWrite, DWORD nNumberOfBytesPerSend,
        //                    LPOVERLAPPED lpOverlapped, LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers, DWORD dwFlags);
        @dllimport
        @stdcall
        @extern
        public func TransmitFile(
            hSocket: uintptr_t,
            hFile: uintptr_t,
            nNumberOfBytesToWrite: u32,
            nNumberOfBytesPerSend: u32,
            lpOverlapped:*void,
            lpTransmitBuffers:*void,
            dwFlags: u32
        ) : int;

        // constants used
        comptime const GENERIC_READ = 0x80000000u;
        comptime const FILE_SHARE_READ = 0x00000001u;
        comptime const OPEN_EXISTING = 3u;

        public func startup() { var dummy : [32]char; WSAStartup(0x202 as ushort, &mut dummy[0]); }
        public func cleanup() { WSACleanup() }

        // wrappers
        public func sock_socket(af:int, typ:int, proto:int) : Socket { return socket(af, typ, proto) as Socket }
        public func sock_bind(s:Socket, addr:*char, addrlen:int) : int { return bind(s as uintptr_t, addr, addrlen) }
        public func sock_listen(s:Socket, backlog:int) : int { return listen(s as uintptr_t, backlog) }
        public func sock_accept(s:Socket, addr:*mut char, addrlen:*mut int) : Socket { return accept(s as uintptr_t, addr, addrlen) as Socket }
        public func sock_recv(s:Socket, buf:*mut char, len:int) : int { return recv(s as uintptr_t, buf, len, 0) }
        public func sock_send(s:Socket, buf:*char, len:int) : int { return send(s as uintptr_t, buf, len, 0) }
        public func sock_close(s:Socket) { closesocket(s as uintptr_t) }
        public func sock_setsockopt(s:Socket, level:int, optname:int, optval:*char, optlen:int) : int { return setsockopt(s as uintptr_t, level, optname, optval, optlen) }
        public func sock_getaddrinfo(node:*char, service:*char, hints:*mut char, res:*mut *mut char) : int { return getaddrinfo(node, service, hints, res) }
        public func sock_freeaddrinfo(res:*mut char) { freeaddrinfo(res) }

        // Optionally define SOCK_CLOEXEC etc later
    } else {
        // POSIX declarations
        @extern public func socket(af:int, typ:int, protocol:int): int;
        @extern public func bind(s: int, name:*char, namelen:int): int;
        @extern public func listen(s: int, backlog:int): int;
        @extern public func accept(s: int, addr:*mut char, addrlen:*mut int): int;
        @extern public func recv(s: int, buf:*mut char, len:int, flags:int): int;
        @extern public func send(s: int, buf:*char, len:int, flags:int): int;
        @extern public func close(fd: int): int;
        @extern public func setsockopt(s: int, level:int, optname:int, optval:*char, optlen:int): int;
        @extern public func getaddrinfo(node:*char, service:*char, hints:*mut char, res:*mut *mut char): int;
        @extern public func freeaddrinfo(res:*mut char): void;

        // file I/O + sendfile
        @extern public func open(path:*char, flags:int, mode:int): int;
        @extern public func close(fd:int): int;
        @extern public func lseek(fd:int, offset: longlong, whence:int): longlong;
        // Linux sendfile: ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count);
        @extern public func sendfile(out_fd:int, in_fd:int, offset:*longlong, count:size_t) : longlong;

        // constants for open/lseek
        comptime const O_RDONLY = 0;
        comptime const SEEK_SET = 0;
        comptime const SEEK_END = 2;

        public func startup() { }
        public func cleanup() { }

        public func sock_socket(af:int, typ:int, proto:int) : Socket { return socket(af, typ, proto) as Socket }
        public func sock_bind(s:Socket, addr:*char, addrlen:int) : int { return bind(s as int, addr, addrlen) }
        public func sock_listen(s:Socket, backlog:int) : int { return listen(s as int, backlog) }
        public func sock_accept(s:Socket, addr:*mut char, addrlen:*mut int) : Socket { return accept(s as int, addr, addrlen) as Socket }
        public func sock_recv(s:Socket, buf:*mut char, len:int) : int { return recv(s as int, buf, len, 0) }
        public func sock_send(s:Socket, buf:*char, len:int) : int { return send(s as int, buf, len, 0) }
        public func sock_close(s:Socket) { close(s as int) }
        public func sock_setsockopt(s:Socket, level:int, optname:int, optval:*char, optlen:int) : int { return setsockopt(s as int, level, optname, optval, optlen) }
        public func sock_getaddrinfo(node:*char, service:*char, hints:*mut char, res:*mut *mut char) : int { return getaddrinfo(node, service, hints, res) }
        public func sock_freeaddrinfo(res:*mut char) { freeaddrinfo(res) }
    }

    @extern public func inet_pton(
        Family : int,
        pszAddrString : *char,
        pAddrBuf : *void
    ) : int;

    // IPv4 sockaddr structs (C layout)
    public struct in_addr { var s_addr: u32; }
    public struct sockaddr_in {
        var sin_family: u16;
        var sin_port: u16;
        var sin_addr: in_addr;
        var sin_zero: [8]char;
    }

    // helper: set socket recv timeout (seconds + microseconds)
    public func set_recv_timeout(s:Socket, secs: long, usecs: long) {
        var tv = timeval{ tv_sec: secs, tv_usec: usecs };
        // level SOL_SOCKET and optname SO_RCVTIMEO constants (platform dependent)
        const SOL_SOCKET = 1
        const SO_RCVTIMEO = 0x1006 // value often different; if incorrect, tests will fail — we can adjust per platform
        sock_setsockopt(s, SOL_SOCKET as int, SO_RCVTIMEO as int, ( &mut tv ) as *char, sizeof(timeval) as int);
    }

    public func htons_port(p: u16) : u16 {
        // portable byte-swap for 16-bit
        return ((p & 0x00FFu) << 8) as u16 | ((p & 0xFF00u) >> 8) as u16;
    }

    // listen by address string and port using getaddrinfo -> bind -> listen loop
    public func listen_addr(addr_str:*char, port: uint) : Socket {
        startup();
        // create socket
        var s = sock_socket(AF_INET as int, SOCK_STREAM as int, IPPROTO_TCP as int);
        if (s == 0 as Socket) {
            panic("socket() failed");
        }

        // prepare sockaddr_in
        var addr = sockaddr_in{
            sin_family: (AF_INET as u16),
            sin_port: htons_port(port as u16),
            sin_addr: in_addr{ s_addr: 0u },
            sin_zero: ['\0','\0','\0','\0','\0','\0','\0','\0']
        };

        // convert textual IP to binary -- if addr_str is null or "0.0.0.0" we set INADDR_ANY
        if(addr_str == null) {
            // INADDR_ANY = 0.0.0.0 -> s_addr already 0
        } else {
            // note: inet_pton expects AF_INET and pointer to in_addr (or out buffer)
            var ret = inet_pton(AF_INET as int, addr_str, &addr.sin_addr.s_addr as *mut char);
            if(ret != 1) {
                // inet_pton returns 1 on success; 0 for invalid, -1 for error
                // fall back to INADDR_ANY if addr was unspecified like "0.0.0.0"
                // but report error for diagnostics
                // (optional debug - remove #if when enabling)
                perror("inet_pton failed");
                // let it remain INADDR_ANY (s_addr = 0)
                addr.sin_addr.s_addr = 0u;
            }
        }

        // setsockopt SO_REUSEADDR
        const SOL_SOCKET = 1;
        const SO_REUSEADDR = 2;
        var optval: int = 1;
        var setr = sock_setsockopt(s, SOL_SOCKET as int, SO_REUSEADDR as int, &optval as *char, sizeof(int) as int);
        if(setr != 0) {
            // non-fatal; continue but log
            comptime if (def.windows) {
                var e = WSAGetLastError();
                // You can format e into string with FormatMessageA if needed (omitted here).
                // For now we keep going.
            } else {
                perror("setsockopt(SO_REUSEADDR) failed");
            }
        }

        // bind
        var bind_r = sock_bind(s, &addr as *char, sizeof(sockaddr_in) as int);
        if(bind_r != 0) {
            // report platform error and close socket
            comptime if (def.windows) {
                var e = WSAGetLastError();
                // simplest: panic with code
                panic("bind() failed (WSAGetLastError): code");
            } else {
                perror("bind() failed");
            }
            sock_close(s);
            panic("bind failed");
        }

        // listen
        if(sock_listen(s, 128) != 0) {
            comptime if(def.windows) {
                var e = WSAGetLastError();
                panic("listen() failed (WSA error)");
            } else {
                perror("listen() failed");
            }
            sock_close(s);
            panic("listen failed");
        }

        // success
        return s;
    }

    public func accept_socket(listen_sock: Socket) : Socket {
        // pass both NULL when we don't want peer address information
        return sock_accept(listen_sock, null, null);
    }

    public func recv_all(s: Socket, buf: *mut u8, cap: usize) : int {
        // helper to read up to cap bytes (single syscall)
        return sock_recv(s, buf as *mut char, cap as int)
    }

    public func send_all(s: Socket, p:*char, len:int) {
        var off = 0;
        while(off < len) {
            var n = sock_send(s, &p[off], len - off);
            if(n <= 0) { break }
            off = off + n;
        }
    }

    public func close_socket(s: Socket) { sock_close(s) }
}

// ===== Buffer implementation =====
public namespace io {
    public struct Buffer {
        var v: std::vector<u8>;
        var read_pos: usize; // how many bytes consumed from the front

        @constructor func constructor() {
            v = std::vector<u8>();
            read_pos = 0u;
        }

        func len(&self) : usize { return if(v.size() >= read_pos) v.size() - read_pos else 0u }

        func ensure_capacity(&mut self, extra: usize) {
            var need = (v.size() - read_pos) + extra;
            if(v.capacity() < need) { v.reserve(need * 2) }
        }

        func append_bytes(&mut self, src:*u8, n: usize) {
            var i = 0u;
            while(i < n) { v.push_back(src[i]); i = i + 1u }
        }

        func consume(&mut self, n: usize) {
            // simple consume: advance read_pos, and compact vector if large
            read_pos = read_pos + n;
            if(read_pos > 4096 && read_pos * 2 > v.size()) {
                // compact
                var new_v = std::vector<u8>();
                var i = read_pos;
                while(i < v.size()) { new_v.push_back(v.get(i)); i = i + 1u }
                v = new_v;
                read_pos = 0u;
            }
        }

        func as_ptr(&self) : *u8 {
            if(v.size() == 0) { return null }
            return &v.get_ptr(read_pos)[0]
        }

        func get_byte(&self, idx: usize) : u8 { return v.get(read_pos + idx) }
    }
}

// ===== HTTP utilities and parser =====
public namespace http {

    public const DEFAULT_READ_BUF = 8192u;

    public struct HeaderMap {

        var headers: std::vector<std::pair<std::string,std::string>>;

        func empty(&self) : bool {
            return headers.empty()
        }

        func print(&self) {
            var start = headers.data()
            const end = start + headers.size()
            printf("headers:\n");
            while(start != end) {
                printf("\theader '%s' : '%s'\n", start.first.data(), start.second.data());
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
                var p = *headers.get_ptr(i);
                // case-insensitive compare using manual loop
                if(strcasecmp(p.first.data(), name) == 0) { return std::Option.Some<std::string>(p.second) }
                i = i + 1u;
            }
            return std::Option.None<std::string>();
        }

    }

    // helper: simple ASCII case-insensitive strcmp
    public func strcasecmp(a:*char, b:*char) : int {
        var i = 0;
        while(true) {
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
    // It borrows a pointer to an io::Buffer containing already-read bytes (headers + maybe some body).
    @direct_init
    public struct Body {

        var sock: net::Socket;               // underlying socket (do not close here)
        var buf: *mut io::Buffer;            // pointer to buffer that holds bytes already read
        var remaining: isize;                // -1 meaning unknown (no Content-Length), otherwise bytes left
        var chunked: bool;                   // transfer-encoding: chunked
        var timeout_secs: long;              // timeout for subsequent recv calls
        var max_body: usize;                 // configured max body size (for limits)
        var closed: bool;                    // has the handler closed this Body?
        // chunked state
        var cur_chunk_left: usize;           // bytes left in current chunk when chunked==true
        var seen_total: usize;               // how many body bytes have been delivered to user (enforce max_body)

        @make
        func empty_make() {
            sock = 0
            buf = null
            remaining = -1
            chunked = false
            timeout_secs = 0
            max_body = 0
            closed = false
            cur_chunk_left = 0
            seen_total = 0
        }

        // construct body from request metadata (call from handle_conn after parsing headers)
        public func make_body(
            sock: net::Socket,
            buf_ptr: *mut io::Buffer,
            content_len: isize,    // -1 if unknown
            chunked: bool,
            timeout_secs: long,
            max_body: usize
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
                seen_total: 0u
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
        // copy bytes
        var i = 0u;
        var srcp = bufref.as_ptr();
        while(i < take) {
            dst[i] = srcp[i];
            i = i + 1u;
        }
        bufref.consume(take);
        return take;
    }

    // PUBLIC: read up to `cap` bytes into dst; returns number of bytes read or <=0 on error.
    // This function supports Content-Length and chunked transfer (basic).
    func (b: &mut Body) read(dst:*mut u8, cap: usize) : int {
        if(b.closed) { return 0 } // EOF for closed
        // Enforce max_body
        if(b.max_body > 0u && b.seen_total >= b.max_body) { return -1 } // too large
        // If chunked mode -> decode chunk frames
        if(b.chunked) {
            // If current chunk has data, use it first
            if(b.cur_chunk_left > 0u) {
                var want = if(cap < b.cur_chunk_left) cap else b.cur_chunk_left;
                // first copy from internal already-read buffer
                var copied = copy_from_buffer(&mut b, dst, want);
                var total_read = copied;
                // if need more and socket provides, recv more
                while(total_read < want) {
                    var n = body_recv(&mut b, &mut dst[total_read], want - total_read);
                    if(n <= 0) { return -1 }
                    total_read = total_read + (n as usize);
                }
                b.cur_chunk_left = b.cur_chunk_left - want;
                b.seen_total = b.seen_total + want;
                if(b.max_body > 0u && b.seen_total > b.max_body) { return -1 }
                // if chunk finished, consume trailing CRLF from stream/buffer
                if(b.cur_chunk_left == 0u) {
                    // consume trailing \r\n after chunk data
                    var tmp : [2]u8;
                    // try to copy from buffer first
                    var c = copy_from_buffer(&mut b, &mut tmp[0], 2);
                    if(c < 2) {
                        var rem = 2 - c;
                        var n = body_recv(&mut b, &mut tmp[c], rem);
                        if(n <= 0) { return -1 }
                    }
                    // ignore values, just discard
                }
                return (want as int);
            }
            // Need to read next chunk size line: it's ASCII hex then CRLF.
            // We'll read bytes from buffer and socket until we find CRLF.
            var linebuf = std::string::empty_str();
            // first consume from buffer if present
            if(b.buf != null && b.buf.len() > 0u) {
                // attempt to read until CRLF appears in buffer
                var i = 0u;
                while(i + 1 < b.buf.len()) {
                    var a = b.buf.get_byte(i);
                    var bb = b.buf.get_byte(i + 1u);
                    if(a == '\r' as u8 && bb == '\n' as u8) {
                        // read the line from buffer
                        var nline = i; // number of bytes before CRLF
                        var ptr = b.buf.as_ptr();
                        var tmpline = std::string::constructor(ptr as *char, nline as size_t);
                        b.buf.consume(nline + 2u); // remove line + CRLF
                        linebuf = tmpline;
                        break;
                    }
                    i = i + 1u;
                }
            }
            // if linebuf still empty, read bytes from socket until CRLF
            while(linebuf.size() == 0u) {
                var tmp : [64]u8;
                var n = body_recv(&mut b, &mut tmp[0], 64);
                if(n <= 0) { return -1 }
                // append into a small temporary string searching for CRLF
                var i = 0;
                while(i < n) {
                    var ch = tmp[i];
                    linebuf.append(ch as char);
                    var L = linebuf.size();
                    if(L >= 2 && linebuf.get(L-2u) == '\r' && linebuf.get(L-1u) == '\n') {
                        // strip trailing CRLF
                        linebuf = linebuf.substring(0u, L-2u);
                        break;
                    }
                    i = i + 1;
                }
            }
            // parse hex chunk size
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
                // final chunk: consume optional trailer until blank line
                // for simplicity, consume bytes until we see "\r\n\r\n"
                var found = false;
                while(!found) {
                    // check buffer first
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
                        var tmp : [DEFAULT_READ_BUF]u8;
                        var n = body_recv(&mut b, &mut tmp[0], DEFAULT_READ_BUF);
                        if(n <= 0) { return -1 }
                        // append to buffer for next iteration
                        if(b.buf != null) { b.buf.append_bytes(&mut tmp[0], n as usize) }
                    }
                }
                // EOF for chunked body
                b.closed = true;
                return 0;
            }
            // set current chunk left and loop to read from it (recursive call)
            b.cur_chunk_left = hex_val;
            // recursive to read from cur_chunk_left portion
            return b.read(dst, cap);
        } // chunked end

        // Non-chunked simple: Content-Length known or unknown.
        if(b.remaining >= 0) {
            // known length: read up to min(cap, remaining)
            var want = if((b.remaining as usize) < cap) (b.remaining as usize) else cap;
            // copy from buffer first
            var copied = copy_from_buffer(&mut b, dst, want);
            var total = copied;
            while(total < want) {
                var n = body_recv(&mut b, &mut dst[total], want - total);
                if(n <= 0) { return -1 }
                total = total + (n as usize);
            }
            b.remaining = b.remaining - (total as isize);
            b.seen_total = b.seen_total + total;
            if(b.max_body > 0u && b.seen_total > b.max_body) { return -1 }
            return (total as int);
        } else {
            // unknown length: read until peer closes (connection: close) — treat as stream
            // copy from buffer first
            var copied = copy_from_buffer(&mut b, dst, cap);
            var total = copied;
            if(total > 0u) {
                b.seen_total = b.seen_total + total;
                if(b.max_body > 0u && b.seen_total > b.max_body) { return -1 }
                return (total as int);
            }
            // otherwise, read from socket
            var n = body_recv(&mut b, dst, cap);
            if(n <= 0) { return (0) } // EOF or error -> return 0 for EOF
            b.seen_total = b.seen_total + (n as usize);
            if(b.max_body > 0u && b.seen_total > b.max_body) { return -1 }
            return n;
        }
    }

    // read all remaining body into a string (useful for small bodies)
    public func (b: &mut Body) read_to_string() : std::Option<std::string> {
        var out = std::string::empty_str();
        var tmp : [DEFAULT_READ_BUF]u8;
        while(true) {
            var n = b.read(&mut tmp[0], DEFAULT_READ_BUF);
            if(n < 0) { return std::Option.None<std::string>() } // error
            if(n == 0) { break } // EOF
            out.append_view(std::string_view(&tmp[0] as *char, n as size_t))
        }
        return std::Option.Some<std::string>(out);
    }

    // read exactly `n` bytes (or return None on error/EOF)
    public func (b: &mut Body) read_exact(n: usize) : std::Option<std::string> {
        var out = std::string::empty_str();
        out.reserve(n);
        var rem = n;
        var tmp : [DEFAULT_READ_BUF]u8;
        while(rem > 0u) {
            var want = if(rem < DEFAULT_READ_BUF) rem else DEFAULT_READ_BUF;
            var r = b.read(&mut tmp[0], want);
            if(r <= 0) { return std::Option.None<std::string>() }
            var part = std::string::constructor(&tmp[0] as *char, r as size_t);
            out.append_string(part);
            rem = rem - (r as usize);
        }
        return std::Option.Some<std::string>(out);
    }

    // drain remaining body to /dev/null (useful when you want to discard the body)
    public func (b: &mut Body) drain() : bool {
        var tmp : [DEFAULT_READ_BUF]u8;
        while(true) {
            var n = b.read(&mut tmp[0], DEFAULT_READ_BUF);
            if(n < 0) { return false }
            if(n == 0) { break }
        }
        return true;
    }

    // close returns resources (no-op here except mark closed)
    public func (b: &mut Body) close() {
        b.closed = true;
    }

    public struct Request {
        var method: std::string;
        var path: std::string;
        var proto: std::string;
        var headers: HeaderMap;
        var body_len: usize;
        var remote: std::string;
        var body : Body

        @constructor func constructor() {
            method = std::string::empty_str();
            path = std::string::empty_str();
            proto = std::string::empty_str();
            headers = HeaderMap();
            body_len = 0u;
            remote = std::string::empty_str();
        }

    }

    public struct ResponseWriter {
        var sock: net::Socket;
        var headers: HeaderMap;
        var status: uint;
        var sent_headers: bool;

        @constructor func constructor(s: net::Socket) {
            sock = s;
            status = 200u;
            sent_headers = false
        }

        func set_header(&mut self, k: std::string, v: std::string) { headers.insert(k, v) }
        func set_header_view(&mut self, k : &std::string_view, v : &std::string_view) {
            headers.insert_view(k, v)
        }
        func send_headers(&mut self, content_len: usize) {
            if(sent_headers) { return }
            var out = std::string::empty_str();
            // Status line (HTTP requires CRLF)
            out.append_view(std::string_view("HTTP/1.1 "));
            var st = std::string::empty_str(); st.append_uinteger(status);
            out.append_string(st);
            out.append_view(std::string_view(" OK\r\n"));

            // Server header
            out.append_view(std::string_view("Server: chemical-http\r\n"));

            // Content-Length
            var clen = std::string::empty_str(); clen.append_uinteger(content_len as ubigint);
            out.append_view(std::string_view("Content-Length: "));
            out.append_string(clen);
            out.append_view(std::string_view("\r\n"));

            // Other headers
            var i = 0u;
            while(i < headers.headers.size()) {
                var p = headers.headers.get_ptr(i);
                out.append_string(p.first);
                out.append_view(std::string_view(": "));
                out.append_string(p.second);
                out.append_view(std::string_view("\r\n"));
                i = i + 1u;
            }

            // End of header section
            out.append_view(std::string_view("\r\n"));

            net::send_all(sock, out.data(), out.size() as int);
            sent_headers = true;
        }
        func write_string(&mut self, s: std::string) { send_headers(s.size()); net::send_all(sock, s.data(), s.size() as int) }
        func write_view(&mut self, v : &std::string_view) {
            send_headers(v.size()); net::send_all(sock, v.data(), v.size() as int)
        }

        // Adds a zero-copy send_file method to ResponseWriter.
        // Returns true on success; false means the caller should fallback to chunked streaming.
        func send_file(&mut self, path_view : &std::string_view, content_type : &std::string_view) : bool {
            // set content-type header if provided
            set_header_view(std::string_view("Content-Type"), content_type);
            const path = path_view.data()
            comptime if(def.windows) {
                // Windows: use CreateFileA + GetFileSizeEx + TransmitFile
                var fh = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, null, OPEN_EXISTING, 0u, 0 as HANDLE);
                if (fh == 0u || fh == -1 as uintptr_t) {
                    return false;
                }
                var filesize: longlong = 0
                if (GetFileSizeEx(fh, (&mut filesize) as *mut LARGE_INTEGER) == 0) {
                    CloseHandle(fh);
                    return false;
                }
                // send headers with Content-Length
                self.send_headers(filesize as usize);

                // TransmitFile: pass 0 as nNumberOfBytesToWrite means send entire file
                var ok = net::TransmitFile(self.sock as uintptr_t, fh as uintptr_t, 0u, 0u, null, null, 0u);
                // Close handle regardless
                CloseHandle(fh);
                return ok != 0;
            } else {
                // POSIX / Linux: use open + lseek + sendfile
                var fd = open(path, O_RDONLY, 0);
                if (fd < 0) { return false; }
                var size_ll = net::lseek(fd, 0 as longlong, SEEK_END);
                if (size_ll < 0) { close(fd); return false; }
                // rewind
                if (net::lseek(fd, 0 as longlong, SEEK_SET) < 0) { close(fd); return false; }

                // send headers with Content-Length
                self.send_headers((size_ll as usize));

                var remaining = size_ll;
                var offset: longlong = 0;
                while (remaining > 0) {
                    // send up to remaining bytes
                    var to_send = if(remaining > (1 << 30)) (1 << 30) as size_t else remaining as size_t;
                    // on Linux sendfile returns number of bytes sent (ssize_t)
                    var sent = net::sendfile(self.sock as int, fd, &mut offset, to_send);
                    if (sent <= 0) {
                        // error or EOF
                        break;
                    }
                    // adjust remaining - note: sendfile may update offset; we'll recompute
                    remaining = size_ll - offset;
                }

                var success = (offset == size_ll);
                close(fd);
                return success;
            }
        }



        func finish(&mut self) {}
    }

    // URL decode (percent-decoding)
    public func url_decode(in_s: &std::string) : std::string {
        var out = std::string::empty_str();
        var i = 0u;
        while(i < in_s.size()) {
            var c = in_s.get(i);
            if(c == '%') {
                if(i + 2 < in_s.size()) {
                    var hi = in_s.get(i+1u); var lo = in_s.get(i+2u);
                    var hv = hex_value(hi); var lv = hex_value(lo);
                    if(hv >= 0 && lv >= 0) {
                        var val = (hv * 16 + lv) as int;
                        out.append(val as char);
                        i = i + 3u;
                        continue;
                    }
                }
                // fallthrough: append '%'
                out.append('%'); i++;
            } else if(c == '+') { out.append(' '); i++ } else { out.append(c); i++ }
        }
        return out;
    }
    public func hex_value(c: char) : int { if(c >= '0' && c <= '9') return (c - '0') as int; if(c >= 'A' && c <= 'F') return (c - 'A' + 10) as int; if(c >= 'a' && c <= 'f') return (c - 'a' + 10) as int; return -1 }

    // Parse query string into simple vector of pairs
    public func parse_query(qs: &std::string) : std::vector<std::pair<std::string,std::string>> {
        var out = std::vector<std::pair<std::string,std::string>>();
        var i = 0u; var start = 0u;
        while(i <= qs.size()) {
            if(i == qs.size() || qs.get(i) == '&') {
                // substring start..i
                var eq = start;
                var found = false;
                while(eq < i) { if(qs.get(eq) == '=') { found = true; break } eq++ }
                if(found) {
                    var k = qs.substring(start, eq);
                    var v = qs.substring(eq + 1u, i);
                    out.push_back(std::pair<std::string,std::string>{ first : url_decode(k), second : url_decode(v) });
                } else if(i > start) {
                    var k2 = qs.substring(start, i);
                    out.push_back(std::pair<std::string,std::string>{ first : url_decode(k2), second : std::string::empty_str() });
                }
                start = i + 1u;
            }
            i++;
        }
        return out;
    }

    // Incremental request reader that reads headers with timeout and enforces limits
    public func read_request_incremental(s: net::Socket, buf: &mut io::Buffer, header_timeout_secs: long, max_header_bytes: usize, max_headers: uint) : std::Option<Request> {
        // set recv timeout
        net::set_recv_timeout(s, header_timeout_secs, 0);
        // read loop
        while(true) {
            // check for CRLFCRLF in buffer (i.e. "\r\n\r\n")
            var i = 0u; var found = false; var crlfpos = 0u;
            while(i + 3 < buf.len()) {
                if(buf.get_byte(i) == '\r' as u8 && buf.get_byte(i+1u) == '\n' as u8 &&
                   buf.get_byte(i+2u) == '\r' as u8 && buf.get_byte(i+3u) == '\n' as u8) {
                    found = true;
                    crlfpos = i + 4u;
                    break;
                }
                i++;
            }
            if(found) {
                // parse request-line and headers from buf.as_ptr()
                var ptr = buf.as_ptr();
                var req_opt = parse_request_from_bytes(ptr, crlfpos, s);
                if(req_opt is std::Option.Some) {
                    var Some(req) = req_opt else unreachable;
                    // consume header bytes
                    buf.consume(crlfpos);
                    return std::Option.Some<Request>(req);
                } else { return std::Option.None<Request>() }
            }
            // no full header yet: read more
            if(buf.len() > max_header_bytes) { return std::Option.None<Request>() }
            var tmp : [DEFAULT_READ_BUF]u8;
            var n = net::recv_all(s, &mut tmp[0], DEFAULT_READ_BUF);
            if(n <= 0) { return std::Option.None<Request>() }
            buf.append_bytes(&mut tmp[0], n as usize);
            if(buf.len() > max_header_bytes) { return std::Option.None<Request>() }
            // loop to check again
        }
    }

    // parse_request_from_bytes: same as earlier parse_request_from_buf but uses raw bytes and returns Request
    public func parse_request_from_bytes(buf:*u8, n: usize, s: net::Socket) : std::Option<Request> {
        // find the first CRLF (end of request-line)
        var crlf_pos = -1;
        var j = 0u;
        while(j + 1 < n) {
            if(buf[j] == '\r' as u8 && buf[j+1] == '\n' as u8) { crlf_pos = j as int; break }
            j++;
        }
        if(crlf_pos == -1) { return std::Option.None<Request>() }

        // find spaces in request-line (method SP path SP proto)
        var sp1 = -1; var sp2 = -1; var k = 0u;
        while(k < (crlf_pos as usize)) {
            if(buf[k] == ' ' as u8 && sp1 == -1) { sp1 = k as int }
            else if(buf[k] == ' ' as u8 && sp2 == -1) { sp2 = k as int; break }
            k++
        }
        if(sp1 == -1 || sp2 == -1) { return std::Option.None<Request>() }

        var req = Request();
        req.method = std::string::constructor((buf + 0) as *char, (sp1 as usize) as size_t);
        req.path = std::string::constructor((buf + (sp1 as usize + 1)) as *char, ((sp2 as usize) - (sp1 as usize) - 1) as size_t);
        req.proto = std::string::constructor((buf + (sp2 as usize + 1)) as *char, ((crlf_pos as usize) - (sp2 as usize) - 1) as size_t);

        // headers parse: iterate lines separated by CRLF until empty line
        var line_start = (crlf_pos as usize) + 2; // skip the first CRLF
        var header_count = 0u;
        while(line_start + 1 < n) {
            // find end of this header line (CRLF)
            var line_end = line_start;
            while(line_end + 1 < n) {
                if(buf[line_end] == '\r' as u8 && buf[line_end+1] == '\n' as u8) { break }
                line_end = line_end + 1
            }
            if(line_end >= n) { break }
            // if empty line (i.e. CRLF right away) -> end of headers
            if(line_end == line_start) { break }

            // find ':' in header line
            var colon = -1; var p = line_start;
            while(p < line_end) {
                if(buf[p] == ':' as u8) { colon = p as int; break }
                p = p + 1
            }
            if(colon != -1) {
                var name = std::string::constructor((buf + line_start) as *char, ((colon as usize) - line_start) as size_t);
                var vstart = (colon as usize) + 1;
                if(vstart < line_end && buf[vstart] == ' ' as u8) { vstart = vstart + 1 }
                var value = std::string::constructor((buf + vstart) as *char, (line_end - vstart) as size_t);
                req.headers.insert(name, value);
                header_count = header_count + 1u;
            }

            line_start = line_end + 2; // advance past CRLF
            if(header_count > 512u) { return std::Option.None<Request>() }
        }

        // Content-Length if present
        var cl_opt = req.headers.get("Content-Length");
        if(cl_opt is std::Option.Some) {
            var Some(v) = cl_opt else unreachable;
            var val = 0u; var ii = 0u;
            while(ii < v.size()) { var c = v.get(ii); if(c >= '0' && c <= '9') { val = val * 10u + (c as usize - '0' as usize) } ii = ii + 1u }
            req.body_len = val;
        }

        // remote address placeholder
        req.remote = std::string::make_no_len("127.0.0.1" as *char);
        return std::Option.Some<Request>(req);
    }
}

// ===== Router + middleware =====
public namespace web {
    public type Handler = std.function<(req: http.Request, res: http.ResponseWriter) => void>
    public type Middleware = std.function<(next: Handler) => Handler>

    public struct Route {
        var method: std::string;
        var pattern: std::string; // e.g. /users/:id
        var handler: Handler;
    }

    public struct Router {
        var routes: std::vector<Route>;
        var middlewares: std::vector<Middleware>;
        @constructor func constructor() { routes = std::vector<Route>(); middlewares = std::vector<Middleware>(); }

        func add(&mut self, method: *char, pattern: *char, h: Handler) {
            routes.push_back(Route{ method: std::string::make_no_len(method), pattern: std::string::make_no_len(pattern), handler: h });
        }

        func use_middleware(&mut self, m: Middleware) { middlewares.push_back(m) }

        func match_route(&self, method: &std::string, path: &std::string, params_out: *mut std::vector<std::pair<std::string,std::string>>) : std::Option<Handler> {
            var i = 0u;
            while(i < routes.size()) {
                var r = *routes.get_ptr(i);
                if(r.method.equals_with_len(method.data(), method.size())) {
                    // match pattern vs path
                    if(match_pattern(r.pattern, path, params_out)) { return std::Option.Some<Handler>(r.handler) }
                }
                i = i + 1u;
            }
            return std::Option.None<Handler>();
        }

        // apply middlewares to a handler and return final handler
        func apply_middlewares(&self, base: Handler) : Handler {
            return base;
            /** TODO:
            var h = base;
            var i = (middlewares.size());
            while(i > 0u) {
                i = i - 1u;
                var m = *middlewares.get_ptr(i);
                h = m(h);
            }
            return h;
            **/
        }

    }

    // match pattern like /users/:id against path and fill params
    public func match_pattern(pattern: &std::string, path: &std::string, params_out: *mut std::vector<std::pair<std::string,std::string>>) : bool {
        // split into segments by '/'
        var pseg = split_segments(pattern);
        var tseg = split_segments(path);
        if(pseg.size() != tseg.size()) { return false }
        var i = 0u;
        while(i < pseg.size()) {
            var ps = *pseg.get_ptr(i);
            var ts = *tseg.get_ptr(i);
            if(ps.size() > 0 && ps.get(0) == ':' ) {
                // param
                var key = ps.substring(1, ps.size());
                params_out.push_back(std::pair<std::string,std::string>{ first : key, second : ts });
            } else {
                if(!ps.equals_with_len(ts.data(), ts.size())) { return false }
            }
            i = i + 1u;
        }
        return true;
    }

    public func split_segments(s: &std::string) : std::vector<std::string> {
        var out = std::vector<std::string>();
        var i = 0u; var start = 0u;
        // skip leading '/'
        if(i < s.size() && s.get(i) == '/') { i++; start = i }
        while(i <= s.size()) {
            if(i == s.size() || s.get(i) == '/') {
                out.push_back(s.substring(start, i));
                start = i + 1u;
            }
            i = i + 1u;
        }
        return out;
    }
}

// ===== Server wiring all pieces together =====
public namespace server {
    public struct ServerConfig {
        var addr: std::string;
        var worker_count: uint;
        var header_timeout_secs: long;
        var max_header_bytes: usize;
        var max_headers: uint;
        var max_body_bytes: usize;
        @constructor func constructor() { addr = std::string::make_no_len(":8080"); worker_count = std.concurrent.hardware_threads() as uint; header_timeout_secs = 5; max_header_bytes = 64u * 1024u; max_headers = 512u; max_body_bytes = 10u * 1024u * 1024u }
    }

    public struct Server {
        var cfg: ServerConfig;
        var pool: std.concurrent.ThreadPool;
        var listen_sock: net::Socket;
        var router: web.Router;
        var run: bool;
        var accept_thread: std.concurrent.Thread;

        @constructor func constructor(cfg_: ServerConfig) {
            cfg = cfg_;
            // pool = std.concurrent.create_pool(cfg.worker_count);
            new (&pool) std.concurrent.create_pool(cfg.worker_count);
            listen_sock = 0u;
            router = web.Router();
            run = false;
            accept_thread = std.concurrent.spawn(||() => null, null as *void); // dummy
        }

        // production handler: parse request, route, and respond
        func handle_conn(&self, s: net::Socket) {
            printf("handle_conn: start socket=%d\n", s);
            if (s == 0u || (s as longlong) < 0) {
                printf("handle_conn: invalid socket {}\n");
                net::close_socket(s); return;
            }

            var buf = io.Buffer();
            var req_opt = http.read_request_incremental(s, buf, self.cfg.header_timeout_secs, self.cfg.max_header_bytes, self.cfg.max_headers);
            if (req_opt is std::Option.None) {
                printf("handle_conn: read_request_incremental -> None for socket={}\n");
                net::close_socket(s); return;
            }
            printf("handle_conn: parsed request for socket=%d\n", s);
            var Some(req) = req_opt else unreachable;

            // do NOT read the body yet — create a lazy Body and give it to the handler via req.body_source
            var body_len: isize = -1;
            var chunked = false;
            // set body_len from req.body_len (was parsed from Content-Length), else -1
            if(req.body_len > 0u) { body_len = req.body_len as isize; }
            // detect chunked
            var te_opt = req.headers.get("Transfer-Encoding");
            if(te_opt is std::Option.Some) {
                var Some(te) = te_opt else unreachable;
                if(te.equals_with_len("chunked", 7)) { chunked = true; body_len = -1; }
            }

            // attach body source to request (we avoid copying the buffer: pass pointer)
            req.body = http.Body.make_body(s, &mut buf as *mut io::Buffer, body_len, chunked, self.cfg.header_timeout_secs * 4, self.cfg.max_body_bytes);

            printf("handle_conn: method='%s' path='%s'\n", req.method.data(), req.path.data());

            var params = std::vector<std::pair<std::string,std::string>>();
            var hopt = self.router.match_route(req.method, req.path, &mut params);

            var resw = http.ResponseWriter(s);

            printf("handle_conn: total headers in response writer = %d\n", resw.headers.headers.size());

            if (hopt is std::Option.Some) {
                var Some(handler) = hopt else unreachable;
                printf("handle_conn: invoking handler for socket=%d\n", s);

                // call the handler with the actual Request object
                handler(req_opt.take(), resw);

                printf("handle_conn: handler returned for socket=%d\n", s);
            } else {
                printf("handle_conn: no handler matched, sending 404 for socket={}\n");
                resw.status = 404u;
                resw.set_header(std::string::make_no_len("Content-Type"), std::string::make_no_len("text/plain; charset=utf-8"));
                resw.write_string(std::string::make_no_len("Not Found\n"));
            }

            net::close_socket(s);
            printf("handle_conn: closed socket={}\n");
        }

        // accept loop — submit work to threadpool
        // TEMP: accept inline to isolate threadpool / closure issues
        func accept_main(arg : *void) : *void {
            var S = arg as *mut Server;
            printf("accept_main: starting\n");
            while (S.run) {
                var s = net.accept_socket(S.listen_sock);
                if (s == 0u || (s as longlong) < 0) {
                    printf("accept_main: accept failed or would-block socket={}\n");
                    std.concurrent.sleep_ms(1u);
                    continue;
                }
                printf("accept_main: accepted %d\n", s);

                // Call handler directly in the acceptor thread (isolate threadpool).
                S.handle_conn(s);

                printf("accept_main: finished handling {}\n");
            }
            printf("accept_main: exiting\n");
            return null;
        }

        func start(&mut self) {
            // parse addr into host and port
            // acceptor will call net.listen_addr
            self.listen_sock = net.listen_addr("0.0.0.0", 8080u);
            self.run = true;
            var th = std.concurrent.spawn(accept_main, &mut self as *void);
            accept_thread = th;
        }

        func serve(&mut self) {
            start();
            accept_thread.join();
        }

        func shutdown(&mut self) {
            run = false;
            net.close_socket(listen_sock);
        }
    }
}