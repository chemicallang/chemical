// Canonical safe TLS surface: owning configuration and stream handles,
// `Result` errors, PEM/DER certificate loading, ALPN, and `std::io` traits.
// The legacy mbedTLS-shaped implementation (`SSLContext`, `SSLConfig`,
// `X509Cert`, `RSAContext`, `ssl_*`) lives in `tls::raw`; new code should use
// this file only.
public namespace tls {

using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;

// ---------------------------------------------------------------------------
// Errors
// ---------------------------------------------------------------------------

public variant TlsError {
    Io(e : std::io::IoError)
    InvalidCertificate(m : string)
    HandshakeFailure(m : string)
    NoSystemCaBundle()
    InvalidConfig(m : string)
    Other(m : string)

    public func message(&self) : string_view {
        switch(self) {
            Io(e) => { return e.message() }
            InvalidCertificate(m) => { return m.to_view() }
            HandshakeFailure(m) => { return m.to_view() }
            NoSystemCaBundle() => { return string_view("no system CA bundle found") }
            InvalidConfig(m) => { return m.to_view() }
            Other(m) => { return m.to_view() }
            default => { return string_view("") }
        }
    }
}

public func io_error(code : int) : std::io::IoError {
    if(code == raw::ERR_SSL_PEER_CLOSE_NOTIFY) {
        return std::io::IoError.Kind(std::io::IoErrorKind::UnexpectedEof())
    }
    if(code == raw::ERR_SSL_TIMEOUT) {
        return std::io::IoError.Kind(std::io::IoErrorKind::TimedOut())
    }
    if(code == raw::ERR_SSL_WANT_READ || code == raw::ERR_SSL_WANT_WRITE) {
        return std::io::IoError.Kind(std::io::IoErrorKind::WouldBlock())
    }
    if(code == raw::ERR_SSL_ALLOC_FAILED) {
        return std::io::IoError.Kind(std::io::IoErrorKind::OutOfMemory())
    }
    if(code == raw::ERR_SSL_CERT_VERIFY_FAILED || code == raw::ERR_SSL_BAD_CERTIFICATE) {
        return std::io::IoError.Kind(std::io::IoErrorKind::PermissionDenied())
    }
    return std::io::IoError.Kind(std::io::IoErrorKind::Other())
}

@retained
public func make_string(value : &str) : string {
    var out = string()
    out.append_view(value.to_view())
    return out
}

func cert_cn(crt : *mut raw::X509Cert) : string {
    var out = string()
    raw::cert_get_cn(crt, &raw mut out)
    return out
}

// ---------------------------------------------------------------------------
// Certificates and keys
// ---------------------------------------------------------------------------

/// A single X.509 certificate. Owns its parsed fields unless it was borrowed
/// from a live TLS session (peer certificates are borrowed).
public struct Certificate {
    internal var ptr : *mut raw::X509Cert
    internal var owns : bool
    internal var cn : string

    @delete
    func delete(&mut self) {
        if(self.ptr == null) { return }
        if(self.owns) {
            raw::cert_free(self.ptr)
            unsafe { dealloc self.ptr }
        }
        self.ptr = null
        self.cn = string()
    }

    func from_der(der : &[u8]) : Result<Certificate, TlsError> {
        if(der.size() == 0) {
            return Result.Err(TlsError.InvalidCertificate(make_string("empty certificate")))
        }
        var mem = unsafe(malloc(sizeof(raw::X509Cert))) as *mut raw::X509Cert
        if(mem == null) {
            return Result.Err(TlsError.Io(std::io::IoError.Kind(std::io::IoErrorKind::OutOfMemory())))
        }
        unsafe { memset(mem as *mut void, 0, sizeof(raw::X509Cert)) }
        raw::x509_cert_init(mem)
        var ret = raw::parse_cert_der(mem, der.data(), der.size())
        if(ret < 0) {
            raw::cert_free(mem)
            unsafe { dealloc mem }
            return Result.Err(TlsError.InvalidCertificate(make_string("invalid DER certificate")))
        }
        return Result.Ok(Certificate { ptr : mem, owns : true, cn : cert_cn(mem) })
    }

