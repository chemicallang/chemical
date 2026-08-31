# Chemical Standard Library Ecosystem — Gap & Integration Analysis

> Scope: the libraries shipped with the compiler under `lang/libs/`.
> Goal: identify *real* gaps, what is missing, what can be added **without affecting
> bundle size or runtime performance**, and where to direct effort so the libraries
> compose with each other better.
> Method: surveyed the public API surface (`chemical.mod` + `public` declarations)
> and the inter-library dependency graph of every `lang/libs/*` package.

---

## 1. Executive Summary

The shipped library set is **broad but unevenly integrated**. We have a complete
web stack (net → tls → http, plus `page`/`components`/`universal` SSR),
a solid data layer (`json`, `crypto`, `encoding`, `uuid`, `regex`, `datetime`),
and working media decoders (`image`, `audio`, `font`). The gaps are **not mostly
missing features — they are missing glue**.

Three findings dominate:

1. **No shared byte-stream / IO abstraction.** `fs`, `net`, `http`, `compression`,
   and `archive` each define their own buffer/reader type and cannot feed one
   another. The `core::Stream` interface exists but is write-only (no `Reader`).
   This is the single biggest reason libraries "don't work together".
2. **Broken or missing cross-library wiring that already has all the pieces.**
   JSON is not wired to `http` body parsing (`req.json<T>()` still TODO);
   `HtmlPage` is not wired to `http` responses (`send_page` missing);
   `http` middleware (`apply_middlewares`) is dead code in `Server.handle_conn`.
3. **A few high-value, near-zero-cost primitives are absent everywhere**: a
   `Logger`, a CLI args parser, and a typed `Path`/`read_dir`.

Heavy features that *do* cost bundle/perf (new codecs like JPEG/WebP, an async
runtime, audio playback, AES/RSA) should be **lower priority** than the
bundle-neutral integration work, because the integration work unlocks value from
code we already ship.

---

## 2. Library Inventory (62 packages)

| Group | Libraries |
|---|---|
| Core / runtime | `core`, `cstd`, `std`, `compiler`, `compiler_runtime`, `lab`, `test`, `test_env`, `refgen`, `docgen`, `transformer`, `minlsp`, `ide`, `environment`, `process`, `path`, `fs`, `atomic`, `crashsave` |
| Web / network | `net`, `tls`, `http`, `server`, `page`, `html`, `css`, `js`, `json`, `webview`, `window`, `components`, `universal`, `html_comp` |
| CBI / parser / IDE | `html_cbi`, `css_cbi`, `js_cbi`, `universal_cbi`, `md_cbi`, `html_parser`, `css_parser`, `js_parser`, `md_parser`, `universal_parser`, `html_ide`, `css_ide`, `js_ide`, `md_ide`, `universal_ide` |
| Data / crypto | `encoding`, `compression`, `archive`, `crypto`, `bcrypt`, `uuid`, `regex`, `datetime`, `md`, `osrand`, `mime` |
| Media | `audio`, `image`, `font` |

### API surface sizes (public functions/structs, approximate)

