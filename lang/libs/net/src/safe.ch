// Canonical safe TCP surface: owning handles, `Result` errors, and `std::io`
// traits. The bare descriptor API (`Socket`, `listen_addr`, `accept_socket`,
// `recv_all`, `send_all`, `close_socket`, `dial`, timeouts, `release_raw`) lives
// in `net::raw` for internals and audited migration code; new user code should
// only need this file.
public namespace net {

using std::Result;

public type NetError = std::io::IoError;

/// The socket error currently pending on this thread, mapped onto the shared
/// `IoErrorKind` taxonomy. Callers can now distinguish "no data yet"
/// (`WouldBlock`/`TimedOut`) from a peer that reset or refused the connection,
/// which is what lets the HTTP reader retry a slow peer without also retrying a
/// dead one.
public func io_error() : std::io::IoError {
    return io_error_from(socket_last_error())
}

/// Map a platform socket/`errno` code onto `IoErrorKind`.
public func io_error_from(code : int) : std::io::IoError {
    comptime if(def.windows) {
        switch(code) {
            WSAEWOULDBLOCK => { return std::io::IoError.Kind(std::io::IoErrorKind.WouldBlock()) }
            WSAEINPROGRESS => { return std::io::IoError.Kind(std::io::IoErrorKind.WouldBlock()) }
            WSAETIMEDOUT => { return std::io::IoError.Kind(std::io::IoErrorKind.TimedOut()) }
            WSAEINTR => { return std::io::IoError.Kind(std::io::IoErrorKind.Interrupted()) }
            WSAECONNRESET => { return std::io::IoError.Kind(std::io::IoErrorKind.ConnectionReset()) }
            WSAECONNABORTED => { return std::io::IoError.Kind(std::io::IoErrorKind::BrokenPipe()) }
            WSAECONNREFUSED => { return std::io::IoError.Kind(std::io::IoErrorKind.ConnectionRefused()) }
            WSAEADDRINUSE => { return std::io::IoError.Kind(std::io::IoErrorKind.AddrInUse()) }
            WSAEADDRNOTAVAIL => { return std::io::IoError.Kind(std::io::IoErrorKind.AddrNotAvailable()) }
            WSAEACCES => { return std::io::IoError.Kind(std::io::IoErrorKind.PermissionDenied()) }
            WSAENAMETOOLONG => { return std::io::IoError.Kind(std::io::IoErrorKind.PathTooLong()) }
            WSAENOTSOCK => { return std::io::IoError.Kind(std::io::IoErrorKind.InvalidInput()) }
            WSAEAFNOSUPPORT => { return std::io::IoError.Kind(std::io::IoErrorKind.Unsupported()) }
            WSAEINVAL => { return std::io::IoError.Kind(std::io::IoErrorKind.InvalidInput()) }
            default => { return std::io::IoError.Kind(std::io::IoErrorKind.Other()) }
        }
    } else {
        switch(code) {
            EWOULDBLOCK => { return std::io::IoError.Kind(std::io::IoErrorKind.WouldBlock()) }
            EINPROGRESS => { return std::io::IoError.Kind(std::io::IoErrorKind.WouldBlock()) }
            ETIMEDOUT => { return std::io::IoError.Kind(std::io::IoErrorKind.TimedOut()) }
            EINTR => { return std::io::IoError.Kind(std::io::IoErrorKind.Interrupted()) }
            ECONNRESET => { return std::io::IoError.Kind(std::io::IoErrorKind.ConnectionReset()) }
            EPIPE => { return std::io::IoError.Kind(std::io::IoErrorKind.BrokenPipe()) }
            ECONNREFUSED => { return std::io::IoError.Kind(std::io::IoErrorKind.ConnectionRefused()) }
            EADDRINUSE => { return std::io::IoError.Kind(std::io::IoErrorKind.AddrInUse()) }
            EADDRNOTAVAIL => { return std::io::IoError.Kind(std::io::IoErrorKind.AddrNotAvailable()) }
            EACCES => { return std::io::IoError.Kind(std::io::IoErrorKind.PermissionDenied()) }
            ENAMETOOLONG => { return std::io::IoError.Kind(std::io::IoErrorKind.PathTooLong()) }
            default => { return std::io::IoError.Kind(std::io::IoErrorKind.Other()) }
        }
    }
}

/// A resolved IPv4 address value. `to_string()` renders `a.b.c.d:port`.
@direct_init
public struct SocketAddr {
    var ip : std::string
    var port : u16