    func from_pem(pem : &[u8]) : Result<Certificate, TlsError> {
        if(pem.size() == 0) {
            return Result.Err(TlsError.InvalidCertificate(make_string("empty PEM")))
        }
        var mem = unsafe(malloc(sizeof(raw::X509Cert))) as *mut raw::X509Cert
        if(mem == null) {
            return Result.Err(TlsError.Io(std::io::IoError.Kind(std::io::IoErrorKind::OutOfMemory())))
        }
        unsafe { memset(mem as *mut void, 0, sizeof(raw::X509Cert)) }
        raw::x509_cert_init(mem)
        var ret = raw::parse_cert_pem(mem, pem.data(), pem.size())
        if(ret < 0) {
            raw::cert_free(mem)
            unsafe { dealloc mem }
            return Result.Err(TlsError.InvalidCertificate(make_string("invalid PEM certificate")))
        }
        return Result.Ok(Certificate { ptr : mem, owns : true, cn : cert_cn(mem) })
    }

    public func subject_cn(&self) : Option<string_view> {
        if(self.cn.empty()) { return Option.None<string_view>() }
        return Option.Some<string_view>(self.cn.to_view())
    }

    public func subject(&self) : Option<string_view> {
        if(self.ptr == null) { return Option.None<string_view>() }
        return Option.Some<string_view>(unsafe(self.ptr.subject.to_view()))
    }

    public func verify_hostname(&self, host : &str) : bool {
        if(self.ptr == null) { return false }
        var made = std::cstring_make(host)
        if(made is Result.Err) { return false }
        var Ok(host_c) = made else unsafe { unreachable }
        return raw::x509_verify_hostname(self.ptr, unsafe(host_c.as_ptr())) == 0
    }

    public func is_valid_now(&self) : bool {
        if(self.ptr == null) { return false }
        return raw::x509_check_date(self.ptr) == 0
    }
}

/// An owned chain of X.509 certificates. The first certificate is the leaf.
@direct_init
public struct CertificateChain {
    internal var ptr : *mut raw::X509Cert
    internal var owns : bool

    @constructor
    func constructor() {
        return CertificateChain { ptr : null, owns : false }
    }

    @delete
    func delete(&mut self) {
        if(self.ptr != null) {
            if(self.owns) { raw::cert_chain_free(self.ptr) }
            self.ptr = null
        }
    }

    public unsafe func raw_handle(&self) : *mut raw::X509Cert {
        return self.ptr
    }

    public unsafe func from_raw_borrowed(ptr : *mut raw::X509Cert) : CertificateChain {
        return CertificateChain { ptr : ptr, owns : false }
    }

    func from_der(der : &[u8]) : Result<CertificateChain, TlsError> {
        var one = Certificate.from_der(der)
        if(one is Result.Err) {
            var Err(e) = one else unsafe { unreachable }
            return Result.Err(e)
        }
        var Ok(cert) = one else unsafe { unreachable }
        return Result.Ok(CertificateChain { ptr : std::replace(&mut cert.ptr, null), owns : true })
    }

