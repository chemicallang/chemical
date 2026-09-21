// Async TLS (design §7 Tier 2).
//
// Additive: every synchronous function in `ssl.ch` keeps its exact signature
// and behaviour. Internally the record layer is now a coroutine over a shared
// `Transport` abstraction (TLS-VT): the synchronous API drives it with
// `async::block_on` and the *blocking* transport (so it behaves exactly like the
// old direct `net::send_all`/`recv_all` calls), while these entry points install
// the *non-blocking* transport so the handshake and record I/O suspend on fd
// readiness instead of occupying a thread-pool thread. Many handshakes can then
// overlap on a single executor.
//
// On Windows `net::recv_async`/`send_async` still fall back to the thread pool
// until the IOCP reactor lands (WIN-IOCP); the connect itself is an inherently
// blocking kernel call and is offloaded by `net::dial_async` on both platforms.
public namespace tls {

// Connect + handshake. Returns 0 on success, <0 on failure (`tls_connect`).
public async func tls_connect_async(ssl : *mut SSLContext, host : *char, port : uint) : int {
    ssl_use_async_transport(ssl)
    return await tls_connect_coro(ssl, host, port)
}

public async func ssl_handshake_async(ssl : *mut SSLContext) : int {
    ssl_use_async_transport(ssl)
    return await ssl_handshake_coro(ssl)
}

public async func ssl_read_async(ssl : *mut SSLContext, buf : *mut u8, len : i32) : int {
    ssl_use_async_transport(ssl)
    return await ssl_read_coro(ssl, buf, len)
}

public async func ssl_write_async(ssl : *mut SSLContext, data : *u8, len : i32) : int {
    ssl_use_async_transport(ssl)
    return await ssl_write_coro(ssl, data, len)
}

// Server-side: accept + handshake on an already-accepted socket. Returns the
// new `SSLContext` (null on failure).
public async func tls_accept_async(sock : net::Socket, cert : *mut X509Cert, priv_key : *mut void) : *mut SSLContext {
    return await tls_accept_coro(sock, cert, priv_key, TLS_RSA_WITH_AES_128_GCM_SHA256, true)
}

}
