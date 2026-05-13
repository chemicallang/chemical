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
        const SOL_SOCKET = if(def.windows) 0xFFFF else 1
        const SO_RCVTIMEO = if(def.windows) 0x1006 else 20
        comptime if(def.windows) {
            // Windows expects DWORD timeout in milliseconds, not struct timeval
            var timeout_ms: u32 = (secs * 1000 + usecs / 1000) as u32;
            sock_setsockopt(s, SOL_SOCKET as int, SO_RCVTIMEO as int, &timeout_ms as *char, sizeof(u32) as int);
        } else {
            var tv = timeval{ tv_sec: secs, tv_usec: usecs };
            sock_setsockopt(s, SOL_SOCKET as int, SO_RCVTIMEO as int, (&mut tv) as *char, sizeof(timeval) as int);
        }
    }

    // helper: set keep alive
    public func set_keep_alive(s:Socket, enable: bool) {
        // Windows SOL_SOCKET = 0xFFFF, POSIX = 1
        const SOL_SOCKET = if(def.windows) 0xFFFF else 1
        const SO_KEEPALIVE = if(def.windows) 8 else 9
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
        comptime if(def.windows) {
            // Windows returns INVALID_SOCKET (~0) on failure, which is not 0
            if (s == 0 as Socket || (s as longlong) < 0) {
                var e = WSAGetLastError();
                printf("socket() failed (WSAGetLastError): %d\n", e as int);
                panic("socket() failed");
            }
        } else {
            if (s == 0 as Socket) {
                panic("socket() failed");
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
        const SOL_SOCKET = if(def.windows) 0xFFFF else 1
        const SO_REUSEADDR = if(def.windows) 4 else 2
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
                printf("bind() failed (WSAGetLastError): %d\n", e as int);
                panic("bind() failed (WSAGetLastError)");
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

    public func dial(addr_str:*char, port: uint) : Socket {
        
        startup();
        
        var addrinfo: *mut char = null;
        var port_str : [8]char;
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
        
        var ret = sock_getaddrinfo(addr_str, &port_str[0], null, &mut addrinfo);
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
            var ai_addr_ptr = *(((curr as usize) + AI_ADDR_OFFSET) as *mut *mut char);
            var ai_addrlen = *(((curr as usize) + 16u) as *mut usize);
            
            s = sock_socket(AF_INET as int, SOCK_STREAM as int, IPPROTO_TCP as int);
            if(s != 0 as Socket && (s as longlong) >= 0) {
                
                if(sock_connect(s, ai_addr_ptr, ai_addrlen as int) == 0) {
                    
                    sock_freeaddrinfo(addrinfo);
                    return s;
                }
                
                sock_close(s);
            }
            
            // move to next (ai_next is at +40 on both POSIX and Windows x64)
            curr = *(((curr as usize) + 40u) as *mut *mut char);
            s = 0 as Socket;
        }

        
        sock_freeaddrinfo(addrinfo);
        return 0 as Socket;
    }


}