    func from_pem(pem : &[u8]) : Result<CertificateChain, TlsError> {
        var all = unsafe(string_view(pem.data() as *char, pem.size()))
        var begin = string_view("-----BEGIN CERTIFICATE-----")
        var end = string_view("-----END CERTIFICATE-----")
        var head : *mut raw::X509Cert = null
        var tail : *mut raw::X509Cert = null
        var pos : size_t = 0
        var count : size_t = 0
        while(pos < all.size()) {
            var rest = all.subview(pos, all.size())
            var b = rest.find(&begin)
            if(b == std::NPOS) { break }
            var from_begin = all.subview(pos + b, all.size())
            var e = from_begin.find(&end)
            if(e == std::NPOS) {
                if(count == 0) {
                    return Result.Err(TlsError.InvalidCertificate(make_string("unterminated PEM certificate")))
                }
                break
            }
            var block_len = e + end.size()
            var mem = unsafe(malloc(sizeof(raw::X509Cert))) as *mut raw::X509Cert
            if(mem == null) {
                if(head != null) { raw::cert_chain_free(head) }
                return Result.Err(TlsError.Io(std::io::IoError.Kind(std::io::IoErrorKind::OutOfMemory())))
            }
            unsafe { memset(mem as *mut void, 0, sizeof(raw::X509Cert)) }
            raw::x509_cert_init(mem)
            var ret = raw::parse_cert_pem(mem, from_begin.data() as *u8, block_len)
            if(ret < 0) {
                raw::cert_free(mem)
                unsafe { dealloc mem }
                if(head != null) { raw::cert_chain_free(head) }
                return Result.Err(TlsError.InvalidCertificate(make_string("invalid PEM certificate")))
            }
            if(head == null) {
                head = mem
                tail = mem
            } else {
                unsafe {
                    tail.next = mem
                    mem.prev = tail
                }
                tail = mem
            }
            count += 1
            pos = pos + b + block_len
        }
        if(head == null) {
            return Result.Err(TlsError.InvalidCertificate(make_string("no PEM certificate found")))
        }
        return Result.Ok(CertificateChain { ptr : head, owns : true })
    }

    public func push(&mut self, cert : Certificate) {
        if(cert.ptr == null) { return }
        if(self.ptr == null) {
            self.ptr = std::replace(&mut cert.ptr, null)
            return
        }
        var tail = self.ptr
        while(unsafe(tail.next) != null) {
            tail = unsafe(tail.next)
        }
        var node = std::replace(&mut cert.ptr, null)
        unsafe {
            node.prev = tail
            node.next = null
            tail.next = node
        }
    }

    public func subject(&self) : Option<string_view> {
        if(self.ptr == null) { return Option.None<string_view>() }
        return Option.Some<string_view>(unsafe(self.ptr.subject.to_view()))
    }
}

/// An owned private key (RSA today; ECDSA keys also load through here).
@direct_init
public struct PrivateKey {
    internal var ptr : *mut void
    internal var kind : int

    @constructor
    func constructor() {
        return PrivateKey { ptr : null, kind : 0 }
    }

    @delete
    func delete(&mut self) {
        if(self.ptr == null) { return }
        if(self.kind == 1) {
            raw::rsa_free(self.ptr as *mut raw::RSAContext)
        } else if(self.kind == 2) {
            raw::ecdsa_context_free(self.ptr as *mut raw::ECDSAContext)
        }
        unsafe { dealloc self.ptr }
        self.ptr = null
    }
}

public func load_certificate(path : &str) : Result<CertificateChain, TlsError> {
    var made = std::cstring_make(path)
    if(made is Result.Err) {
        return Result.Err(TlsError.Io(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput())))
    }
    var Ok(cpath) = made else unsafe { unreachable }
    var cert = raw::x509_crt_load_pem_file(unsafe(cpath.as_ptr()))
    if(cert == null) {
        return Result.Err(TlsError.InvalidCertificate(make_string("cannot load certificate file")))
    }
    return Result.Ok(CertificateChain { ptr : cert, owns : true })
}

public func load_rsa_private_key(path : &str) : Result<PrivateKey, TlsError> {
    var made = std::cstring_make(path)
    if(made is Result.Err) {
        return Result.Err(TlsError.Io(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput())))
    }
    var Ok(cpath) = made else unsafe { unreachable }
    var key = raw::rsa_privkey_load_pem_file(unsafe(cpath.as_ptr()))
    if(key == null) {
        return Result.Err(TlsError.InvalidCertificate(make_string("cannot load RSA private key")))
    }
    return Result.Ok(PrivateKey { ptr : key as *mut void, kind : 1 })
}

