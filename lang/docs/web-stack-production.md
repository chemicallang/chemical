# Web stack: production readiness (2026-11-22)

The question this answers: **can the stack serve real traffic today, and where would it hurt
first?** It is a checklist, not an opinion — each row names how it was verified (a test, a
measurement, or the source that implements it). The functional findings behind it live in
`web-stack-gaps.md`; the plan is `web-stack-plan.md`.

Scope: a single-process HTTP/1.1 server rendering Chemical pages and serving build artifacts.
Multi-process clustering, HTTP/2 and edge caching are deliberately out of scope.

## Verdict

**Ready for a production deployment**: marketing sites, docs, authenticated dashboards,
form-driven apps, file uploads and an SPA with a typed client. Sessions can be kept in a
directory shared by every process on the host (or a shared volume), every request can be logged,
and pages can be compressed on the wire.

**What is still missing is scale-out beyond shared storage** (sessions are files, not Redis, so
a multi-host fleet needs a volume every host can write) and **per-request panic containment** —
build the serving module `[no_panic]` and the compiler proves it instead. Panic isolation is the
last checklist row open; it is written out under "The one thing left" below.

## Readiness checklist

| # | A deployment needs | Status | Evidence |
|---|---|---|---|
| 1 | Correct keep-alive, keep it fast | **done** | `TCP_NODELAY` on accepted sockets; bench: 25,000 req/s keep-alive vs 48 req/s before; `web-probes-test.sh bench` fails above 5 ms p50 |
| 2 | `Date` on every response (RFC 9110 §6.6.1) | **done** | `http::format_http_date` + per-second cache (`DateStamp`); `PROD_every_response_carries_a_valid_date`, `PROD_date_formatter_matches_known_instants` |
| 3 | `Range` / resumable downloads | **done (in-memory bodies)** | `http::parse_byte_range` + `apply_byte_range`; `PROD_range_answers_partial_content`, `PROD_range_parser_handles_the_forms` |
| 4 | Say *why* a request was refused | **done** | 413 / 431 / 408 / 400 from `status_for_error`; oversized complete heads now rejected (`parser.ch`); `PROD_oversized_body_is_413`, `PROD_oversized_head_is_431` |
| 5 | Bound request size / timeouts (slowloris) | **done** | `ServerConfig.max_header_bytes` 64 KiB, `max_headers` 512, `max_body_bytes` 10 MiB, header/body timeouts enforced per message |
| 6 | Compression | **done (opt-in for renders)** | artifacts: gzip at build + negotiation + `Vary`. Renders: `App.set_dynamic_gzip(true)` compresses a `Text` body ≥ 1 KiB when `Accept-Encoding` accepts gzip, sets `Content-Encoding` + `Vary`, and keeps the body only when it is smaller; measured 2,589 → 483 bytes for 665 µs of CPU (below) |
| 7 | Caching correctness | **done** | `no-cache` + `ETag` + `If-None-Match` → 304 for frozen pages/artifacts, `no-store` for per-request renders (policy is per route) |
| 8 | Sessions + CSRF | **done** | `SessionStore` (in-memory + snapshot load/save), `HttpOnly`/`SameSite=Lax`, optional `Secure` (`SessionStore.make(name, secure)`); `ctx.rotate_session()` + `ctx.rotate_csrf()` at a privilege change (session fixation), proven by the `tasks` probe: the sign-in re-sends the cookie and the pre-login id stops signing in |
| 9 | TLS | **done** | `ServerConfig.cert_file`/`key_file` → `tls::TlsServerStream` per connection |
| 10 | Graceful shutdown | **done** | `Server.handle()` → `ShutdownHandle.shutdown()`; connection loop exits, workers join |
| 11 | XSS / injection safety | **done** | `{expr}` escapes once in text and attributes, `{!expr}` is explicit raw, script/style bodies stay raw text; plugin suite pins it. Header injection is refused at the wire: a response whose header name, header value or status text carries a control character is answered `500` + close instead of being split (`PROD_unsafe_response_head_is_refused`), and `Client.request` refuses a request head the same way (`PROD_unsafe_request_head_is_refused`) |
| 12 | Typed client code shipped by the server | **done** | `app.add_client_module()` publishes the `--web` bundle + module script; `spa/client_boot_check.mjs` boots it in Node |
| 13 | A way to observe a request | **done** | `ServerConfig.access_log`: `AccessLogger.combined()` (Common Log Format + duration on stderr) or `AccessLogger.make(sink)` for any destination; the entry carries peer, method, target, status, bytes and the wall time of the whole exchange. Off by default and one branch when off |
| 14 | Sessions that survive a restart / a second process | **done (shared directory)** | `SessionStore.make_durable(dir)` / `app.durable_sessions(dir)`: one record file per session, written atomically (temp + rename) on every change and re-read per request, so another process over the same directory sees it. `set_ttl` (idle timeout) + `sweep_expired()`. Cross-process proof: `web-app-test.sh` "a durable session survives in a process that only has the directory". No Redis/DB backend — a shared volume is the cross-host story |
| 15 | Multipart uploads (files) | **done** | `http::parse_multipart` / `req.multipart()`: parts with `name`/`filename`/`Content-Type`, bodies kept byte-exact (NUL, CR, LF), `max_parts` bound, `MissingBoundary`/`InvalidBoundary`/`MalformedPart`/`TooManyParts` errors, `fields()` for the text fields. `MP_text_field_and_binary_file_survive` and five more in `--server` |
| 16 | Ranges for streamed files | **done** | `apply_byte_range` seeks a `File` body and shortens it, so the writer streams the slice (no read into memory): `206` + `Content-Range` + the slice's length, `416` past the end. `http::FileServer` (the `server.serve` static path) also sends `ETag` + `Last-Modified` and answers `If-None-Match` with `304` *without opening the file*; `PROD_file_body_serves_ranges_and_validators` |
| 17 | Panic isolation | **missing** | a panicking handler follows the module's panic policy; there is no per-request unwind boundary. Not needed in a `[no_panic]` module, where the compiler proves a handler cannot panic |

