// net::raw — low-level descriptor API (legacy).
//
// This is the ONLY declaration of `net::raw` in the module; nested namespaces
// cannot be reopened across files, so the raw surface lives here as one unit.
// Canonical code uses the owning `TcpListener`/`TcpStream` handles in `safe.ch`.
// Everything here speaks bare `Socket` descriptors, raw pointers, `*_async`
// fd APIs, and `try_listen_addr`/`dial` helpers; it is not memory-safe to call
// without a matching `close_socket` and a valid buffer.

public namespace net {

public namespace raw {


    public type Socket = usize;

    // Common constants
    comptime const AF_INET = 2
    comptime const AF_INET6 = 10
    comptime const SOCK_STREAM = 1
    comptime const IPPROTO_TCP = 6
    comptime const AI_PASSIVE = 1

    struct timeval { var tv_sec: long; var tv_usec: long }

    // helper: set socket recv timeout (seconds + microseconds)
    public func set_recv_timeout(s:Socket, secs: long, usecs: long) {
        const SOL_SOCKET = if(def.windows) 0xFFFF else 1
        const SO_RCVTIMEO = if(def.windows) 0x1006 else 20
        comptime if(def.windows) {
            // Windows expects DWORD timeout in milliseconds, not struct timeval
            var timeout_ms: u32 = (secs * 1000 + usecs / 1000) as u32;
            net::sock_setsockopt(s, SOL_SOCKET as int, SO_RCVTIMEO as int, &timeout_ms as *char, sizeof(u32) as int);
        } else {
            var tv = timeval{ tv_sec: secs, tv_usec: usecs };
            net::sock_setsockopt(s, SOL_SOCKET as int, SO_RCVTIMEO as int, (&mut tv) as *char, sizeof(timeval) as int);
        }
    }

    // helper: set keep alive
    public func set_keep_alive(s:Socket, enable: bool) {
        // Windows SOL_SOCKET = 0xFFFF, POSIX = 1
        const SOL_SOCKET = if(def.windows) 0xFFFF else 1
        const SO_KEEPALIVE = if(def.windows) 8 else 9
        var optval: int = if(enable) 1 else 0;
        net::sock_setsockopt(s, SOL_SOCKET as int, SO_KEEPALIVE as int, &optval as *char, sizeof(int) as int);
    }

    public func htons_port(p: u16) : u16 {
        // portable byte-swap for 16-bit
        return ((p & 0x00FFu) << 8) as u16 | ((p & 0xFF00u) >> 8) as u16;
    }

    // listen by address string and port using getaddrinfo -> bind -> listen loop
    public func try_listen_addr(addr_str:*char, port: uint) : Socket {

        net::startup();
        // create socket
        var s = net::sock_socket(AF_INET as int, SOCK_STREAM as int, IPPROTO_TCP as int);
        comptime if(def.windows) {
            // Windows returns INVALID_SOCKET (~0) on failure, which is not 0
            if (s == 0 as Socket || (s as longlong) < 0) {
                var e = unsafe(WSAGetLastError());
                unsafe { printf("socket() failed (WSAGetLastError): %d\n", e as int); }
                return 0 as Socket;
            }
        } else {
            if (s == 0 as Socket) {
                return 0 as Socket;
            }
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
            var ret = unsafe(inet_pton(AF_INET as int, addr_str, &addr.sin_addr.s_addr as *mut char));
            if(ret != 1) {
                // inet_pton returns 1 on success; 0 for invalid, -1 for error
                // fall back to INADDR_ANY if addr was unspecified like "0.0.0.0"
                // but report error for diagnostics
                // (optional debug - remove #if when enabling)
                unsafe { perror("inet_pton failed") };
                // let it remain INADDR_ANY (s_addr = 0)
                addr.sin_addr.s_addr = 0u;
            }
        }

        // setsockopt SO_REUSEADDR
        const SOL_SOCKET = if(def.windows) 0xFFFF else 1
        const SO_REUSEADDR = if(def.windows) 4 else 2
        var optval: int = 1;
        var setr = net::sock_setsockopt(s, SOL_SOCKET as int, SO_REUSEADDR as int, &optval as *char, sizeof(int) as int);
        if(setr != 0) {
            // non-fatal; continue but log
            comptime if (def.windows) {
                var e = unsafe(WSAGetLastError());
                // You can format e into string with FormatMessageA if needed (omitted here).
                // For now we keep going.
            } else {
                unsafe { perror("setsockopt(SO_REUSEADDR) failed") };
            }
        }

        // bind
        var bind_r = net::sock_bind(s, &addr as *char, sizeof(sockaddr_in) as int);
        if(bind_r != 0) {
            // report platform error and close socket
            comptime if (def.windows) {
                var e = unsafe(WSAGetLastError());
                unsafe { printf("bind() failed (WSAGetLastError): %d\n", e as int); }
            } else {
                unsafe { perror("bind() failed") };
            }
            net::sock_close(s);
            return 0 as Socket;
        }

        // listen
        if(net::sock_listen(s, 128) != 0) {
            comptime if(def.windows) {
                var e = unsafe(WSAGetLastError());
            } else {
                unsafe { perror("listen() failed") };
            }
            net::sock_close(s);
            return 0 as Socket;
        }

        // success
        return s;
    }

