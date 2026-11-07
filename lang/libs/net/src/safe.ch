// Safe TCP surface: owning handles, `Result` errors, and `std::io` traits.
// The raw `Socket` API remains available for the HTTP/TLS internals that still
// need it; new user code should use `TcpListener`/`TcpStream`.
public namespace net {

using std::Result;

public func io_error() : std::io::IoError {
    return std::io::IoError.Kind(std::io::IoErrorKind.Other())
}

public struct TcpListener {
    internal var fd : Socket

    @delete
    func delete(&mut self) {
        if(self.fd != 0 as Socket) {
            close_socket(self.fd)
            self.fd = 0 as Socket
        }
    }

    /// Accept one connection, blocking until a peer arrives.
    func accept(&mut self) : Result<TcpStream, std::io::IoError> {
        if(self.fd == 0 as Socket) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        var s = accept_socket(self.fd)
        if(s == 0 as Socket) {
            return Result.Err(io_error())
        }
        return Result.Ok(TcpStream { fd : s })
    }

    func close(&mut self) {
        if(self.fd != 0 as Socket) {
            close_socket(self.fd)
            self.fd = 0 as Socket
        }
    }

    func valid(&self) : bool {
        return self.fd != 0 as Socket
    }
}

public struct TcpStream {
    internal var fd : Socket

    @delete
    func delete(&mut self) {
        if(self.fd != 0 as Socket) {
            close_socket(self.fd)
            self.fd = 0 as Socket
        }
    }

    /// Read up to `buf.size()` bytes. Returns 0 on EOF.
    func read_bytes(&mut self, buf : &mut [u8]) : Result<size_t, std::io::IoError> {
        if(self.fd == 0 as Socket) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        if(buf.size() > 2147483647 as size_t) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        var n = recv_all(self.fd, buf.data(), buf.size())
        if(n < 0) {
            return Result.Err(io_error())
        }
        return Result.Ok(n as size_t)
    }

    /// Write bytes, retrying short writes until all bytes are sent.
    func write_bytes(&mut self, buf : &[u8]) : Result<size_t, std::io::IoError> {
        if(self.fd == 0 as Socket) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        if(buf.size() > 2147483647 as size_t) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        var n = send_all(self.fd, buf.data() as *char, buf.size() as int)
        if(n < 0) {
            return Result.Err(io_error())
        }
        return Result.Ok(n as size_t)
    }

    func close(&mut self) {
        if(self.fd != 0 as Socket) {
            close_socket(self.fd)
            self.fd = 0 as Socket
        }
    }

    func valid(&self) : bool {
        return self.fd != 0 as Socket
    }
}

/// Bind and listen on `addr:port`. No panic on bind/listen failure.
public func bind_tcp(addr : &str, port : u16) : Result<TcpListener, std::io::IoError> {
    var made = std::cstring_make(addr)
    if(made is Result.Err) {
        return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
    }
    var Ok(host) = made else unsafe { unreachable }
    var s = try_listen_addr(unsafe(host.as_ptr()), port as uint)
    if(s == 0 as Socket) {
        return Result.Err(io_error())
    }
    return Result.Ok(TcpListener { fd : s })
}

/// Dial `addr:port` with a blocking connect.
public func connect_tcp(addr : &str, port : u16) : Result<TcpStream, std::io::IoError> {
    var made = std::cstring_make(addr)
    if(made is Result.Err) {
        return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
    }
    var Ok(host) = made else unsafe { unreachable }
    var s = dial(unsafe(host.as_ptr()), port as uint)
    if(s == 0 as Socket) {
        return Result.Err(io_error())
    }
    return Result.Ok(TcpStream { fd : s })
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
                return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::UnexpectedEof()))
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
                    return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::OutOfMemory()))
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

} // namespace net