## What "ready" means in numbers

Same host, one thread, loopback, TCC backend, `--mode debug_quick`, single-threaded client
(`lang/tests/web/probes/bench`, idle machine):

| route | `app.handle` p50 | new connection p50 | keep-alive p50 |
|---|---|---|---|
| frozen page | 3 µs | 63 µs | 38 µs |
| frozen shell + 2 fills | 5 µs | 66 µs | 44 µs |
| per-request 24-row render | 31 µs | 100 µs | 73 µs |

The render row is 3 µs faster than it was because the document is now *moved* into the
response instead of copied twice (`render_dynamic` copied it into its result, `response_of`
into the body) — A/B over alternating binaries, 4 runs each: 32/32/33/34 → 31/31/31/32 µs p50.
A microsecond saved per allocation is only worth claiming because it was measured; the
remaining time is the 24 rows themselves, which is the app's cost, not the framework's.

The framework is microseconds; the network shape is what costs. A page with ten assets that
does not reuse connections pays ~600 µs of accept/close for nothing, which is why keep-alive
plus `TCP_NODELAY` is a readiness item rather than a tuning knob.

## The one thing left before a public deployment

**Per-request panic containment (17).** A single bad request must not take the process down on a
deployment that allows panics. Either build the serving module `[no_panic]` (then the compiler
proves the absence of panics, which is the stronger answer) or wait for an unwind boundary around
the handler.

File ranges (16), access logging, dynamic gzip, durable sessions and multipart uploads were the
previous entries on this list and are all in now (rows 6, 13, 14, 15, 16). Gzip is opt-in for a
reason, so the trade-off is written down:

| route (`lang/tests/web/probes/bench`, `/live`, 24 rows) | body | p50 with `app.handle` |
|---|---|---|
| no compression | 2,589 B | 31 µs |
| `Accept-Encoding: gzip`, `set_dynamic_gzip(true)` | 483 B | 665 µs |

5.4× fewer bytes for ~630 µs of CPU per request. That is a good trade on a slow link or a
mobile client and a bad one on a warm LAN, and it is why the default is off: turn it on when
bandwidth costs more than CPU, or compress at the proxy instead. The encoder is the repo's own
pure-Chemical fixed-Huffman deflate (`compression::gzip`), so making it cheap is a codec job —
table-driven CRC and precomputed codes, or a zlib binding — not a web-stack one.

## Ambiguous framing is refused, not guessed