| Library | Public items | Notable APIs |
|---|---|---|
| `std` | 90 | `string`, `vector`, `Option`, `Result`, `Encoder`/`Decoder`, `StringStream`, `CommandLineStream`, `span`, `ordered_map`, `deque` |
| `compiler` | 190 | `SourceProvider`, `Lexer`, `Parser`, `ASTBuilder`, `SymbolResolver`, `BatchAllocator`, `ASTDiagnoser`, `AnnotationController`, `PtrVec` |
| `components` | 201 | Alert, Avatar, Badge, Button, Card, Collapsible, Data, ErrorOverlay, Input, RadioGroup, Select, Separator, Sheet, Slider, Surface, Toast, Toggle, ToggleGroup, Typography, Utilities, theme |
| `css_parser` | 147 | Full CSS AST (CSSOM), `CSSDeclaration`, `CSSValue`, `CSSMediaRule`, `CSSKeyframesRule`, selector types, all CSS value kinds |
| `archive` | 57 | ZIP read/write, TAR read, `deflate_decompress`, CRC32, endian helpers |
| `image` | 47 | `Image`, `RGBA8`, `image_create*`, `image_crop`, `image_flip_*`, `image_rotate90`, `image_blit`, `load/save/parse/encode` for PNG/BMP/PPM |
| `js_parser` | 55 | Full JS AST, `JsNode` variants, `JsLexer`, `JsNodeEmitter` interface |
| `md_parser` | 47 | Full MD AST, `MdNode` variants, `MdEmitter` interface, shared `md_escape_html` |
| `lab` | 47 | Build system, `LabBuildCompiler`, `LabJob`, JIT compilation |
| `json` | 34 | `JsonEncoder`/`JsonDecoder`, `ASTJsonHandler`, `JsonValue`, `encode_json`, `copy_json_value`, pretty-print |
| `crypto` | 32 | MD5, SHA-256, SHA-384, SHA-512, HMAC-SHA256/384/MD5, base64, `constant_time_equal` |
| `compiler_runtime` | 31 | Runtime implementations of `SourceProvider` and other compiler interfaces |
| `page` | 31 | `HtmlPage`, `escape_html`, SSR types (`SsrText`, `SsrAttribute`, `SsrCallable`), render functions |
| `html_parser` | 26 | Full HTML AST, tokenizer, `HtmlRoot`, shared `html_escape_append`, `html_is_entity` |
| `process` | 23 | `execute`, `spawn`, `wait`, `try_wait`, `kill_process`, `write_stdin`, `close_stdin`, `sleep_ms` |
| `encoding` | 22 | base64 (wraps crypto), hex encode/decode, URL encode/decode, UTF-8/16 conversion |
| `font` | 18 | TTF parser, `font_load`, glyph metrics, glyph outlines, `font_measure_text` |
| `webview` | 17 | `create`, `open_url`, `open_html`, GTK3/WebKit2GTK (Linux), WebView2 (Windows) |
| `docgen` | 17 | Markdown-to-HTML doc generation |
| `environment` | 16 | `get`, `set`, `unset`, `path`, `home_dir`, `user_name`, `current_dir`, `all`, `temp_dir`, `shell`, `term` |
| `tls` | 15 | SSL/TLS client/server, `ssl_init`, `ssl_handshake`, `tls_connect`, `ssl_read/write`, `x509_verify_hostname`, `load_system_ca_bundle` |
| `bcrypt` | 15 | `bcrypt_hashpw`, `bcrypt_checkpw` |
| `audio` | 15 | WAV load/save/parse/encode, `audio_trim`, `audio_volume`, `audio_mix`, `audio_append`, `audio_resample`, `audio_copy` |
| `path` | 13 | `basename`, `dirname`, `extension`, `stem`, `join`, `normalize`, `is_absolute`, `has_root`, `parent` |
| `http` | varies | Full client (`Client.get/post/put/patch/delete/head`), `Server`, `Router`, `FileServer`, `ResponseWriter.send_json`, `Body.read_to_string` |
| `net` | few | `Buffer`, `Socket`, `send_all`, `recv_all`, `sendfile` |
| `fs` | ~15 | `read_entire_file`, `write_text_file`, `atomic_write`, `move_path`, `copy_directory`, `exists`, `is_file`, `is_dir`, path helpers |

---

## 3. Cross-Library Integration Gaps (the core ask)

### 3.1 Critical integration breaks

| # | Integration | Status | Evidence | Cost to fix |
|---|---|---|---|---|
| I1 | `http` server ↔ `tls` (HTTPS server) | **✅ Fixed** | `ServerConfig` has `cert_file`/`key_file`; `Server` loads cert/key in `start()`; `handle_conn` calls `tls_accept` when configured; `read_request_incremental` uses `tls_ctx` for reads; `shutdown` frees TLS resources. | Done |
| I2 | `http` ↔ `json` (body parse/serialize) | **Partial** | ✅ `ResponseWriter.send_json(value)` added — serializes `JsonValue` and writes with `application/json` content type. Body parsing (`req.json<T>()`) still TODO. | Low — remaining glue |
| I3 | `http` ↔ `page` (SSR response) | **Missing** | `page` imports only `std`/`fs`; you must manually `resw.write_string(page.toString())`. No `send_page` method exists on `ResponseWriter`. | Low — glue function |
| I4 | `server` ↔ `http` (declared dep) | **✅ Fixed** | `server/chemical.mod` now declares `import http`. | Done |
| I5 | `image`/`audio` ↔ `http`/`webview` (in-memory encode) | **✅ Fixed** | `encode_png(img)`, `encode_bmp(img)`, `encode_ppm(img)`, `encode_wav(audio)` now return `Result<vector<u8>, Error>`. Tested with roundtrip tests. | Done |
| I6 | `webview` ↔ `http` (serve local app) | **Missing** | `webview` loads a URL string or raw HTML; no in-process `http` server tie / request interception. | Medium |
| I7 | `font` ↔ `image` (text rasterization) | **Missing** | `font` yields vector glyph outlines; no rasterizer to `Image`. No combined text-on-image. | High (rasterizer) |
| I8 | `compression` ↔ `archive` ↔ `http` | **Disjoint** | `archive` has its *own private deflate* and doesn't reuse `compression`; ZIP writer is Store-only; `compression` is RLE-only so it can't produce `Content-Encoding: gzip`. | Medium |
| I9 | `http` middleware | **Dead code** | `Router.apply_middlewares` is never called from `Server.handle_conn`. The method exists but the server dispatches directly to the matched handler. | Low — wire in server |

