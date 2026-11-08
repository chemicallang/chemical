// Safe TLS client surface. The raw mbedTLS-style context remains available
// behind this file for the HTTP client and the test suite; normal callers use
// `tls::connect` and the returned `TlsStream`.
public namespace tls {

using std::Option;
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

    func alpn_protocol(&self) : Option<std::string_view> {
        if(self.ctx.alpn_negotiated == null) { return Option.None<std::string_view>() }
        return Option.Some<std::string_view>(unsafe(std::string_view(self.ctx.alpn_negotiated, self.ctx.alpn_negotiated_len)))
    }

    func peer_subject(&self) : Option<std::string> {
        if(self.ctx.peer_cert == null) { return Option.None<std::string>() }
        return Option.Some<std::string>(unsafe(self.ctx.peer_cert.subject.copy()))
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

// ---------------------------------------------------------------------------
// Server-side TLS and certificate/key ownership
// ---------------------------------------------------------------------------

public struct CertificateChain {
    internal var ptr : *mut X509Cert

    @delete
    func delete(&mut self) {
        if(self.ptr != null) {
            cert_chain_free(self.ptr)
            self.ptr = null
        }
    }

    func subject(&self) : Option<std::string> {
        if(self.ptr == null) { return Option.None<std::string>() }
        return Option.Some<std::string>(unsafe(self.ptr.subject.copy()))
    }
}

public struct PrivateKey {
    internal var ptr : *mut void
    internal var kind : int

    @delete
    func delete(&mut self) {
        if(self.ptr == null) { return }
        if(self.kind == 1) {
            rsa_free(self.ptr as *mut RSAContext)
        } else if(self.kind == 2) {
            ecdsa_context_free(self.ptr as *mut ECDSAContext)
        }
        unsafe { dealloc self.ptr }
        self.ptr = null
    }
}

public func load_certificate(path : &str) : Result<CertificateChain, std::io::IoError> {
    var made = std::cstring_make(path)
    if(made is Result.Err) {
        return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind.InvalidInput()))
    }
    var Ok(cpath) = made else unsafe { unreachable }
    var cert = x509_crt_load_pem_file(unsafe(cpath.as_ptr()))
    if(cert == null) {
        return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::NotFound()))
    }
    return Result.Ok(CertificateChain { ptr : cert })
}

public func load_rsa_private_key(path : &str) : Result<PrivateKey, std::io::IoError> {
    var made = std::cstring_make(path)
    if(made is Result.Err) {
        return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
    }
    var Ok(cpath) = made else unsafe { unreachable }
    var key = rsa_privkey_load_pem_file(unsafe(cpath.as_ptr()))
    if(key == null) {
        return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::NotFound()))
    }
    return Result.Ok(PrivateKey { ptr : key as *mut void, kind : 1 })
}

public func load_ec_private_key(path : &str) : Result<PrivateKey, std::io::IoError> {
    var made = std::cstring_make(path)
    if(made is Result.Err) {
        return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
    }
    var Ok(cpath) = made else unsafe { unreachable }
    var key = ec_privkey_load_hex_file(unsafe(cpath.as_ptr()))
    if(key == null) {
        return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::NotFound()))
    }
    return Result.Ok(PrivateKey { ptr : key as *mut void, kind : 2 })
}

public func rsa_private_key_from_components(n : [u8], d : [u8]) : Result<PrivateKey, std::io::IoError> {
    if(n.size() == 0 || d.size() == 0 || n.size() > 512 || d.size() > 512) {
        return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
    }
    var ctx = unsafe(malloc(sizeof(RSAContext))) as *mut RSAContext
    if(ctx == null) { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::OutOfMemory())) }
    unsafe { memset(ctx as *mut void, 0, sizeof(RSAContext)) }
    rsa_init(ctx, RSA_PKCS_V15, 0)
    var ret = rsa_import_privkey(ctx, n.data(), n.size(), d.data(), d.size())
    if(ret < 0) {
        rsa_free(ctx)
        unsafe { dealloc ctx }
        return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
    }
    return Result.Ok(PrivateKey { ptr : ctx as *mut void, kind : 1 })
}

