# net/http Library Separation Plan

> **Status: Design Proposal**
> 
> Separates `lang/libs/net/` into two independent modules:
> - `lang/libs/net/` — core networking primitives (sockets, addresses, I/O buffer)
> - `lang/libs/http/` — HTTP protocol types, client, server, router

---

## 1. Current State

All networking code lives under `lang/libs/net/`:

```
net/
  chemical.mod          module net, imports cstd + std, platform sources
  posix/platform_api.ch  POSIX socket externs
  win/platform_api.ch    Windows socket externs + IOCP
  win/iocp.ch            IOCP API
  src/
    api.ch               net::in_addr, net::sockaddr_in, extern inet_pton
    main.ch              net::Socket type, socket ops (listen, accept, recv, send,
                          close, dial), timeouts, keepalive, getaddrinfo
    io.ch                net::io::Buffer — byte buffer for reading/writing
    http.ch              net::http::Request, Response, Body, HeaderMap, QueryMap,
                          ResponseWriter, URL decoding, request/response parsers,
                          chunked transfer encoding
    client.ch            net::Client, net::http::RequestBuilder, net::http::URL
    server.ch            net::server::Server, ServerConfig
    web.ch               net::web::Router, Route, Handler
    static.ch            static file serving via sendfile/TransmitFile
```

### Current Namespace Layout

| Namespace | Contains | Nature |
|-----------|----------|--------|
| `net::` | Socket, in_addr, sockaddr_in, listen_addr, accept_socket, recv_all, send_all, close_socket, dial, set_recv_timeout, set_keep_alive, htons_port | Core networking |
| `net::io::` | Buffer | I/O utility |
| `net::http::` | Request, Response, Body, HeaderMap, QueryMap, ResponseWriter, URL decoding, request/response parsing, chunked encoding, strcasecmp, hex_value | HTTP protocol |
| `net::server::` | Server, ServerConfig | HTTP server |
| `net::web::` | Router, Route, Handler, Middleware | Web routing |
| `net::` | Client, URL, RequestBuilder | HTTP client |

### Current Consumers

| Consumer | What it uses |
|----------|-------------|
| `lang/tests/src/libs/net/net_test.ch` | `net::Client()` — HTTP client tests |
| `lang/compiled/emaillib/src/EmailProvider.ch` | `net::TlsClient()` — TLS email |
| `lang/compiled/secure_net/` | `net::TlsClient()`, `http::URL::parse` — secure HTTP layer |
| `lang/libs/page/src/page.ch` | CDN imports (unrelated) |
| `lang/compiled/*/` | Various apps use net for serving |

---

## 2. Target State

### `lang/libs/net/` — Core Networking Primitives

```
net/
  chemical.mod          module net, source "src", source "posix"/"win"
  posix/platform_api.ch  same as before
  win/platform_api.ch    same as before
  win/iocp.ch            same as before
  src/
    api.ch               net::in_addr, net::sockaddr_in, extern inet_pton
    main.ch              net::Socket type, listen, accept, recv, send, close,
                          dial, timeouts, keepalive, getaddrinfo
    buffer.ch            net::Buffer (was net::io::Buffer, simplified path)
    compat.ch            [Optional] Re-exports for APIs that moved to http
```

**What stays in `net`:** Everything socket-level. No HTTP concepts.

**Key changes:**
- `net::io::Buffer` → `net::Buffer` (simplify namespace)
- Remove `io.ch` (rename to `buffer.ch` with cleaner namespace)
- No changes to socket API signatures

### `lang/libs/http/` — HTTP Protocol & Server/Client (NEW)

```
http/
  chemical.mod          module http, source "src", import cstd + std + net
  build.lab              build script for the http module
  src/
    types.ch             http::Request, Response, Body, HeaderMap, QueryMap
    writer.ch            http::ResponseWriter
    client.ch            http::Client, http::URL, http::RequestBuilder
    server.ch            http::server::Server, ServerConfig
    router.ch            http::web::Router, Route, Handler (was net::web)
    static.ch            Static file serving (send_file moved here)
    parser.ch            Request/response parsers, chunked encoding, URL decoding
```