    public func listen_addr(addr_str : *char, port : uint) : Socket {
        var s = try_listen_addr(addr_str, port)
        if(s == 0 as Socket) {
            panic("listen failed")
        }
        return s
    }

    public func accept_socket(listen_sock: Socket) : Socket {
        // pass both NULL when we don't want peer address information
        // Windows non-blocking accept yields INVALID_SOCKET (~0) when no
        // connection is pending; normalize every failure to 0 so callers
        // checking `== 0` never mistake an error for a live socket.
        var s = net::sock_accept(listen_sock, null, null);
        if(s == 0u || ((s as longlong) < 0)) { return 0u }
        return s;
    }

    public func recv_all(s: Socket, buf: *mut u8, cap: usize) : int {
        // A null buffer is only valid for a zero-length read; the C socket
        // length is signed, so reject lengths that would wrap to negative.
        if(cap > 0 && buf == null) { return -1 }
        if(cap > 2147483647 as usize) { return -1 }
        if(cap == 0) { return 0 }
        // helper to read up to cap bytes (single syscall)
        return net::sock_recv(s, buf as *mut char, cap as int)
    }

    public func send_all(s: Socket, p:*char, len:int) : int {
        unsafe {
            if(len < 0) { return -1 }
            if(len > 0 && p == null) { return -1 }
            var off = 0;
            while(off < len) {
                var n = net::sock_send(s, &raw p[off], len - off);
                if(n <= 0) { return -1 }
                off = off + n;
            }
            return len

        }
    }

    public func close_socket(s: Socket) { net::sock_close(s) }

    public func dial(addr_str:*char, port: uint) : Socket {
        if(addr_str == null || port > 65535u) { return 0 as Socket; }
        net::startup();

        var addrinfo: *mut char = null;
        // 5 decimal digits for a validated TCP/UDP port plus a NUL.
        var port_str : [16]char;
        // manually convert port to string
        var p = port; var i = 0;
        if (p == 0) { port_str[0] = '0'; port_str[1] = '\0'; }
        else {
            var tmp : [8]char; var cnt = 0;
            while(p > 0) { tmp[cnt] = (p % 10 + '0' as uint) as char; p = p / 10; cnt = cnt + 1; }
            while(cnt > 0) { port_str[i] = tmp[cnt-1]; i = i + 1; cnt = cnt - 1; }
            port_str[i] = '\0';
        }

        var hints : [64]char; // assuming size of addrinfo
        // set ai_family=AF_INET, ai_socktype=SOCK_STREAM
        // This is fragile as struct definition is opaque.
        // Let's pass null hints for now to get any address.

        var ret = net::sock_getaddrinfo(addr_str, &raw port_str[0], null, &raw mut addrinfo);
        if(ret != 0) {

            return 0 as Socket;
        }

        // iterate addrinfo (linked list)
        var s: Socket = 0 as Socket;
        var curr = addrinfo;
        while(curr != null) {
            // Layout on 64-bit:
            // POSIX: ai_flags(4) ai_family(4) ai_socktype(4) ai_protocol(4) ai_addrlen(4) pad(4) ai_addr(8) ai_canonname(8) ai_next(8)
            //   => ai_addr at +24, ai_addrlen at +16, ai_next at +40
            // Windows: ai_flags(4) ai_family(4) ai_socktype(4) ai_protocol(4) ai_addrlen(8) ai_canonname(8) ai_addr(8) ai_next(8)
            //   => ai_addr at +32, ai_addrlen at +16, ai_next at +40
            const AI_ADDR_OFFSET = if(def.windows) 32 else 24
            var ai_addr_ptr = unsafe(*(((curr as usize) + AI_ADDR_OFFSET) as *mut *mut char));
            var ai_addrlen = unsafe(*(((curr as usize) + 16u) as *mut usize));

            s = net::sock_socket(AF_INET as int, SOCK_STREAM as int, IPPROTO_TCP as int);
            if(s != 0 as Socket && (s as longlong) >= 0) {

                if(net::sock_connect(s, ai_addr_ptr, ai_addrlen as int) == 0) {

                    net::sock_freeaddrinfo(addrinfo);
                    return s;
                }

                net::sock_close(s);
            }

            // move to next (ai_next is at +40 on both POSIX and Windows x64)
            curr = unsafe(*(((curr as usize) + 40u) as *mut *mut char));
            s = 0 as Socket;
        }


        net::sock_freeaddrinfo(addrinfo);
        return 0 as Socket;
    }