    @constructor
    func constructor() {
        return SocketAddr {
            ip : std::string(),
            port : 0u
        }
    }

    public func to_string(&self) : std::string {
        var out = std::string()
        out.append_string(&self.ip)
        out.append(':')
        out.append_uinteger(self.port as ubigint)
        return out
    }
}

func socket_addr_from_raw(raw_addr : &raw::sockaddr_in) : SocketAddr {
    var value = raw_addr.sin_addr.s_addr
    var ip = std::string()
    // `s_addr` is in network byte order; on a little-endian host the first octet
    // is the low byte of the integer.
    ip.append_uinteger((value & 0xFFu) as ubigint)
    ip.append('.')
    ip.append_uinteger(((value >> 8) & 0xFFu) as ubigint)
    ip.append('.')
    ip.append_uinteger(((value >> 16) & 0xFFu) as ubigint)
    ip.append('.')
    ip.append_uinteger(((value >> 24) & 0xFFu) as ubigint)
    var port = raw_addr.sin_port
    return SocketAddr {
        ip : std::replace(&mut ip, std::string()),
        port : (((port & 0xFFu) << 8) | ((port >> 8) & 0xFFu)) as u16
    }
}

/// How to shut down a TCP stream.
public variant Shutdown {
    Read()
    Write()
    Both()
}

@direct_init
public struct TcpListener {
    internal var fd : raw::Socket

    @constructor
    func constructor() {
        return TcpListener { fd : 0 as raw::Socket }
    }

    @delete
    func delete(&mut self) {
        if(self.fd != 0 as raw::Socket) {
            raw::close_socket(self.fd)
            self.fd = 0 as raw::Socket
        }
    }

    /// Bind and listen on `addr:port`. No panic on bind/listen failure.
    public func bind(addr : &str, port : u16) : Result<TcpListener, NetError> {
        var made = std::cstring_make(addr)
        if(made is Result.Err) {
            return Result.Err(NetError.Kind(std::io::IoErrorKind.InvalidInput()))
        }
        var Ok(host) = made else unsafe { unreachable }
        var s = raw::try_listen_addr(unsafe(host.as_ptr()), port as uint)
        if(s == 0 as raw::Socket) {
            return Result.Err(io_error())
        }
        return Result.Ok(TcpListener { fd : s })
    }

    /// Accept one connection, blocking until a peer arrives.
    public func accept(&mut self) : Result<TcpStream, NetError> {
        if(self.fd == 0 as raw::Socket) {
            return Result.Err(NetError.Kind(std::io::IoErrorKind.InvalidInput()))
        }
        var s = raw::accept_socket(self.fd)
        if(s == 0 as raw::Socket) {
            return Result.Err(io_error())
        }
        return Result.Ok(TcpStream { fd : s })
    }

    public func local_addr(&self) : Result<SocketAddr, NetError> {
        if(self.fd == 0 as raw::Socket) {
            return Result.Err(NetError.Kind(std::io::IoErrorKind.InvalidInput()))
        }
        var raw_addr : raw::sockaddr_in = zeroed<raw::sockaddr_in>()
        if(net::sock_local_addr(self.fd, &raw mut raw_addr) != 0) {
            return Result.Err(io_error())
        }
        return Result.Ok(socket_addr_from_raw(&raw_addr))
    }

    public func set_nonblocking(&mut self) {
        if(self.fd != 0 as raw::Socket) { raw::set_nonblocking(self.fd) }
    }

    public func set_recv_timeout_ms(&mut self, ms : u64) {
        if(self.fd != 0 as raw::Socket) {
            raw::set_recv_timeout(self.fd, (ms / 1000u) as long, ((ms % 1000u) * 1000u) as long)
        }
    }