**What moves to `http`:**

| Symbol | Old Path | New Path |
|--------|----------|----------|
| `http::Request` | `net::http::Request` | `http::Request` |
| `http::Response` | `net::http::Response` | `http::Response` |
| `http::Body` | `net::http::Body` | `http::Body` |
| `http::HeaderMap` | `net::http::HeaderMap` | `http::HeaderMap` |
| `http::QueryMap` | `net::http::QueryMap` | `http::QueryMap` |
| `http::ResponseWriter` | `net::http::ResponseWriter` | `http::ResponseWriter` |
| `http::URL` | `net::http::URL` | `http::URL` |
| `http::RequestBuilder` | `net::http::RequestBuilder` | `http::RequestBuilder` |
| `net::Client` | `net::Client` | `http::Client` |
| `net::server::Server` | `net::server::Server` | `http::server::Server` |
| `net::server::ServerConfig` | `net::server::ServerConfig` | `http::server::ServerConfig` |
| `net::web::Router` | `net::web::Router` | `http::web::Router` |
| `net::web::Route` | `net::web::Route` | `http::web::Route` |
| `net::web::Handler` | `net::web::Handler` | `http::web::Handler` |
| `net::web::Middleware` | `net::web::Middleware` | `http::web::Middleware` |

### Dependency Graph

```
http ──depends-on──► net ──depends-on──► cstd + std
  │                    │
  │                    └── platform (posix/win)
  │
  └── TLS (future): http could use a future net::TlsSocket
                    or a separate tls module
```

---

## 3. API Migration Path — Keeping It Intact

### User-facing changes

**Before (current):**
```chemical
import net

// Core networking
var sock = net::dial("example.com", 80)
net::send_all(sock, "GET /\r\n", 7)

// HTTP client
var client = net::Client()
var res = client.get("http://example.com/")

// HTTP server
var server = server::Server(server::ServerConfig{ ... })

// HTTP types
var req = net::http::Request()
var headers = net::http::HeaderMap()
```

**After (migrated):**
```chemical
import net
import http   // new import

// Core networking — unchanged
var sock = net::dial("example.com", 80)
net::send_all(sock, "GET /\r\n", 7)

// HTTP client — namespace change only
var client = http::Client()
var res = client.get("http://example.com/")

// HTTP server — namespace change only
var server = http::server::Server(http::server::ServerConfig{ ... })

// HTTP types — namespace change only
var req = http::Request()
var headers = http::HeaderMap()
```

**The migration for users is:**
1. Add `import http` (or replace `import net` with `import net` + `import http`)
2. Replace `net::http::` with `http::`
3. Replace `net::Client` with `http::Client`
4. Replace `net::server::` with `http::server::`
5. Replace `net::web::` with `http::web::`
6. Everything else (`net::Socket`, `net::dial`, etc.) stays the same

### Option A: Compatibility Module (Recommended)

Add `lang/libs/net/src/compat.ch` that re-exports the moved symbols:

```chemical
// lang/libs/net/src/compat.ch
// Compatibility re-exports for symbols that moved to the http module
// Deprecated: import http instead

public import http::Client as net::Client
public import http::Request as net::http::Request
public import http::Response as net::http::Response
public import http::server::Server as net::server::Server
// ... etc
```

This allows existing code with `import net` to keep compiling (with a deprecation warning). New code uses `import http`.

### Option B: No Compatibility (Clean Break)

Skip the compat module. Users update imports and namespaces once. Simpler in the long run.

**Recommendation: Option A** — the compat module is a small maintenance cost and prevents breaking existing compiled apps.

---

## 4. TLS Integration (Future)

The separation sets up a clean path for TLS:

### Architecture

```
http ──┬── depends on net for TCP sockets
       │
       ├── OR depends on tls for TLS sockets
       │
       └── (Same http::Client works with both)
```

### Option A: `net::TlsSocket` (TLS in `net` module)

Add TLS support directly to the `net` module as `net::TlsSocket`:

```chemical
// net gets a TlsSocket type wrapping a Socket with TLS
public struct TlsSocket {
    var inner: Socket;
    // TLS context
}

public func tls_connect(host: *char, port: uint) : TlsSocket
public func tls_recv(s: TlsSocket, buf: *mut u8, cap: usize) : int
public func tls_send(s: TlsSocket, data: *char, len: int)
```

The `http::Client` can then work with both:
```chemical
public func request(&self, ...) {
    if(url.scheme == "https") {
        var s = net::tls_connect(url.host.data(), url.port)
        // TLS I/O
    } else {
        var s = net::dial(url.host.data(), url.port)
        // plain TCP I/O
    }
}
```

**Pros:** Single module to import, simpler API.
**Cons:** `net` gains crypto dependencies (OpenSSL/mbedTLS/etc).

### Option B: Separate `tls` Module

Create `lang/libs/tls/` as an independent module:

```chemical
module tls
import net  // uses net::Socket

public struct TlsSocket {
    var inner: net::Socket
    // ...
}
```

The `http` module imports both:
```chemical
module http
import net
import tls  // optional, for HTTPS
```

**Pros:** Clean separation, `net` stays crypto-free.
**Cons:** More modules to manage.

**Recommendation: Option B** — TLS as a separate module, because:
- Keeps `net` dependency-free (no OpenSSL/libressl linkage unless needed)
- Aligns with the separation philosophy of this plan
- Users who don't need TLS don't pay for it
- Future-proof: `tls` can support DTLS, QUIC, etc. without bloating `net`

### How http::Client uses it

```chemical
// In http::client.ch (simplified):
func request(&self, req: &http::RequestBuilder) -> Result<Response, string> {
    var s: net::Socket
    if(req.url.scheme == "https") {
        var tls_sock = tls::connect(req.url.host.data(), req.url.port)
        if(tls_sock is Result.Err) { return ... }
        s = tls_sock.unwrap()  // TlsSocket converts to net::Socket?
    } else {
        s = net::dial(req.url.host.data(), req.url.port)
    }
    // ... send request, read response ...
}
```

---

## 5. Implementation Plan

### Phase 1: Create `http` Module

**Step 1: Create directory structure**
```bash
mkdir -p lang/libs/http/src
```

**Step 2: Create `chemical.mod`**
```chemical
module http
source "src"
import cstd
import std
import net

// No platform-specific sources — all platform-specific code stays in net
```

**Step 3: Create `build.lab`**
```chemical
import lab

func build(ctx: *mut BuildContext, exe_job: *mut LabJob) -> *mut Module {
    return ctx.get_module("http", exe_job)
}
```

**Step 4: Move HTTP source files**
- Copy HTTP-related code from `net/src/` to `http/src/`
- Change namespaces from `net::http::X` → `http::X`, `net::server::X` → `http::server::X`, `net::web::X` → `http::web::X`
- Change `net::Client` → `http::Client`
- All `net::` socket calls (recv_all, send_all, etc.) now go through the `import net` dependency

### Phase 2: Clean up `net` Module

**Step 5: Remove HTTP-specific files from net**
- Remove from `net/src/`: `http.ch`, `client.ch`, `server.ch`, `web.ch`, `static.ch`
- Rename `io.ch` → `buffer.ch` with namespace change `net::io::Buffer` → `net::Buffer`
- (Optional) Add `compat.ch` for backward compatibility

**Step 6: Update `net/chemical.mod`**
- Remove HTTP-specific source references (they no longer exist)

### Phase 3: Update Build System & Consumers