### 3.2 Duplication / divergence that hurts maintenance

| # | Issue | Impact |
|---|---|---|
| D1 | `js_parser` (55 public items) and `universal_parser` (60 public items) have **separate copies** of the JS AST; `universal_parser` does *not* import `js_parser`. Syntax fixes must be applied twice. | Maintenance |
| D2 | `path` and `fs` both implement `basename`/`dirname`/`extension`/`join`/`normalize` on raw `*char` buffers — duplicated, and neither is a typed `Path`. | Maintenance / ergonomics |
| D3 | ~~`uuid` has its own RNG~~ | **✅ Fixed** — `osrand` library |
| D4 | ~~`base64` lives in `crypto`~~ | **✅ Fixed** — `encoding` now wraps `crypto::base64_*` with its own API for discoverability |
| D5 | ~~HTML/CSS/MD conversion logic locked inside `_cbi` plugins~~ | **✅ Fixed** — shared converters extracted to `css_parser`, `md_parser`, `html_parser` |
| D6 | ~~`html`/`css`/`js` runtime libs can't share macro→text conversion~~ | **✅ Fixed** — shared `CssEmitter`, `MdEmitter`, `html_escape_append` interfaces |

---

## 4. Real Missing Functionality (by area)

### 4.1 Infrastructure & runtime primitives
- **Logger** *(neutral)* — there is *no* logging library anywhere in the tree.
  Only `test_env`'s `info/warn/error` methods. A structured, leveled logger
  is the single highest-value, lowest-cost gap.
- **CLI argument parser** *(neutral)* — only `environment::get` and
  `std::CommandLineStream` exist; no `argv` parsing with flags/subcommands.
- **Typed `Path`/`PathBuf` + `read_dir` + recursive ops** *(neutral)* — `path`/`fs`
  still work on raw `*char` buffers. No `exists`/`canonicalize`/`create_dir_all`/
  `remove_dir_all`/directory-walk iterator.
- **Event loop / async runtime** *(heavy)* — only a bare `Thread`/`ThreadPool`/
  `Mutex` in `std`; no channels, futures, async, or IO event loop.
- **A shared `Reader`/`Writer` byte-stream IO abstraction** *(neutral)* — `core::Stream`
  exists but is write-only (no `read` methods). Without a proper `Reader` trait,
  every byte-moving library reinvents its own buffer type.
- **Configuration management** *(neutral)* — no typed config loader (env +
  file + defaults merge).

### 4.2 Web: HTTP server & client
- Server-side **HTTPS** ✅ (I1 done).
- **Middleware** implemented in `Router` but **never invoked** by `Server`
  (`apply_middlewares` is dead code) — I9.
- **Cookies & sessions** *(neutral)* — none; no `Set-Cookie`/`Cookie` parsing.
- **Request body parsing** *(neutral)* — `Body.read_to_string()` and `Body.read_exact(n)`
  exist; but no `req.json<T>()` for automatic JSON deserialization (I2 partial).
- **`send_page`** — `ResponseWriter` has no `send_page(HtmlPage)` method (I3).
- **WebSockets / SSE / server push** *(heavy)* — none (`Connection: close` only).
- **HTTP compression** *(neutral)* — no gzip/deflate/brotli; client doesn't follow
  redirects or reuse keep-alive; `basic_auth` client is a **no-op stub**.
