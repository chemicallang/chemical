public namespace net {

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
        var tv = timeval{ tv_sec: secs, tv_usec: usecs };
        // level SOL_SOCKET and optname SO_RCVTIMEO constants (platform dependent)
        const SOL_SOCKET = 1
        const SO_RCVTIMEO = 0x1006 // value often different; if incorrect, tests will fail — we can adjust per platform
        sock_setsockopt(s, SOL_SOCKET as int, SO_RCVTIMEO as int, ( &mut tv ) as *char, sizeof(timeval) as int);
    }

    // helper: set keep alive
    public func set_keep_alive(s:Socket, enable: bool) {
        const SOL_SOCKET = 1;
        const SO_KEEPALIVE = 8;
        var optval: int = if(enable) 1 else 0;
        sock_setsockopt(s, SOL_SOCKET as int, SO_KEEPALIVE as int, &optval as *char, sizeof(int) as int);
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