    public func close(&mut self) {
        if(self.fd != 0 as raw::Socket) {
            raw::close_socket(self.fd)
            self.fd = 0 as raw::Socket
        }
    }

    public func valid(&self) : bool {
        return self.fd != 0 as raw::Socket
    }
}

@direct_init
public struct TcpStream {
    internal var fd : raw::Socket

    @constructor
    func constructor() {
        return TcpStream { fd : 0 as raw::Socket }
    }

    @delete
    func delete(&mut self) {
        if(self.fd != 0 as raw::Socket) {
            raw::close_socket(self.fd)
            self.fd = 0 as raw::Socket
        }
    }

    /// Dial `addr:port` with a blocking connect.
    public func connect(addr : &str, port : u16) : Result<TcpStream, NetError> {
        var made = std::cstring_make(addr)
        if(made is Result.Err) {
            return Result.Err(NetError.Kind(std::io::IoErrorKind.InvalidInput()))
        }
        var Ok(host) = made else unsafe { unreachable }
        var s = raw::dial(unsafe(host.as_ptr()), port as uint)
        if(s == 0 as raw::Socket) {
            return Result.Err(NetError.Kind(std::io::IoErrorKind.ConnectionRefused()))
        }
        return Result.Ok(TcpStream { fd : s })
    }

    /// Read up to `buf.size()` bytes. Returns 0 on EOF.
    public func read_bytes(&mut self, buf : &mut [u8]) : Result<size_t, std::io::IoError> {
        if(self.fd == 0 as raw::Socket) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind.InvalidInput()))
        }
        if(buf.size() > 2147483647 as size_t) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind.InvalidInput()))
        }
        var n = raw::recv_all(self.fd, buf.data(), buf.size())
        if(n < 0) {
            return Result.Err(io_error())
        }
        return Result.Ok(n as size_t)
    }

    /// Write bytes, retrying short writes until all bytes are sent.
    public func write_bytes(&mut self, buf : &[u8]) : Result<size_t, std::io::IoError> {
        if(self.fd == 0 as raw::Socket) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind.InvalidInput()))
        }
        if(buf.size() > 2147483647 as size_t) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind.InvalidInput()))
        }
        var n = raw::send_all(self.fd, buf.data() as *char, buf.size() as int)
        if(n < 0) {
            return Result.Err(io_error())
        }
        return Result.Ok(n as size_t)
    }

    /// Set the receive timeout in milliseconds. `0` disables the timeout.
    public func set_read_timeout(&mut self, ms : u64) : Result<std::Unit, std::io::IoError> {
        if(self.fd == 0 as raw::Socket) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind.InvalidInput()))
        }
        raw::set_recv_timeout(self.fd, (ms / 1000u) as long, ((ms % 1000u) * 1000u) as long)
        return Result.Ok(std::Unit { })
    }

    public func shutdown(&mut self, how : Shutdown) : Result<std::Unit, std::io::IoError> {
        if(self.fd == 0 as raw::Socket) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind.InvalidInput()))
        }
        var code : int = 2
        switch(how) {
            Read() => { code = 0 }
            Write() => { code = 1 }
            Both() => { code = 2 }
        }
        if(net::sock_shutdown(self.fd, code) != 0) {
            return Result.Err(io_error())
        }
        return Result.Ok(std::Unit { })
    }

    public func set_keep_alive(&mut self, enable : bool) {
        if(self.fd != 0 as raw::Socket) { raw::set_keep_alive(self.fd, enable) }
    }

    public func set_nonblocking(&mut self) {
        if(self.fd != 0 as raw::Socket) { raw::set_nonblocking(self.fd) }
    }

    public func local_addr(&self) : Result<SocketAddr, NetError> {
        if(self.fd == 0 as raw::Socket) {
            return Result.Err(NetError.Kind(std::io::IoErrorKind.InvalidInput()))
        }
        var raw_addr : raw::sockaddr_in = zeroed<raw::sockaddr_in>()
        if(net::sock_local_addr(self.fd, &raw mut raw_addr) != 0) {
            return Result.Err(io_error())
        }
        return Result.Ok(socket_addr_from_raw(&raw_addr))
    }

    public func peer_addr(&self) : Result<SocketAddr, NetError> {
        if(self.fd == 0 as raw::Socket) {
            return Result.Err(NetError.Kind(std::io::IoErrorKind.InvalidInput()))
        }
        var raw_addr : raw::sockaddr_in = zeroed<raw::sockaddr_in>()
        if(net::sock_peer_addr(self.fd, &raw mut raw_addr) != 0) {
            return Result.Err(io_error())
        }
        return Result.Ok(socket_addr_from_raw(&raw_addr))
    }

    public func close(&mut self) {
        if(self.fd != 0 as raw::Socket) {
            raw::close_socket(self.fd)
            self.fd = 0 as raw::Socket
        }
    }

    public func valid(&self) : bool {
        return self.fd != 0 as raw::Socket
    }
}

