// Async `net` sockets (design §7 Tier 1).
//
// Additive: every synchronous function in `api.ch`/`main.ch` is unchanged. The
// sockets returned here are put into non-blocking mode and driven by the async
// reactor (`async::readable` / `async::writable`); do not pass them to the
// synchronous helpers.
//
// Connect and accept are offloaded to the runtime thread pool (connect is an
// inherently blocking kernel call). `recv`/`send` are genuinely asynchronous on
// POSIX: they retry after awaiting fd readiness. Windows (whose reactor is a
// stub) falls back to the thread pool for all of them.
//
// The `*_async` entry points speak raw file descriptors (`int`), and
// `AsyncSocket` is a thin synchronous wrapper. Two compiler limitations drive
// this shape:
//   * B23 — a generic `FutureHandle<StructT>` corrupts a plain-struct payload on
//     LLVM, so futures never carry `AsyncSocket`;
//   * the 2c async lowering mis-handles field access on a struct-typed async
//     parameter, so struct arguments stay out of coroutine bodies.
public namespace net {

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
    public func read(&self, buf : *mut u8, cap : usize) : core::async::FutureHandle<int> {
        return recv_async(self.fd, buf, cap)
    }

    // Await a full write of `len` bytes (returns `len`, or <0 on error).
    public func write(&self, data : *char, len : int) : core::async::FutureHandle<int> {
        return send_async(self.fd, data, len)
    }

    // Await the next inbound connection; resolves with the accepted fd (>0) or 0.
    public func accept(&self) : core::async::FutureHandle<int> {
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
public func dial_async(addr_str : *char, port : uint) : core::async::FutureHandle<int> {
    return async::spawn_blocking<int>(|addr_str, port|() => {
        var s = dial(addr_str, port)
        if(s == 0 as Socket) {
            return 0
        }
        set_nonblocking(s)
        return s as int
    })
}

// ---- accept ----------------------------------------------------------------

// Resolves with the accepted fd (>0), or 0 on failure.
public async func accept_async(listener : int) : int {
    comptime if(def.windows) {
        var accepted = await async::spawn_blocking<int>(|listener|() => accept_socket(listener as Socket) as int)
        return accepted
    } else {
        set_nonblocking(listener as Socket)
        var accepted : Socket = 0 as Socket
        while(accepted == 0 as Socket) {
            var ready = await async::readable(listener)
            accepted = accept_socket(listener as Socket)
        }
        set_nonblocking(accepted)
        return accepted as int
    }
}

// ---- recv / send -----------------------------------------------------------

public async func recv_async(fd : int, buf : *mut u8, cap : usize) : int {
    comptime if(def.windows) {
        var n = await async::spawn_blocking<int>(|fd, buf, cap|() => recv_all(fd as Socket, buf, cap))
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
    comptime if(def.windows) {
        var n = await async::spawn_blocking<int>(|fd, data, len|() => send_all(fd as Socket, data, len))
        return n
    } else {
        var off = 0
        while(off < len) {
            var n = sock_send(fd as Socket, &raw data[off], len - off)
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

}