public func load_ec_private_key(path : &str) : Result<PrivateKey, TlsError> {
    var made = std::cstring_make(path)
    if(made is Result.Err) {
        return Result.Err(TlsError.Io(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput())))
    }
    var Ok(cpath) = made else unsafe { unreachable }
    var key = raw::ec_privkey_load_hex_file(unsafe(cpath.as_ptr()))
    if(key == null) {
        return Result.Err(TlsError.InvalidCertificate(make_string("cannot load EC private key")))
    }
    return Result.Ok(PrivateKey { ptr : key as *mut void, kind : 2 })
}

public func rsa_private_key_from_components(n : &[u8], d : &[u8]) : Result<PrivateKey, TlsError> {
    if(n.size() == 0 || d.size() == 0 || n.size() > 512 || d.size() > 512) {
        return Result.Err(TlsError.InvalidConfig(make_string("invalid RSA key components")))
    }
    var ctx = unsafe(malloc(sizeof(raw::RSAContext))) as *mut raw::RSAContext
    if(ctx == null) {
        return Result.Err(TlsError.Io(std::io::IoError.Kind(std::io::IoErrorKind::OutOfMemory())))
    }
    unsafe { memset(ctx as *mut void, 0, sizeof(raw::RSAContext)) }
    raw::rsa_init(ctx, raw::RSA_PKCS_V15, 0)
    var ret = raw::rsa_import_privkey(ctx, n.data(), n.size(), d.data(), d.size())
    if(ret < 0) {
        raw::rsa_free(ctx)
        unsafe { dealloc ctx }
        return Result.Err(TlsError.InvalidConfig(make_string("invalid RSA key components")))
    }
    return Result.Ok(PrivateKey { ptr : ctx as *mut void, kind : 1 })
}