/// Compatibility helper: bind and listen. Prefer `TcpListener.bind`.
@deprecated("use TcpListener.bind")
public func bind_tcp(addr : &str, port : u16) : Result<TcpListener, NetError> {
    return TcpListener.bind(addr, port)
}

/// Compatibility helper: dial. Prefer `TcpStream.connect`.
@deprecated("use TcpStream.connect")
public func connect_tcp(addr : &str, port : u16) : Result<TcpStream, NetError> {
    return TcpStream.connect(addr, port)
}

impl std::io::Read for TcpStream {
    func read(&mut self, buf : &mut [u8]) : Result<size_t, std::io::IoError> {
        return self.read_bytes(buf)
    }
    func read_exact(&mut self, buf : &mut [u8]) : Result<size_t, std::io::IoError> {
        var pos : size_t = 0
        while(pos < buf.size()) {
            var sub = unsafe(std::mut_span<u8>(unsafe(buf.data() + pos), buf.size() - pos))
            var r = self.read_bytes(sub)
            if(r is Result.Err) { return r }
            var Ok(n) = r else unsafe { unreachable }
            if(n == 0) {
                return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind.UnexpectedEof()))
            }
            pos += n
        }
        return Result.Ok(pos)
    }
    func try_read_to_end(&mut self, out : &mut std::vector<u8>) : Result<size_t, std::io::IoError> {
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
                    return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind.OutOfMemory()))
                }
                i += 1
            }
            total += n
        }
        return Result.Ok(total)
    }
}

impl std::io::Write for TcpStream {
    func write(&mut self, buf : &[u8]) : Result<size_t, std::io::IoError> {
        return self.write_bytes(buf)
    }
    func write_all(&mut self, buf : &[u8]) : Result<size_t, std::io::IoError> {
        return self.write_bytes(buf)
    }
    func flush(&mut self) : Result<size_t, std::io::IoError> {
        return Result.Ok(0 as size_t)
    }
}

// Async safe I/O over an owning TcpStream. The buffer is borrowed for the
// duration of the returned future; the stream handle is only used to read the
// descriptor before awaiting, so the future does not hold a long-lived borrow.
public async func read_async(stream : &mut TcpStream, buf : &mut [u8]) : Result<size_t, std::io::IoError> {
    if(stream.fd == 0 as raw::Socket) {
        return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind.InvalidInput()))
    }
    if(buf.size() > 2147483647 as size_t) {
        return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind.InvalidInput()))
    }
    var fd = stream.fd
    var n = await raw::recv_async(fd as int, buf.data(), buf.size())
    if(n < 0) { return Result.Err(io_error()) }
    return Result.Ok(n as size_t)
}

public async func write_async(stream : &mut TcpStream, buf : &[u8]) : Result<size_t, std::io::IoError> {
    if(stream.fd == 0 as raw::Socket) {
        return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind.InvalidInput()))
    }
    if(buf.size() > 2147483647 as size_t) {
        return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind.InvalidInput()))
    }
    var fd = stream.fd
    var n = await raw::send_async(fd as int, buf.data() as *char, buf.size() as int)
    if(n < 0) { return Result.Err(io_error()) }
    return Result.Ok(n as size_t)
}

} // namespace net