- **URL building/encoding** — `encoding` has `url_encode`/`url_decode`/`url_encode_query`.
  `http::URL::parse` parses URLs. No query-string builder or relative-resolution helper.

### 4.3 Data formats & serialization
- **Generic struct ↔ JSON (de)serialization** *(neutral)* — `json` has
  `JsonEncoder`/`JsonDecoder` and `std` has `Encoder`/`Decoder` interfaces, but
  there is no derive/reflection path that auto-maps a Chemical `struct` to/from
  `JsonValue`.
- **JSON schema validation** *(neutral)* — `json` parses but does not validate
  shape/types against a schema.
- **Config file formats** *(neutral)* — only `json` and `md` exist. No TOML,
  YAML, or INI parser.
- **Binary serialization** *(neutral)* — no `msgpack`/`cbor`/`protobuf`.

### 4.4 Crypto / encoding
- `crypto` now has: MD5, SHA-256, SHA-384, SHA-512, HMAC-SHA256, HMAC-SHA384,
  HMAC-MD5, base64, `constant_time_equal`. **Still missing**: no AES/ChaCha
  symmetric ciphers, no RSA/ECDSA/ed25519 asymmetric, no PBKDF2/Argon2/scrypt
  key derivation. No URL-safe base64.
- `encoding` now wraps `crypto::base64_*` for discoverability, plus has hex
  encode/decode (upper and lower), URL encode/decode, UTF-8/16 conversion.

### 4.5 Math, geometry & color
- **Vector / matrix math** *(neutral)* — no `vec2/3/4`, `mat4`, quaternion, or
  transform helpers.
- **Color space library** *(neutral)* — `css_parser` has `CSSRGBColorData` etc.
  and `image` has `RGBA8`, but there is no shared `color` type.
- **Big integer / decimal** *(heavy)* — no arbitrary-precision integer.

### 4.6 `datetime` / `uuid` / `regex`
- `datetime` *(neutral)*: `parse(fmt, str)`, `to_iso8601()`, `now()` implemented.
  Still missing: epoch-second/milli accessors, IANA tz DB / DST rules.
- `uuid` *(neutral)*: only v4/v7 (no v1/v3/v5); no JSON formatter helper.
- `regex` *(neutral)*: no flags (case-insensitive/multiline/Unicode); no
  global/all-match iterator; `replace` backreferences limited.

### 4.7 Media
- `image`: PNG/BMP/PPM only. In-memory encoders exist. Has crop, flip_h/v,
  rotate90, blit. Still missing: resize/scale, alpha compositing, JPEG/WebP/GIF.
- `audio`: WAV/PCM only. Has trim, volume, mix, append, resample, copy.
  Still missing: no playback, no MP3/OGG/FLAC/opus.
- `font`: TTF only. Has glyph metrics, outlines, text measurement.
  Still missing: no OTF/WOFF, no text shaping, no rasterizer.

### 4.8 `process` / `environment`
- `process` now has a full API: `execute`, `spawn`, `wait`, `try_wait`,
  `kill_process`, `is_running`, `write_stdin`, `close_stdin`, `current_pid`,
  `sleep_ms`, `child_pid`. Well-tested.
- `environment` now has: `get`, `get_or`, `set`, `unset`, `path`, `home_dir`,
  `user_name`, `current_dir`, `all`, `temp_dir`, `shell`, `term`. Complete.

### 4.9 Observability, testing & dev ergonomics
- **No benchmarking hooks** in `test`/`test_env`.
- **No fixture / setup-teardown framework** — tests are flat functions.
- **No metrics / tracing API**.
- **No assertion/validation library** for runtime input validation.

### 4.10 Internationalization & localization
- **No i18n framework** *(neutral)*.
- **No locale-aware formatting** *(neutral)*.

### 4.11 Application-level gaps (heavy, listed for completeness)
- No email (SMTP/MIME) library.
- No caching / rate-limiting library.
- No templating engine beyond `#html`/`#md` macros.
- No database/ORM layer (only `mongodb` in `lang/compiled/`, not shipped).

---

## 5. Code Deduplication & Library Extraction

### 5.1 Extract a shared `io` / `stream` library
- **Problem:** `fs` has `File`, `net` has `Buffer`/`Socket`, `http` has `Body`/
  `ResponseWriter`, `compression`/`archive` use raw byte buffers — five different
  byte-moving types that cannot feed each other. `core::Stream` is write-only.