public func load_system_ca_bundle() : Result<CertificateChain, TlsError> {
    var ca = raw::load_system_ca_bundle()
    if(ca == null) {
        return Result.Err(TlsError.NoSystemCaBundle())
    }
    return Result.Ok(CertificateChain { ptr : ca, owns : true })
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

/// Owned TLS configuration. Certificate/key setters borrow the caller's
/// handles; they only have to outlive the handshake that consumes this config.
public struct TlsConfig {
    internal var conf : raw::SSLConfig
    internal var ca : *mut raw::X509Cert
    internal var own_cert : *mut raw::X509Cert
    internal var own_key : *mut void
    internal var hostname : string
    internal var alpn_storage : vector<string>
    internal var alpn_ptrs : vector<*char>
    internal var insecure : bool

    @constructor
    func constructor(endpoint : int) {
        return TlsConfig {
            conf : raw::ssl_config_init(endpoint),
            ca : null,
            own_cert : null,
            own_key : null,
            hostname : string(),
            alpn_storage : vector<string>(),
            alpn_ptrs : vector<*char>(),
            insecure : false
        }
    }

    func client() : TlsConfig {
        return TlsConfig(raw::SSL_IS_CLIENT)
    }

    func server() : TlsConfig {
        return TlsConfig(raw::SSL_IS_SERVER)
    }

    public func set_ca_chain(&mut self, chain : &CertificateChain) : Result<std::Unit, TlsError> {
        if(chain.ptr == null) {
            return Result.Err(TlsError.InvalidConfig(make_string("empty CA chain")))
        }
        self.ca = chain.ptr
        unsafe { raw::ssl_set_ca_chain(&raw mut self.conf, chain.ptr) }
        return Result.Ok(std::Unit { })
    }

    public unsafe func set_ca_chain_ptr(&mut self, ca : *mut raw::X509Cert) : Result<std::Unit, TlsError> {
        if(ca == null) {
            return Result.Err(TlsError.InvalidConfig(make_string("empty CA chain")))
        }
        self.ca = ca
        unsafe { raw::ssl_set_ca_chain(&raw mut self.conf, ca) }
        return Result.Ok(std::Unit { })
    }

    public func set_own_chain(&mut self, chain : &CertificateChain) : Result<std::Unit, TlsError> {
        if(chain.ptr == null) {
            return Result.Err(TlsError.InvalidConfig(make_string("empty certificate chain")))
        }
        self.own_cert = chain.ptr
        self.conf.own_cert = chain.ptr
        return Result.Ok(std::Unit { })
    }

    public func set_own_key(&mut self, key : &PrivateKey) : Result<std::Unit, TlsError> {
        if(key.ptr == null) {
            return Result.Err(TlsError.InvalidConfig(make_string("empty private key")))
        }
        self.own_key = key.ptr
        self.conf.own_key = key.ptr
        return Result.Ok(std::Unit { })
    }

    public func set_hostname(&mut self, host : &str) : Result<std::Unit, TlsError> {
        self.hostname = make_string(host)
        self.conf.hostname = self.hostname.data()
        return Result.Ok(std::Unit { })
    }

    public func set_alpn(&mut self, protocols : &[string_view]) : Result<std::Unit, TlsError> {
        if(protocols.size() > 255) {
            return Result.Err(TlsError.InvalidConfig(make_string("too many ALPN protocols")))
        }
        self.alpn_storage.clear()
        self.alpn_ptrs.clear()
        var i : size_t = 0
        while(i < protocols.size()) {
            self.alpn_storage.push_back(unsafe(protocols.get(i).to_string()))
            i += 1
        }
        i = 0
        while(i < self.alpn_storage.size()) {
            self.alpn_ptrs.push_back(unsafe(self.alpn_storage.get_ptr(i).data()))
            i += 1
        }
        if(self.alpn_ptrs.size() == 0) {
            self.conf.alpn_list = null
            self.conf.alpn_count = 0
            return Result.Ok(std::Unit { })
        }
        unsafe { raw::ssl_set_alpn_protocols(&raw mut self.conf, self.alpn_ptrs.data() as *mut *char, self.alpn_ptrs.size() as u8) }
        return Result.Ok(std::Unit { })
    }

    /// Disable certificate and hostname verification (tests/self-signed only).
    public func insecure_skip_verify(&mut self, skip : bool = true) {
        self.insecure = skip
        if(skip) {
            self.conf.authmode = raw::SSL_VERIFY_NONE
        } else {
            self.conf.authmode = raw::SSL_VERIFY_REQUIRED
        }
    }
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
        cipher_suite : raw::TLS_RSA_WITH_AES_128_GCM_SHA256
    }
}

// ---------------------------------------------------------------------------
// Streams
// ---------------------------------------------------------------------------

func make_ctx(cfg : &TlsConfig) : *mut raw::SSLContext {
    var ctx = unsafe(malloc(sizeof(raw::SSLContext))) as *mut raw::SSLContext
    if(ctx == null) { return null }
    raw::ssl_init(ctx)
    unsafe { raw::ssl_set_config(ctx, &raw mut cfg.conf) }
    return ctx
}

@retained
public func peer_from_ctx(ctx : *mut raw::SSLContext) : Certificate {
    if(ctx == null) {
        return Certificate { ptr : null, owns : false, cn : string() }
    }
    var peer = unsafe(ctx.peer_cert)
    if(peer == null) {
        return Certificate { ptr : null, owns : false, cn : string() }
    }
    return Certificate { ptr : peer, owns : false, cn : cert_cn(peer) }
}

/// Owning, verified TLS client stream. Dropping it frees the TLS context and
/// closes the transport socket.
public struct TlsStream {
    internal var ctx : *mut raw::SSLContext
    internal var peer : Certificate

    @delete
    func delete(&mut self) {
        if(self.ctx != null) {
            raw::ssl_free(self.ctx)
            unsafe { dealloc self.ctx }
            self.ctx = null
        }
        self.peer = Certificate { ptr : null, owns : false, cn : string() }
    }