**Step 7: Update `lang/tests/build.lab`**
```chemical
import "@http/build.lab" as httpMod
```
- Add http module to the build
- Add http as a dependency for test modules that use it

**Step 8: Update all consumers**
- `lang/tests/src/libs/net/net_test.ch` — update imports
- `lang/compiled/secure_net/` — update imports
- `lang/compiled/emaillib/` — update imports
- Any compiled apps using `net::http::` or `net::Client`

**Step 9: Run tests**
```bash
./scripts/build.sh --tcc
./scripts/test.sh --tcc
```

### Phase 4: (Future) TLS Module

**Step 10: Create `lang/libs/tls/`**
- Wraps OpenSSL/BoringSSL/mbedTLS
- Provides `tls::connect()`, `tls::accept()`
- Depends on `net` module
- `http` module optionally imports it for HTTPS

### File Mappings

| Current File | New Location | Changes Required |
|-------------|-------------|------------------|
| `net/src/http.ch` | `http/src/types.ch` + `http/src/writer.ch` + `http/src/parser.ch` | Namespace: `net::http::` → `http::`. Socket calls get `net::` prefix |
| `net/src/client.ch` | `http/src/client.ch` | `net::Client` → `http::Client`, `net::http::URL` → `http::URL` |
| `net/src/server.ch` | `http/src/server.ch` | `net::server::` → `http::server::` |
| `net/src/web.ch` | `http/src/router.ch` | `net::web::` → `http::web::` |
| `net/src/static.ch` | `http/src/static.ch` | Move alongside server code |
| `net/src/io.ch` | `net/src/buffer.ch` | `net::io::Buffer` → `net::Buffer` |
| `net/src/api.ch` | `net/src/api.ch` | No change |
| `net/src/main.ch` | `net/src/main.ch` | No change (remove HTTP imports if any) |

---

## 6. Risk Assessment

| Risk | Mitigation |
|------|-----------|
| **Breaking existing apps**: `net::Client` → `http::Client` breaks all existing HTTP client code | Add compat shim in `net/compat.ch` or coordinate a migration window |
| **Circular dependency**: If `net` tries to re-export `http` symbols | `net` must never import `http`. Compat shim re-imports from `http` module, not from `net` |
| **Missing symbols**: Something in `net` accidentally depends on HTTP | grep for all `http::`, `server::`, `web::` references in net source before removing |
| **Build system**: `build.lab` wiring for new module | Follow existing pattern (see `lang/libs/json/build.lab`) |
| **Test breakage**: Tests directly reference `net::http::` or `net::Client` | Update all test imports as part of the migration |

---

## 7. Verification Checklist

- [ ] `./scripts/build.sh --tcc` compiles cleanly
- [ ] `./scripts/build.sh --llvm` compiles cleanly (if applicable)
- [ ] `./scripts/test.sh --tcc` — all existing tests pass
- [ ] `./scripts/test.sh --tcc --plugins` — library tests pass
- [ ] `lang/compiled/*` apps compile and work
- [ ] HTTP client (`http::Client.get()`) works end-to-end
- [ ] HTTP server (`http::server::Server`) starts and handles requests
- [ ] `net::Buffer` (was `net::io::Buffer`) works for raw socket I/O
- [ ] (Optional) `import net` + `net::Client` works via compat shim

---

## 8. Summary

| Metric | Value |
|--------|-------|
| Files moved | 5 (http.ch, client.ch, server.ch, web.ch, static.ch) |
| Files modified | 3 (net/chemical.mod, io.ch→buffer.ch, optional compat.ch) |
| Files created | 6+ (http/*, build.lab) |
| Namespace changes | 4 (`net::http::` → `http::`, `net::server::` → `http::server::`, `net::web::` → `http::web::`, `net::Client` → `http::Client`) |
| Consumer updates | ~5 files (tests, compiled apps) |
| Risk level | Low with compat shim, Medium without |