Two HTTP implementations that frame one message differently leave the remainder in the
connection, where the *next* request is read — an attacker's request, carrying the server's
trust. So `http::read_request_body` rejects (400 + close) instead of choosing:

| request shape | why it is refused |
|---|---|
| `Transfer-Encoding` **and** `Content-Length` | a proxy may prefer the length, this server the codings |
| `Transfer-Encoding` that is not exactly `chunked` | falling back to a length would be a guess |
| two `Content-Length` fields | header lookup would silently pick the first |

A single unambiguous `Content-Length` or a plain `chunked` body is unchanged. Pinned by
`PROD_ambiguous_request_framing_is_refused`.

## Deliberately not on the list

- **HTTP/2**: needs a different connection and framing model; a reverse proxy terminates it.
- **WebSocket/SSE**: `dom` has no typed surface for either yet; `#js` is the escape hatch.
- **Image pipeline (resize/srcset)**: a build-time concern, not a serving one.
- **i18n/RTL**: app-level today; the escaping contract makes it safe to do by hand.

## Configuration knobs

```chemical
var cfg = server.ServerConfig()
cfg.access_log = http.AccessLogger.combined()      // CLF + duration on stderr

// or route it yourself: the sink runs on the serving thread, so it owns its locking
cfg.access_log = http.AccessLogger.make(||(entry : &http.AccessEntry) => {
    var line = http.format_access_line(entry)
    write_to_my_log(&line)
})
```

```
127.0.0.1:51234 - - [22/Nov/2026:10:07:02 +0000] "GET /pricing HTTP/1.1" 200 2565 38us
```

The line is Common Log Format so existing analysers parse it, plus the request's wall time in
microseconds at the end. The method and target are escaped inside the quoted field (`\"`, `\\`,
`\xHH` for a control byte): the target is attacker-controlled, and an unescaped quote would let
it read as the status and size — a forged line in the operator's record. `-` stands in for a field the entry does not have. A request the
framework refused (413/431/408/400) is logged too — that is the line an operator actually wants
— except for a peer that closed an idle keep-alive connection, which is not a request at all.

```chemical
var app = web.App.make("out", "app", "assets")
app.set_gzip(true)            // artifacts: precompressed at build
app.set_dynamic_gzip(true)    // renders: compressed per request (see the cost above)
```

Both are negotiation-correct: a client that does not send `Accept-Encoding`, or sends `gzip;q=0`,
gets the uncompressed body, and every compressed response carries `Vary: Accept-Encoding` so a
shared cache cannot hand a compressed body to a client that did not ask for one.

## Sessions that outlive the process

```chemical
// Sessions that outlive the process and are shared with the other workers.
var app = web::App.make("out", "app", "assets")
app.durable_sessions("var/sessions")   // private directory, shared by every process
app.session_store().set_ttl(86400)     // idle timeout; 0 (default) never expires
```

The directory holds one `<id>.sess` file per session: `<last_seen> <csrf_hex>` then one
`<key_hex>=<value_hex>` line per value. It is written to a temp file and renamed, so a crash
during a write loses that one session, not the store. `load` re-reads the record on every
request, which is what makes a second process see the first one's writes — budget one small
file read per request that carries a cookie. An id that is not hex is refused before it can
become a path (`../../etc/passwd` is not a session).

## File uploads

```chemical
// A file upload, in a route:
var parsed = ctx.req().multipart()          // Result<MultipartForm, MultipartError>
if(parsed is Result.Err) { return Result.Err(http::HttpError.BadRequest()) }
var Ok(form) = parsed else unsafe { unreachable }
var caption = form.field("caption")          // Option<string_view>
var upload = form.file("photo")              // *mut MultipartPart, null when absent
if(upload != null) {
    var name = upload.filename_or_empty()    // what the client called it: never a path
    var kind = upload.content_type_or_empty()
    var bytes = upload.data.as_span()        // byte-exact
    save_upload(name, kind, bytes)           // the app decides where it may land
}
```

`multipart` returns `Err` for anything it cannot parse rather than a partial form, so a handler
never saves half an upload and reports success. The body is already bounded by
`ServerConfig.max_body_bytes`, and `max_parts` (default 1000) bounds how many parts one body may
split into. Streaming a part straight to disk is not implemented; a part is held in memory.