/// Borrowed server credentials. The lifetime keeps the certificate chain and
/// private key alive for at least as long as this config, including the
/// `accept` call that consumes it.
public struct TlsServerConfig<'a> {
    internal var cert : &'a CertificateChain
    internal var key : &'a PrivateKey
    internal var cipher_suite : int
}

public func <'a> server_config(certs : &'a CertificateChain, key : &'a PrivateKey) : TlsServerConfig<'a> {
    return TlsServerConfig<'a> {
        cert : certs,
        key : key,
        cipher_suite : TLS_RSA_WITH_AES_128_GCM_SHA256
    }
}

// Owning server-side TLS stream. `tls_accept` transfers the accepted socket
// into the context; the caller must release its `TcpStream` handle first.
public struct TlsServerStream {
    internal var ctx : *mut SSLContext

    @delete
    func delete(&mut self) {
        if(self.ctx != null) {
            ssl_free(self.ctx)
            unsafe { dealloc self.ctx }
            self.ctx = null
        }
    }

    func read_bytes(&mut self, buf : &mut [u8]) : Result<size_t, std::io::IoError> {
        if(self.ctx == null) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        if(buf.size() > 2147483647 as size_t) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        var n = ssl_read(self.ctx, buf.data(), buf.size() as i32)
        if(n < 0) { return Result.Err(io_error(n)) }
        return Result.Ok(n as size_t)
    }

    func write_bytes(&mut self, buf : &[u8]) : Result<size_t, std::io::IoError> {
        if(self.ctx == null) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        if(buf.size() > 2147483647 as size_t) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        var n = ssl_write(self.ctx, buf.data(), buf.size() as i32)
        if(n < 0) { return Result.Err(io_error(n)) }
        return Result.Ok(n as size_t)
    }

    func close_notify(&mut self) {
        if(self.ctx != null) { var _ = ssl_close_notify(self.ctx) }
    }

    func alpn_protocol(&self) : Option<std::string_view> {
        if(self.ctx == null) { return Option.None<std::string_view>() }
        var neg = unsafe(self.ctx.alpn_negotiated)
        if(neg == null) { return Option.None<std::string_view>() }
        var len = unsafe(self.ctx.alpn_negotiated_len)
        return Option.Some<std::string_view>(unsafe(std::string_view(neg, len)))
    }
}

/// Accept a TLS connection on an already-connected TCP stream. Ownership of the
/// socket moves into the returned stream.
public func <'a> accept(stream : net::TcpStream, cfg : &'a TlsServerConfig<'a>) : std::Result<TlsServerStream, std::io::IoError> {
    if(cfg.cert.ptr == null || cfg.key.ptr == null) {
        return std::Result.Err<TlsServerStream, std::io::IoError>(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
    }
    var fd = unsafe(net::release_raw(&mut stream))
    var ctx = tls_accept(fd, cfg.cert.ptr, cfg.key.ptr, cfg.cipher_suite)
    if(ctx == null) {
        return std::Result.Err<TlsServerStream, std::io::IoError>(std::io::IoError.Kind(std::io::IoErrorKind::ConnectionReset()))
    }
    return std::Result.Ok<TlsServerStream, std::io::IoError>(TlsServerStream { ctx : ctx })
}

impl std::io::Read for TlsServerStream {
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
            if(n == 0) { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::UnexpectedEof())) }
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

impl std::io::Write for TlsServerStream {
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
            if(n == 0) { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::Other())) }
            pos += n
        }
        return Result.Ok(pos)
    }
    func flush(&mut self) : Result<size_t, std::io::IoError> {
        return Result.Ok(0 as size_t)
    }
}


} // namespace tls