    /// Connect to `host:port` using `cfg`, then verify the peer hostname unless
    /// verification was explicitly disabled in the config.
    func connect(host : &str, port : u16, cfg : &TlsConfig) : Result<TlsStream, TlsError> {
        var made = std::cstring_make(host)
        if(made is Result.Err) {
            return Result.Err(TlsError.Io(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput())))
        }
        var Ok(host_c) = made else unsafe { unreachable }
        var ctx = make_ctx(cfg)
        if(ctx == null) {
            return Result.Err(TlsError.Io(std::io::IoError.Kind(std::io::IoErrorKind::OutOfMemory())))
        }
        raw::ssl_set_hostname(ctx, unsafe(host_c.as_ptr()))
        var ret = raw::tls_connect(ctx, unsafe(host_c.as_ptr()), port as uint)
        if(ret < 0) {
            raw::ssl_free(ctx)
            unsafe { dealloc ctx }
            return Result.Err(TlsError.Io(io_error(ret)))
        }
        if(!cfg.insecure && unsafe(ctx.peer_cert) != null) {
            if(raw::x509_verify_hostname(unsafe(ctx.peer_cert), unsafe(host_c.as_ptr())) != 0) {
                raw::ssl_free(ctx)
                unsafe { dealloc ctx }
                return Result.Err(TlsError.HandshakeFailure(make_string("TLS hostname mismatch")))
            }
        }
        return Result.Ok(TlsStream { ctx : ctx, peer : peer_from_ctx(ctx) })
    }

    /// Server-side accept from an explicit certificate chain + private key.
    /// The borrowed credential handles are kept alive by the local
    /// `TlsServerConfig` for the whole handshake.
    func accept(stream : net::TcpStream, certs : &CertificateChain, key : &PrivateKey) : Result<TlsServerStream, TlsError> {
        var cfg = server_config(certs, key)
        return tls::accept(stream, &cfg)
    }

    /// Wrap an already-connected `TcpStream` as a TLS client (e.g. a proxy
    /// CONNECT tunnel). The config's hostname is used for SNI/verification.
    func wrap(stream : net::TcpStream, cfg : &TlsConfig) : Result<TlsStream, TlsError> {
        var fd = unsafe(net::raw::release_tcp(&mut stream))
        var ctx = make_ctx(cfg)
        if(ctx == null) {
            net::raw::close_socket(fd)
            return Result.Err(TlsError.Io(std::io::IoError.Kind(std::io::IoErrorKind::OutOfMemory())))
        }
        raw::ssl_set_socket(ctx, fd)
        if(!cfg.hostname.empty()) {
            raw::ssl_set_hostname(ctx, cfg.hostname.data())
        }
        var ret = raw::ssl_handshake(ctx)
        if(ret < 0) {
            raw::ssl_free(ctx)
            unsafe { dealloc ctx }
            return Result.Err(TlsError.Io(io_error(ret)))
        }
        if(!cfg.insecure && unsafe(ctx.peer_cert) != null && !cfg.hostname.empty()) {
            if(raw::x509_verify_hostname(unsafe(ctx.peer_cert), cfg.hostname.data()) != 0) {
                raw::ssl_free(ctx)
                unsafe { dealloc ctx }
                return Result.Err(TlsError.HandshakeFailure(make_string("TLS hostname mismatch")))
            }
        }
        return Result.Ok(TlsStream { ctx : ctx, peer : peer_from_ctx(ctx) })
    }

    public func into_box(stream : &mut TlsStream) : *mut TlsStream {
        var p = unsafe(malloc(sizeof(TlsStream))) as *mut TlsStream
        if(p == null) { return null }
        unsafe { *p = TlsStream { ctx : null, peer : Certificate { ptr : null, owns : false, cn : string() } } }
        unsafe { p.ctx = stream.ctx }
        unsafe { p.peer = stream.peer }
        stream.ctx = null
        stream.peer = Certificate { ptr : null, owns : false, cn : string() }
        return p
    }

    func read_bytes(&mut self, buf : &mut [u8]) : Result<size_t, std::io::IoError> {
        if(self.ctx == null) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        if(buf.size() > 2147483647 as size_t) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        var n = raw::ssl_read(self.ctx, buf.data(), buf.size() as i32)
        if(n < 0) { return Result.Err(io_error(n)) }
        return Result.Ok(n as size_t)
    }

    func write_bytes(&mut self, data : &[u8]) : Result<size_t, std::io::IoError> {
        if(self.ctx == null) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        if(data.size() > 2147483647 as size_t) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        var n = raw::ssl_write(self.ctx, data.data(), data.size() as i32)
        if(n < 0) { return Result.Err(io_error(n)) }
        return Result.Ok(n as size_t)
    }

    public func set_read_timeout(&mut self, ms : u64) : Result<std::Unit, std::io::IoError> {
        if(self.ctx == null) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        unsafe {
            net::raw::set_recv_timeout(self.ctx.transport_socket,
                                  (ms / 1000u) as long,
                                  ((ms % 1000u) * 1000u) as long)
        }
        return Result.Ok(std::Unit { })
    }

    public func close_notify(&mut self) : Result<std::Unit, std::io::IoError> {
        if(self.ctx == null) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        var ret = raw::ssl_close_notify(self.ctx)
        if(ret < 0) { return Result.Err(io_error(ret)) }
        return Result.Ok(std::Unit { })
    }

    public func alpn_protocol(&self) : Option<string_view> {
        if(self.ctx == null) { return Option.None<string_view>() }
        var neg = unsafe(self.ctx.alpn_negotiated)
        if(neg == null) { return Option.None<string_view>() }
        return Option.Some<string_view>(unsafe(string_view(neg, unsafe(self.ctx.alpn_negotiated_len))))
    }

    public func peer_certificate(&self) : Option<Certificate> {
        if(self.peer.ptr == null) { return Option.None<Certificate>() }
        return Option.Some<Certificate>(Certificate {
            ptr : self.peer.ptr,
            owns : false,
            cn : unsafe(self.peer.cn.copy())
        })
    }
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
    func try_read_to_end(&mut self, out : &mut vector<u8>) : Result<size_t, std::io::IoError> {
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

/// Owning server-side TLS stream.
public struct TlsServerStream {
    internal var ctx : *mut raw::SSLContext
    internal var peer : Certificate

    @delete
    func delete(&mut self) {
        if(self.ctx != null) {
            raw::ssl_free(self.ctx)
            unsafe { dealloc self.ctx }
            self.ctx = null
        }
        self.peer = Certificate { ptr : null, owns : false, cn : string() }
    }

    public func into_box(stream : &mut TlsServerStream) : *mut TlsServerStream {
        var p = unsafe(malloc(sizeof(TlsServerStream))) as *mut TlsServerStream
        if(p == null) { return null }
        unsafe { *p = TlsServerStream { ctx : null, peer : Certificate { ptr : null, owns : false, cn : string() } } }
        unsafe { p.ctx = stream.ctx }
        unsafe { p.peer = stream.peer }
        stream.ctx = null
        stream.peer = Certificate { ptr : null, owns : false, cn : string() }
        return p
    }

    func read_bytes(&mut self, buf : &mut [u8]) : Result<size_t, std::io::IoError> {
        if(self.ctx == null) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        if(buf.size() > 2147483647 as size_t) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        var n = raw::ssl_read(self.ctx, buf.data(), buf.size() as i32)
        if(n < 0) { return Result.Err(io_error(n)) }
        return Result.Ok(n as size_t)
    }

    func write_bytes(&mut self, data : &[u8]) : Result<size_t, std::io::IoError> {
        if(self.ctx == null) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        if(data.size() > 2147483647 as size_t) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        var n = raw::ssl_write(self.ctx, data.data(), data.size() as i32)
        if(n < 0) { return Result.Err(io_error(n)) }
        return Result.Ok(n as size_t)
    }

    public func set_read_timeout(&mut self, ms : u64) : Result<std::Unit, std::io::IoError> {
        if(self.ctx == null) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        unsafe {
            net::raw::set_recv_timeout(self.ctx.transport_socket,
                                  (ms / 1000u) as long,
                                  ((ms % 1000u) * 1000u) as long)
        }
        return Result.Ok(std::Unit { })
    }

    public func close_notify(&mut self) : Result<std::Unit, std::io::IoError> {
        if(self.ctx == null) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        var ret = raw::ssl_close_notify(self.ctx)
        if(ret < 0) { return Result.Err(io_error(ret)) }
        return Result.Ok(std::Unit { })
    }

    public func alpn_protocol(&self) : Option<string_view> {
        if(self.ctx == null) { return Option.None<string_view>() }
        var neg = unsafe(self.ctx.alpn_negotiated)
        if(neg == null) { return Option.None<string_view>() }
        return Option.Some<string_view>(unsafe(string_view(neg, unsafe(self.ctx.alpn_negotiated_len))))
    }

    public func peer_certificate(&self) : Option<Certificate> {
        if(self.peer.ptr == null) { return Option.None<Certificate>() }
        return Option.Some<Certificate>(Certificate {
            ptr : self.peer.ptr,
            owns : false,
            cn : unsafe(self.peer.cn.copy())
        })
    }
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
            if(n == 0) {
                return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::UnexpectedEof()))
            }
            pos += n
        }
        return Result.Ok(pos)
    }
    func try_read_to_end(&mut self, out : &mut vector<u8>) : Result<size_t, std::io::IoError> {
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

/// Accept a TLS connection on an already-connected TCP stream. Ownership of the
/// socket moves into the returned server stream.
@retained
public func <'a> accept(stream : net::TcpStream, cfg : &'a TlsServerConfig<'a>) : std::Result<TlsServerStream, TlsError> {
    if(cfg.cert.ptr == null || cfg.key.ptr == null) {
        return std::Result.Err<TlsServerStream, TlsError>(TlsError.InvalidConfig(make_string("server certificate or key missing")))
    }
    var fd = unsafe(net::raw::release_tcp(&mut stream))
    var ctx = raw::tls_accept(fd, cfg.cert.ptr, cfg.key.ptr, cfg.cipher_suite)
    if(ctx == null) {
        return std::Result.Err<TlsServerStream, TlsError>(TlsError.HandshakeFailure(make_string("TLS server handshake failed")))
    }
    return std::Result.Ok<TlsServerStream, TlsError>(TlsServerStream { ctx : ctx, peer : peer_from_ctx(ctx) })
}

@retained
public func <'a> wrap_server(stream : net::TcpStream, cfg : &'a TlsServerConfig<'a>) : std::Result<TlsServerStream, TlsError> {
    return accept(stream, cfg)
}

// Compatibility helpers for the pre-canonical client surface.
public func connect(host : &str, port : u16) : Result<TlsStream, TlsError> {
    var cfg = TlsConfig.client()
    var ca = load_system_ca_bundle()
    if(ca is Result.Err) {
        var Err(e) = ca else unsafe { unreachable }
        return Result.Err(e)
    }
    var Ok(chain) = ca else unsafe { unreachable }
    var set = cfg.set_ca_chain(&chain)
    if(set is Result.Err) {
        var Err(e) = set else unsafe { unreachable }
        return Result.Err(e)
    }
    return TlsStream.connect(host, port, &cfg)
}

public func connect_insecure(host : &str, port : u16) : Result<TlsStream, TlsError> {
    var cfg = TlsConfig.client()
    cfg.insecure_skip_verify()
    return TlsStream.connect(host, port, &cfg)
}

} // namespace tls