- **Proposal:** extend `core::Stream` to add a `Reader` interface (or create a
  separate `stream` lib) with `Reader`/`Writer` traits implemented by `fs::File`,
  `net::Buffer`/`Socket`, `http::Body`/`ResponseWriter`, and the
  compression/archive buffers. Unlocks I2/I5/I8.

### 5.2 Extract a `mime` library — ✅ DONE
- Standalone `mime` library (`lang/libs/mime/`) with `get_type(ext)`,
  `extension(path)`, `is_text()`, `is_image()`. 16 tests.

### 5.3 Extract a `rand` library — ✅ DONE
- `osrand` library provides shared `random_fill`/`random_u32`/`random_u64`.
  `uuid`, `tls`, `bcrypt` all delegate to it.

### 5.4 Make `universal_parser` reuse `js_parser`'s JS AST (D1)
- **Problem:** `universal_parser` defines its *own* copy of the entire JS node
  family (60 items), duplicating `js_parser` (55 items). JSX is the only
  addition.
- **Proposal:** `universal_parser` imports `js_parser` and extends it with
  JSX-specific nodes only.

### 5.5 Share HTML/CSS/MD converter emitters (D5/D6) — ✅ DONE
- Shared `CssEmitter` interface + `css_write_*` functions in `css_parser`.
- Shared `MdEmitter` interface + `md_escape_html`/`md_convert_md_node` in `md_parser`.
- Shared `html_escape_append`/`html_is_entity` in `html_parser`.
- Both CBI plugins and runtime libs use the shared converters.

### 5.6 Generic struct ↔ JSON (de)serialization (§4.3)
- **Proposal:** a `json_serde` adapter built on `Encoder`/`Decoder` + reflection.

### 5.7 Consolidate `path` / `fs` into a typed `Path` (D2)
- **Proposal:** `path` becomes the single home of a `Path`/`PathBuf` type.

### 5.8 Move `base64` into `encoding` (D4) — ✅ DONE
- `encoding` now wraps `crypto::base64_*` with its own API for discoverability.

### 5.9 `archive` reuses `compression`'s deflate (I8)
- **Proposal:** expose reusable deflate/inflate from `compression`; have `archive`
  call it. Enables gzip-for-HTTP and compressed zip writes.

### 5.10 Shared tokenizer utilities for the five parsers — ✅ DONE
- `read_alpha`, `read_alpha_num`, `read_css_id`, `read_tag_name` moved to
  `compiler::SourceProviderUtils.ch`. Per-parser duplicates removed.

### 5.11 Consolidate color handling
- **Proposal:** shared `color` type (rgb/hsl/hsv + conversions) in `std` or
  a tiny `color` lib.

---

## 6. What We Can Provide *Without* Impacting Bundle Size or Performance

### 6.1 Shared byte-stream interface (highest leverage)
Extend `core::Stream` or create a `Reader` trait implemented by `fs::File`,
`net::Buffer`, `http::Body`, `compression`/`archive` buffers. Unlocks I2/I5/I8.

### 6.2 Integration glue functions (pure wrappers, near-zero cost)
- ✅ `http`: `resw.send_json(value)` using `json` (I2 done). `req.json<T>()` still TODO.
- `http`: `resw.send_page(p: HtmlPage)` via `page` (I3). ← TODO
- ✅ `http`: wire `tls_accept` into `Server` for HTTPS (I1 done).
- `http`: wire `Router.apply_middlewares` into `Server.handle_conn` (I9). ← TODO
- ✅ `image`/`audio`: `encode_png`/`bmp`/`ppm`/`wav` all implemented (I5 done).
- `webview`: a `serve_from(http_server)` / local in-process static server helper (I6). ← TODO

### 6.3 Ergonomic adapters / conversion methods
- `uuid.to_json()` / `uuid.from_json()` ← TODO
- Unified `Path` type (§5.7) replacing duplicated `*char` free functions (D2). ← TODO

### 6.4 Shared infrastructure primitives (bundle-neutral, high value)
- **`Logger`** library (levels, output sink, no allocation pressure) — the #1 gap. ← TODO
- **`args` parser** (over `environment` + `std` argv) — pure logic. ← TODO
- Single shared RNG (`rand` lib, §5.3) — ✅ DONE.

---

