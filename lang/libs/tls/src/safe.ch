// Safe TLS client surface. The raw mbedTLS-style context remains available
// behind this file for the HTTP client and the test suite; normal callers use
// `tls::connect` and the returned `TlsStream`.
public namespace tls {

using std::Result;

public func io_error(code : int) : std::io::IoError {
    if(code == ERR_SSL_PEER_CLOSE_NOTIFY) {
        return std::io::IoError.Kind(std::io::IoErrorKind::UnexpectedEof())
    }
    if(code == ERR_SSL_TIMEOUT) {
        return std::io::IoError.Kind(std::io::IoErrorKind::TimedOut())
    }
    if(code == ERR_SSL_WANT_READ || code == ERR_SSL_WANT_WRITE) {
        return std::io::IoError.Kind(std::io::IoErrorKind::WouldBlock())
    }
    if(code == ERR_SSL_ALLOC_FAILED) {
        return std::io::IoError.Kind(std::io::IoErrorKind::OutOfMemory())
    }
    if(code == ERR_SSL_CERT_VERIFY_FAILED || code == ERR_SSL_BAD_CERTIFICATE) {
        return std::io::IoError.Kind(std::io::IoErrorKind::PermissionDenied())
    }
    return std::io::IoError.Kind(std::io::IoErrorKind::Other())
}

/// Owning, verified TLS client stream. Dropping it frees the TLS context and
/// closes the transport socket (via `SSLContext`'s destructor).
public struct TlsStream {
    internal var ctx : SSLContext

    func read_bytes(&mut self, buf : &mut [u8]) : Result<size_t, std::io::IoError> {
        if(buf.size() > 2147483647 as size_t) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        var n = ssl_read(&raw mut self.ctx, buf.data(), buf.size() as i32)
        if(n < 0) {
            return Result.Err(io_error(n))
        }
        return Result.Ok(n as size_t)
    }

    func write_bytes(&mut self, buf : &[u8]) : Result<size_t, std::io::IoError> {
        if(buf.size() > 2147483647 as size_t) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        var n = ssl_write(&raw mut self.ctx, buf.data(), buf.size() as i32)
        if(n < 0) {
            return Result.Err(io_error(n))
        }
        return Result.Ok(n as size_t)
    }

    /// Send a TLS close_notify alert. The transport is closed when the stream is
    /// dropped even if this is not called.
    func close_notify(&mut self) {
        var _ = ssl_close_notify(&raw mut self.ctx)
    }
}

/// Connect to `host:port`, load the system CA bundle, and require a valid
/// certificate with a matching hostname.
public func connect(host : &str, port : u16) : Result<TlsStream, std::io::IoError> {
    return connect_impl(host, port, true)
}

/// Connect without peer verification. Only for tests and explicit opt-outs.
public func connect_insecure(host : &str, port : u16) : Result<TlsStream, std::io::IoError> {
    return connect_impl(host, port, false)
}

func connect_impl(host : &str, port : u16, verify : bool) : Result<TlsStream, std::io::IoError> {
    var made = std::cstring_make(host)
    if(made is Result.Err) {
        return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
    }
    var Ok(host_c) = made else unsafe { unreachable }

    var ctx : SSLContext = unsafe(std::mem.zeroed_unchecked<SSLContext>())
    ssl_init(&raw mut ctx)
    var cfg = ssl_config_init(SSL_IS_CLIENT)
    if(verify) {
        var ca = load_system_ca_bundle()
        if(ca == null) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::PermissionDenied()))
        }
        cfg.ca_chain = ca
    } else {
        cfg.authmode = SSL_VERIFY_NONE
    }
    ssl_set_config(&raw mut ctx, &raw mut cfg)

    var ret = tls_connect(&raw mut ctx, unsafe(host_c.as_ptr()), port as uint)
    if(ret < 0) {
        return Result.Err(io_error(ret))
    }
    return Result.Ok(TlsStream { ctx : ctx })
}

impl std::io::Read for TlsStream {
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

impl std::io::Write for TlsStream {
    func write(&mut self, buf : &[u8]) : Result<size_t, std::io::IoError> {
        return self.write_bytes(buf)
    }
    func write_all(&mut self, buf : &[u8]) : Result<size_t, std::io::IoError> {
        var pos : size_t = 0
        while(pos < buf.size()) {
            var sub : [u8] = buf.subview(pos, buf.size())
            var r = self.write_bytes(sub)
            if(r is Result.Err) { return r }
            var Ok(n) = r else unsafe { unreachable }
            if(n == 0) {
                return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::Other()))
            }
            pos += n
        }
        return Result.Ok(pos)
    }
    func flush(&mut self) : Result<size_t, std::io::IoError> {
        return Result.Ok(0 as size_t)
    }
}

} // namespace tls
