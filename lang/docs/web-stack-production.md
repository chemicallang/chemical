# Web stack: production readiness (2026-11-22)

The question this answers: **can the stack serve real traffic today, and where would it hurt
first?** It is a checklist, not an opinion — each row names how it was verified (a test, a
measurement, or the source that implements it). The functional findings behind it live in
`web-stack-gaps.md`; the plan is `web-stack-plan.md`.

Scope: a single-process HTTP/1.1 server rendering Chemical pages and serving build artifacts.
Multi-process clustering, HTTP/2 and edge caching are deliberately out of scope.

## Verdict

**Ready for a single-node, TLS-terminated-at-the-proxy deployment**: marketing sites, docs,
authenticated dashboards, form-driven apps and an SPA with a typed client. **Not ready** for
large file/upload traffic (no multipart parsing, no `File`-body ranges), for multi-instance
deployments (sessions live in memory), or for anything needing an access log line per request
(there is no logging hook). Those are listed under "Not ready" with their blocking defect.

## Readiness checklist

| # | A deployment needs | Status | Evidence |
|---|---|---|---|
| 1 | Correct keep-alive, keep it fast | **done** | `TCP_NODELAY` on accepted sockets; bench: 25,000 req/s keep-alive vs 48 req/s before; `web-probes-test.sh bench` fails above 5 ms p50 |
| 2 | `Date` on every response (RFC 9110 §6.6.1) | **done** | `http::format_http_date` + per-second cache (`DateStamp`); `PROD_every_response_carries_a_valid_date`, `PROD_date_formatter_matches_known_instants` |
| 3 | `Range` / resumable downloads | **done (in-memory bodies)** | `http::parse_byte_range` + `apply_byte_range`; `PROD_range_answers_partial_content`, `PROD_range_parser_handles_the_forms` |
| 4 | Say *why* a request was refused | **done** | 413 / 431 / 408 / 400 from `status_for_error`; oversized complete heads now rejected (`parser.ch`); `PROD_oversized_body_is_413`, `PROD_oversized_head_is_431` |
| 5 | Bound request size / timeouts (slowloris) | **done** | `ServerConfig.max_header_bytes` 64 KiB, `max_headers` 512, `max_body_bytes` 10 MiB, header/body timeouts enforced per message |
| 6 | Compression | **partial** | build artifacts get gzip + negotiation + `Vary`; **dynamic responses are not compressed** |
| 7 | Caching correctness | **done** | `no-cache` + `ETag` + `If-None-Match` → 304 for frozen pages/artifacts, `no-store` for per-request renders (policy is per route) |
| 8 | Sessions + CSRF | **done** | `SessionStore` (in-memory + snapshot load/save), `HttpOnly`/`SameSite=Lax`, optional `Secure` (`SessionStore.make(name, secure)`) |
| 9 | TLS | **done** | `ServerConfig.cert_file`/`key_file` → `tls::TlsServerStream` per connection |
| 10 | Graceful shutdown | **done** | `Server.handle()` → `ShutdownHandle.shutdown()`; connection loop exits, workers join |
| 11 | XSS / injection safety | **done** | `{expr}` escapes once in text and attributes, `{!expr}` is explicit raw, script/style bodies stay raw text; plugin suite pins it |
| 12 | Typed client code shipped by the server | **done** | `app.add_client_module()` publishes the `--web` bundle + module script; `spa/client_boot_check.mjs` boots it in Node |
| 13 | A way to observe a request | **missing** | no access log, no per-request metrics hook |
| 14 | Multi-instance sessions | **missing** | `SessionStore` is process memory (+ a file snapshot); no Redis/DB backend |
| 15 | Multipart uploads | **missing** | `http::Form` parses `application/x-www-form-urlencoded` only |
| 16 | Ranges for streamed files | **missing** | `ResponseBody.File` streams whole; ranges need in-memory bodies |
| 17 | Panic isolation | **missing** | a panicking handler follows the module's panic policy; there is no per-request unwind boundary |

## What "ready" means in numbers

Same host, one thread, loopback, TCC backend, `--mode debug_quick`, single-threaded client
(`lang/tests/web/probes/bench`, idle machine):

| route | `app.handle` p50 | new connection p50 | keep-alive p50 |
|---|---|---|---|
| frozen page | 3 µs | 63 µs | 38 µs |
| frozen shell + 2 fills | 5 µs | 66 µs | 44 µs |
| per-request 24-row render | 34 µs | 103 µs | 77 µs |

The framework is microseconds; the network shape is what costs. A page with ten assets that
does not reuse connections pays ~600 µs of accept/close for nothing, which is why keep-alive
plus `TCP_NODELAY` is a readiness item rather than a tuning knob.

## The three things to do before a public deployment

1. **Access logging (13).** One line per request (method, path, status, bytes, duration) behooves
   any operator, and it is a small, additive hook on `ServerConfig`.
2. **Dynamic gzip (6).** Pages are the biggest thing on the wire after the bundle; compressing a
   rendered page above ~1 KiB is a 3–5× byte saving, and the negotiation code already exists for
   artifacts.
3. **Per-request panic containment (17).** A single bad request must not take the process down
   on a deployment that allows panics; with `[no_panic]` modules the compiler already proves the
   absence of panics, so this matters exactly for the modules that opt out.

## Deliberately not on the list

- **HTTP/2**: needs a different connection and framing model; a reverse proxy terminates it.
- **WebSocket/SSE**: `dom` has no typed surface for either yet; `#js` is the escape hatch.
- **Image pipeline (resize/srcset)**: a build-time concern, not a serving one.
- **i18n/RTL**: app-level today; the escaping contract makes it safe to do by hand.