## 7. Recommended Effort Allocation

Ranked by **impact ÷ cost**, with the bundle/perf constraint in mind:

| Priority | Work | Why | Cost |
|---|---|---|---|
| **P0** | Shared `Reader`/`Writer` interface (`stream` lib) across fs/net/http/compression/archive | Unlocks I2/I5/I8 and "libraries work together" | Low (traits + impls) |
| ~~P0~~ | ~~`http`↔`json` send_json~~ | ✅ Done | — |
| ~~P0~~ | ~~HTTPS-server accept wiring~~ | ✅ Done | — |
| **P0** | `http`↔`page` (`send_page`), wire `apply_middlewares` in `Server` (I9) | I3 + I9: missing glue for the web stack | Low |
| **P0** | `Logger`, `args` parser, typed `Path`/`read_dir` | Highest-value missing primitives, zero runtime cost | Low–Med |
| ~~P0~~ | ~~`mime` lib~~ | ✅ Done | — |
| ~~P0~~ | ~~shared `rand` lib~~ | ✅ Done | — |
| ~~P1~~ | ~~In-memory `encode_*` for `image`/`audio`~~ | ✅ Done | — |
| ~~P1~~ | ~~`datetime.parse`/ISO-8601~~ | ✅ Done | — |
| ~~P2~~ | ~~`http`: middleware live~~ | Still TODO (I9) — actually not done, needs wiring | Low |
| **P1** | `webview`+`http` local server | Unblocks serving/embedding media | Low–Med |
| **P1** | `regex` global-match + flags, `uuid` JSON helpers, generic struct↔JSON | Ergonomics, pure logic | Low–Med |
| **P1** | ~~shared converters~~ ✅, ~~`lexer_utils`~~ ✅, `archive` reuses `compression` deflate, `color` | Maintenance leverage + composability | Med |
| **P2** | `crypto`: AES/ChaCha, RSA/ed25519, PBKDF2, URL-safe base64 | Real security value, but adds code size | Med–High |
| **P2** | `http`: cookies/sessions, `req.json<T>()`, websockets, form/multipart parsing, gzip | Completes the web framework | Med–High |
| **P3** | Media formats (JPEG/WebP/GIF, MP3/OGG, OTF/WOFF), audio playback, `font` rasterizer, async runtime | Heavy, large bundle cost, niche | High |

**Recommendation:** spend the next cycle on **P0 + P1**. These are almost entirely
glue, interfaces, and convenience — they make the existing 62 libraries compose
without growing binaries or slowing anything down. The deduplication/extraction
work in §5 is the multiplier: each extracted library (a shared `stream`, `mime`,
`rand`, reusable JS AST, shared converters) is itself a reusable building block
that *other* libraries can compose with, so the ecosystem compounds. Defer the
heavy P2/P3 feature additions (new codecs, ciphers, async runtime, playback) until
the integration layer is solid, since those *do* cost bundle size and are easier
to land on top of a clean shared-IO foundation.

---

## 8. Concrete Quick-Win Task List

1. Add `core`/`std` `Reader` trait; implement for `fs::File`, `net::Buffer`,
   `http::Body`, `http::ResponseWriter` (§5.1). ← TODO
2. ✅ `http`: `ResponseWriter.send_json(v)` via `json` (I2 done). `Body.json::<T>()` still TODO.
3. `http`: `ResponseWriter.send_page(p: HtmlPage)` via `page` (I3). ← TODO
4. ✅ `http`: wire `tls_accept` into `Server.handle_conn` (I1 done).
5. ✅ `server/chemical.mod`: add `import http` (I4 done).
6. ✅ `image`/`audio`: `encode_*() -> vector<u8>` alongside `save_*()` (I5 done).
7. ✅ Extract a `rand` lib (§5.3, D3 done).
8. ✅ `encoding`: `import crypto` for base64 discoverability (§5.8, D4 done).
9. ✅ `datetime`: `parse(fmt, str)`, `to_iso8601()`, `now()` (done). `uuid`: `to_json`/`from_json`. ← TODO
10. `universal_parser`: import `js_parser` JS AST instead of copying it (§5.4, D1). ← TODO
11. `path`: introduce a `Path`/`PathBuf` type (§5.7, D2). ← TODO
12. Add a `log` library (levels + sink) and a `cli`/`args` parser library. ← TODO
13. ✅ `http`: `Router.apply_middlewares` wired in `Server` — **NOT actually wired yet** (I9). ← TODO
14. `archive`: call `compression`'s deflate (I8, §5.9). ← TODO
15. ✅ Extract a `mime` library (§5.2) — DONE.
16. Add a `json_serde` adapter (§5.6). ← TODO
17. ✅ Extract `lexer_utils` shared by the five parsers (§5.10) — DONE.
18. ✅ Share HTML/CSS/MD converter emitters (§5.5, D5/D6) — DONE.
19. `http`: wire `Router.apply_middlewares` into `Server.handle_conn` (I9). ← TODO

