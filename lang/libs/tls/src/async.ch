// Async TLS (design §7 Tier 2).
//
// Additive: every synchronous function in `ssl.ch` is unchanged. The handshake
// is multi-RTT and record I/O blocks on the network, so each entry point
// offloads the blocking work to the runtime thread pool via
// `async::spawn_blocking`. The executor keeps running, and many handshakes can
// overlap (bounded by the pool size).
//
// A genuinely non-blocking TLS state machine would need the transport to
// suspend mid-handshake; the synchronous library is not structured for that, so
// v1 concurrency comes from the pool. Pointers/ints only — no struct payloads —
// so this avoids B23 (`FutureHandle<StructT>` on LLVM) and B24 (struct-typed
// async parameters on 2c).
public namespace tls {

// Connect + handshake. Returns 0 on success, <0 on failure (`tls_connect`).
public async func tls_connect_async(ssl : *mut SSLContext, host : *char, port : uint) : int {
    return await async::spawn_blocking<int>(|ssl, host, port|() => tls_connect(ssl, host, port))
}

public async func ssl_handshake_async(ssl : *mut SSLContext) : int {
    return await async::spawn_blocking<int>(|ssl|() => ssl_handshake(ssl))
}

public async func ssl_read_async(ssl : *mut SSLContext, buf : *mut u8, len : i32) : int {
    return await async::spawn_blocking<int>(|ssl, buf, len|() => ssl_read(ssl, buf, len))
}

public async func ssl_write_async(ssl : *mut SSLContext, data : *u8, len : i32) : int {
    return await async::spawn_blocking<int>(|ssl, data, len|() => ssl_write(ssl, data, len))
}

// Server-side: accept + handshake on an already-accepted socket. Returns the
// new `SSLContext` (null on failure).
public async func tls_accept_async(sock : net::Socket, cert : *mut X509Cert, priv_key : *mut void) : *mut SSLContext {
    return await async::spawn_blocking<*mut SSLContext>(|sock, cert, priv_key|() => tls_accept(sock, cert, priv_key))
}

}