    @extern protected func inet_pton(
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



@direct_init
public struct AsyncSocket {
    var fd : int

    public func valid(&self) : bool {
        return self.fd > 0
    }

    public func raw(&self) : int {
        return self.fd
    }

    public func close(&self) {
        if(self.fd > 0) {
            close_socket(self.fd as Socket)
        }
    }

    // Await the next chunk of bytes (0 = EOF, <0 = error).
    public func read(&self, buf : *mut u8, cap : usize) : core::async::FutureHandle<'static, int> {
        return recv_async(self.fd, buf, cap)
    }

    // Await a full write of `len` bytes (returns `len`, or <0 on error).
    public func write(&self, data : *char, len : int) : core::async::FutureHandle<'static, int> {
        return send_async(self.fd, data, len)
    }

    // Await the next inbound connection; resolves with the accepted fd (>0) or 0.
    public func accept(&self) : core::async::FutureHandle<'static, int> {
        return accept_async(self.fd)
    }
}

// Wrap a raw fd.
public func async_socket(fd : int) : AsyncSocket {
    return AsyncSocket { fd : fd }
}

// Bind + listen synchronously (fast) and return an async listener.
public func async_listener(addr_str : *char, port : uint) : AsyncSocket {
    return AsyncSocket { fd : listen_addr(addr_str, port) as int }
}

// ---- connect ---------------------------------------------------------------

// Resolves with the connected fd (>0), or 0 on failure.
public func dial_async(addr_str : *char, port : uint) : core::async::FutureHandle<'static, int> {
    return unsafe(async::spawn_blocking<int>(|addr_str, port|() => {
        var s = dial(addr_str, port)
        if(s == 0 as Socket) {
            return 0
        }
        net::sock_set_nonblocking(s)
        return s as int
    }))
}

// ---- accept ----------------------------------------------------------------

// Resolves with the accepted fd (>0), or 0 on failure.
public async func accept_async(listener : int) : int {
    comptime if(def.windows) {
        var accepted = await unsafe(net::iocp::async_iocp_accept(listener as Socket))
        return accepted
    } else {
        net::sock_set_nonblocking(listener as Socket)
        var accepted : Socket = 0 as Socket
        while(accepted == 0 as Socket) {
            var ready = await async::readable(listener)
            accepted = accept_socket(listener as Socket)
        }
        net::sock_set_nonblocking(accepted)
        return accepted as int
    }
}

// ---- recv / send -----------------------------------------------------------

public async func recv_async(fd : int, buf : *mut u8, cap : usize) : int {
    if(cap > 0 && buf == null) { return -1 }
    if(cap > 2147483647 as usize) { return -1 }
    comptime if(def.windows) {
        var n = await unsafe(net::iocp::async_iocp_recv(fd as Socket, buf, cap))
        return n
    } else {
        var n = recv_all(fd as Socket, buf, cap)
        while(n < 0 && (get_errno() == EAGAIN || get_errno() == EINTR)) {
            var ready = await async::readable(fd)
            n = recv_all(fd as Socket, buf, cap)
        }
        return n
    }
}

public async func send_async(fd : int, data : *char, len : int) : int {
    if(len < 0) { return -1 }
    if(len > 0 && data == null) { return -1 }
    comptime if(def.windows) {
        var off = 0
        while(off < len) {
            var n = await unsafe(net::iocp::async_iocp_send(fd as Socket, unsafe(&raw data[off]), len - off))
            if(n <= 0) {
                return -1
            }
            off = off + n
        }
        return len
    } else {
        var off = 0
        while(off < len) {
            var n = net::sock_send(fd as Socket, unsafe(&raw data[off]), len - off)
            if(n > 0) {
                off = off + n
                continue
            }
            if(n < 0 && (get_errno() == EAGAIN || get_errno() == EINTR)) {
                var ready = await async::writable(fd)
                continue
            }
            return -1
        }
        return len
    }
}



// Owning-handle bridge for wrappers that transfer the descriptor to another
// subsystem (e.g. TLS accept). The caller becomes responsible for closing it.
public unsafe func release_tcp(stream : &mut net::TcpStream) : Socket {
    var fd = stream.fd
    stream.fd = 0 as Socket
    return fd
}

public func set_nonblocking(s : Socket) { net::sock_set_nonblocking(s) }
public func set_blocking(s : Socket) { net::sock_set_blocking(s) }

public func listener_fd(listener : &net::TcpListener) : Socket { return listener.fd }
public func stream_fd(stream : &net::TcpStream) : Socket { return stream.fd }

public func stream_from_fd(fd : Socket) : net::TcpStream { return net::TcpStream { fd : fd } }
public func listener_from_fd(fd : Socket) : net::TcpListener { return net::TcpListener { fd : fd } }

public func stream_box_from_fd(fd : Socket) : *mut net::TcpStream {
    var p = unsafe(malloc(sizeof(net::TcpStream))) as *mut net::TcpStream
    if(p == null) { return null }
    unsafe { *p = net::TcpStream { fd : 0 as Socket } }
    unsafe { p.fd = fd }
    return p
}

} // namespace raw
} // namespace net