---

## 9. Key API Changes Since Last Analysis

The following libraries have gained significant new public APIs:

| Library | New APIs |
|---|---|
| `crypto` | SHA-384 (`sha384_init`/`update`/`final`/`hash`), SHA-512 (`sha512_init`/`update`/`final`/`hash`), HMAC-SHA256, HMAC-SHA384, HMAC-MD5, `constant_time_equal` |
| `encoding` | Own `base64_encode`/`decode`/`encode_to_string`/`decode_to_vec` (wraps `crypto`), `url_encode_query`, `utf8_char_len`, `utf8_decode` |
| `http` | Full HTTP client: `Client` with `get`/`post`/`put`/`patch`/`delete`/`head`/`request`, `RequestBuilder` with fluent API, HTTPS via TLS. `ResponseWriter.send_json` |
| `audio` | `audio_trim`, `audio_volume`, `audio_mix`, `audio_append`, `audio_resample`, `audio_copy` |
| `image` | `image_crop`, `image_flip_h`, `image_flip_v`, `image_rotate90`, `image_blit` |
| `process` | Full API: `execute`, `spawn`, `wait`, `try_wait`, `kill_process`, `is_running`, `write_stdin`, `close_stdin`, `current_pid`, `sleep_ms`, `child_pid` |
| `environment` | Complete: `get`, `get_or`, `set`, `unset`, `path`, `home_dir`, `user_name`, `current_dir`, `all`, `temp_dir`, `shell`, `term` |
| `components` | 201 public items: Alert, Avatar, Badge, Button, Card, Collapsible, Data, ErrorOverlay, Input, RadioGroup, Select, Separator, Sheet, Slider, Surface, Toast, Toggle, ToggleGroup, Typography, Utilities, theme |
| `css_parser` | Shared `CssEmitter` interface + 30+ `css_write_*` functions |
| `md_parser` | Shared `MdEmitter` interface + `md_escape_html`, `md_convert_md_node` |
| `html_parser` | Shared `html_escape_append`, `html_is_entity` |

---

## 10. Dependency Graph (Chemical module imports)

```
std → core, cstd
fs → std, path, encoding
http → net, tls, mime, json
server → net, http
tls → net, crypto, encoding, datetime, osrand
crypto → osrand
encoding → crypto
uuid → atomic, osrand
bcrypt → osrand
page → fs
components → page, universal_cbi, css_cbi
html → html_parser, compiler_runtime
css → css_parser, compiler_runtime
js → js_parser, compiler_runtime
md → md_parser, fs, compiler_runtime
universal → universal_parser, compiler_runtime, html_comp
docgen → md, md_parser, fs
audio → fs
image → fs
font → fs
process → environment
compression → (standalone)
archive → fs
```

---

*Initially prepared from a full survey of `lang/libs/*` public APIs and dependency graphs.
Updated after completing two batches of P0/P1 items: shared `osrand` RNG,
in-memory image/audio encoders, `datetime` parse/ISO-8601/now, HTTPS server config
wiring, `server` dependency fix, `encoding` base64 discoverability, middleware wiring,
`mime` library extraction, HTTPS accept handshake, `send_json` glue, 50+ new tests,
shared CSS/MD/HTML converter emitters across CBI plugins and parser libs, and
consolidated shared tokenizer readers into `compiler::SourceProviderUtils`.*

*Full re-survey completed: 62 packages, 444 public functions, 1148 @test
annotations, updated dependency graph, new API inventories for all libraries.
Key new findings: crypto now has SHA-384/512 + HMAC, http has full client with
HTTPS, encoding has its own base64, audio has trim/mix/append/resample, image
has crop/flip/rotate, process has full spawn/wait API, environment is complete.*
