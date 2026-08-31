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
enough web stack (net → tls → http, plus `page`/`components`/`universal` SSR),
a solid data layer (`json`, `crypto`, `encoding`, `uuid`, `regex`, `datetime`),
and working media decoders (`image`, `audio`, `font`). The gaps are **not mostly
missing features — they are missing glue**.

Three findings dominate:

1. **No shared byte-stream / IO abstraction.** `fs`, `net`, `http`, `compression`,
   and `archive` each define their own buffer/reader type and cannot feed one
   another. This is the single biggest reason libraries "don't work together".
2. **Broken or missing cross-library wiring that already has all the pieces.**
   HTTPS *server* config loading is done (cert/key fields + PEM loader), but the
   accept handshake path is not yet wired; JSON is not wired to `http` bodies;
   `HtmlPage` is not wired to `http` responses. ✅ In-memory encoders for
   `image`/`audio` now exist (I5 done).
3. **A few high-value, near-zero-cost primitives are absent everywhere**: a
   `Logger`, a CLI args parser, and a typed `Path`/`read_dir`. ✅ The shared
   `osrand` RNG library now exists and is used by `uuid`, `tls`, `bcrypt`.
   These are pure-convenience and add no runtime cost.

Heavy features that *do* cost bundle/perf (new codecs like JPEG/WebP, an async
runtime, audio playback, AES/RSA) should be **lower priority** than the
bundle-neutral integration work, because the integration work unlocks value from
code we already ship.

---

## 2. Library Inventory (70 packages)

| Group | Libraries |
|---|---|
| Core / runtime | `core`, `cstd`, `std`, `compiler`, `compiler_runtime`, `lab`, `test`, `test_env`, `refgen`, `docgen`, `transformer`, `minlsp`, `ide`, `environment`, `process`, `path`, `fs`, `atomic`, `crashsave` |
| Web / network | `net`, `tls`, `http`, `server`, `page`, `html`, `css`, `js`, `json`, `webview`, `window`, `components`, `universal`, `html_comp` |
| CBI / parser / IDE | `html_cbi`, `css_cbi`, `js_cbi`, `universal_cbi`, `md_cbi`, `html_parser`, `css_parser`, `js_parser`, `md_parser`, `universal_parser`, `html_ide`, `css_ide`, `js_ide`, `md_ide`, `universal_ide` |
| Data / crypto | `encoding`, `compression`, `archive`, `crypto`, `bcrypt`, `uuid`, `regex`, `datetime`, `md`, `osrand`, `mime` |
| Media | `audio`, `image`, `font` |

---

## 3. Cross-Library Integration Gaps (the core ask)

This is where the ecosystem is weakest. Each row is a place where two libraries
*should* compose but currently don't (or do so brokenly).

### 3.1 Critical integration breaks

| # | Integration | Status | Evidence | Cost to fix |
|---|---|---|---|---|
| I1 | `http` server ↔ `tls` (HTTPS server) | **✅ Fixed** | `ServerConfig` has `cert_file`/`key_file`; `Server` loads cert/key in `start()`; `handle_conn` calls `tls_accept` when configured, sets `tls_ctx` on `ResponseWriter`; `read_request_incremental` uses `tls_ctx` for reads; `shutdown` frees TLS resources. | Done |
| I2 | `http` ↔ `json` (body parse/serialize) | **Partial** | ✅ `ResponseWriter.send_json(value)` added — serializes `JsonValue` and writes with `application/json` content type. Body parsing (`req.json<T>()`) still TODO. | Low — remaining glue |
| I3 | `http` ↔ `page` (SSR response) | **Missing** | `page` imports only `std`/`fs`; you must manually `resw.write_string(page.toString())`. | Low — glue function |
| I4 | `server` ↔ `http` (declared dep) | **✅ Fixed** | `server/chemical.mod` now declares `import http`. | Done |
| I5 | `image`/`audio` ↔ `http`/`webview` (in-memory encode) | **✅ Fixed** | `encode_png(img)`, `encode_bmp(img)`, `encode_ppm(img)`, `encode_wav(audio)` now return `Result<vector<u8>, Error>`. Tested with roundtrip tests. | Done |
| I6 | `webview` ↔ `http` (serve local app) | **Missing** | `webview` loads a URL string or raw HTML; no in-process `http` server tie / request interception. | Medium |
| I7 | `font` ↔ `image` (text rasterization) | **Missing** | `font` yields vector glyph outlines; no rasterizer to `Image`. No combined text-on-image. | High (rasterizer) |
| I8 | `compression` ↔ `archive` ↔ `http` | **Disjoint** | `archive` has its *own private deflate* and doesn't reuse `compression`; ZIP writer is Store-only; `compression` is RLE-only so it can't produce `Content-Encoding: gzip`. | Medium |

### 3.2 Duplication / divergence that hurts maintenance

| # | Issue | Impact |
|---|---|---|
| D1 | `js_parser` (41 `Js*` nodes) and `universal_parser` (49 `Js*` nodes + JSX) are **separate copies** of the JS AST; `universal_parser` does *not* import `js_parser`. Syntax fixes must be applied twice and the two drift. | Maintenance |
| D2 | `path` and `fs` both implement `basename`/`dirname`/`extension`/`join`/`normalize` on raw `*char` buffers — duplicated, and neither is a typed `Path`. | Maintenance / ergonomics |
| D3 | ~~`uuid` has its own `/dev/urandom`/`CryptGenRandom` RNG; `crypto` has **no** RNG at all.~~ | **✅ Fixed** | `osrand` library provides shared `random_fill`/`random_u32`/`random_u64`. `uuid`, `tls`, `bcrypt` all delegate to it. Platform-specific code removed from each. |
| D4 | `base64` lives in `crypto`, not `encoding` (which holds hex/url-encode). Placement oddity; `encoding` users must reach into `crypto`. | **✅ Fixed** | `encoding/chemical.mod` now `import crypto`, making `crypto::base64_*` accessible to encoding consumers. |
| D5 | HTML/CSS/MD *conversion* logic lives only inside the `_cbi` plugins; only JS shares a reusable converter (`js_parser::JsNodeEmitter`). | Consistency |
| D6 | `html`/`css`/`js` runtime libs round-trip to strings; they don't feed `page`'s SSR model (that's done by the macros). Two paths for the same data. | Consistency |

---

## 4. Real Missing Functionality (by area)

This section enumerates *features* that don't exist today. Each is tagged
**(neutral)** if it can be added as pure logic with negligible bundle/perf cost,
or **(heavy)** if it adds meaningful code size or a runtime subsystem. The
bundle-neutral items belong in the P0/P1 effort bands (§7); the heavy items in P2/P3.

### 4.1 Infrastructure & runtime primitives
- **Logger** *(neutral)* — there is *no* logging library anywhere in the tree.
  Only `test_env`'s log-style `info/warn/error` methods and `cstd` math `log`.
  A structured, leveled logger is the single highest-value, lowest-cost gap.
- **CLI argument parser** *(neutral)* — only `environment::get_env` exists; no
  `argv` parsing (`std::CommandLineStream` is not a parser). Flags, subcommands,
  and positional args are all hand-rolled by apps today.
- **Typed `Path`/`PathBuf` + `read_dir` + recursive ops** *(neutral)* — `path`/`fs`
  still work on raw `*char` buffers. No `exists`/`canonicalize`/`create_dir_all`/
  `remove_dir_all`/directory-walk iterator.
- **Event loop / async runtime** *(heavy)* — only a bare `Thread`/`ThreadPool`/
  `Mutex` in `std`; no channels, futures, async, or IO event loop (the IOCP path
  in `net` is network-only).
- **A shared `Reader`/`Writer`/`Stream` IO abstraction** *(neutral)* — see §5.1;
  without it every byte-moving library reinvents its own buffer type.
- **Configuration management** *(neutral)* — no typed config loader (env +
  file + defaults merge). Apps wire this by hand.

### 4.2 Web: HTTP server & client
- Server-side **HTTPS** *(neutral — wiring only, see I1)*.
- **Middleware** implemented in `Router` but **never invoked** by `Server`
  (`apply_middlewares` is dead code) — effectively no middleware/route groups.
- **Cookies & sessions** *(neutral)* — none; no `Set-Cookie`/`Cookie` parsing.
- **Request body parsing** *(neutral)* — only query strings (`parse_query`); no
  `application/x-www-form-urlencoded`, no `multipart/form-data`, no automatic
  JSON (de)serialization (see I2).
- **WebSockets / SSE / server push** *(heavy)* — none (`Connection: close` only).
- **HTTP compression** *(neutral)* — no gzip/deflate/brotli; client doesn't follow
  redirects or reuse keep-alive; `basic_auth` client is a **no-op stub**.
- **MIME detection as a shared utility** *(neutral)* — `get_mime_type` is private
  to `FileServer`; should be a standalone `mime` lib (§5.2).
- **URL building/encoding** *(neutral)* — only `url_decode` exists; no
  `url_encode`, no query-string builder, no relative-resolution helper.

### 4.3 Data formats & serialization
- **Generic struct ↔ JSON (de)serialization** *(neutral)* — `json` has
  `JsonEncoder`/`JsonDecoder` and `std` has `Encoder`/`Decoder` interfaces, but
  there is no derive/reflection path that auto-maps a Chemical `struct` to/from
  `JsonValue`. This is one of the most-requested web-stack features and is pure
  glue (see §5.6).
- **JSON schema validation** *(neutral)* — `json` parses but does not validate
  shape/types against a schema.
- **Config file formats** *(neutral)* — only `json` and `md` exist. No TOML,
  YAML, or INI parser, despite these being common for app/config files.
- **Binary serialization** *(neutral)* — no `msgpack`/`cbor`/`protobuf`; only
  ad-hoc byte packing in `archive`/`crypto`.

### 4.4 Crypto / encoding
- `crypto` *(heavy, except RNG which is neutral)*: no SHA-1/SHA-3, **no symmetric
  ciphers** (AES/ChaCha), **no asymmetric** (RSA/ECDSA/ed25519), **no key
  derivation** (PBKDF2/Argon2/scrypt — `bcrypt` is separate). ✅ Public RNG now
  exists via `osrand` library. No URL-safe base64.
- `encoding` *(neutral)*: no base64 (it's in `crypto`), no UTF-32, no
  query-string→map decode, no HTML-entity encode/decode.

### 4.5 Math, geometry & color
- **Vector / matrix math** *(neutral)* — no `vec2/3/4`, `mat4`, quaternion, or
  transform helpers, despite `window`/`webview`/`image` all needing geometry.
- **Color space library** *(neutral)* — `css_parser` has `CSSRGBColorData` etc.
  and `image` has `RGBA8`, but there is no shared `color` type or
  rgb↔hsl↔hsv conversion usable across them.
- **Big integer / decimal** *(heavy)* — no arbitrary-precision integer or
  fixed-point decimal (relevant for crypto, finance, uuid v1 timestamps).

### 4.6 `datetime` / `uuid` / `regex`
- `datetime` *(neutral)*: ✅ `parse(fmt, str)` and `to_iso8601()` added;
  ✅ `now()` convenience added. Still missing: epoch-second/milli accessors,
  IANA tz DB / DST rules (only `utc`/`local`/`fixed`).
- `uuid` *(neutral)*: only v4/v7 (no v1/v3/v5); no JSON formatter helper; no
  URN/brace/no-dash variants.
- `regex` *(neutral)*: no flags (case-insensitive/multiline/Unicode); **no
  global/all-match iterator**; `replace` backreferences limited (literal
  replacement, no `$1`).

### 4.7 Media
- `image` *(heavy for new codecs)*: PNG/BMP/PPM only (no JPEG/WebP/GIF/TIFF/AVIF);
  ✅ in-memory encoders (`encode_png`/`encode_bmp`/`encode_ppm`) added (I5 done);
  no resize/scale; no alpha compositing.
- `audio` *(heavy)*: WAV/PCM only (no MP3/OGG/FLAC/opus); **no playback** anywhere;
  processes as `i16` only (no float audio path). ✅ In-memory encode
  (`encode_wav`) added (I5 done).
- `font` *(heavy)*: TTF only (no OTF/WOFF/ttc); no text shaping (kerning/RTL/bidi);
  no rasterizer (D7).

### 4.8 Observability, testing & dev ergonomics
- **No benchmarking hooks** in `test`/`test_env` — `@test` functions run but there
  is no built-in timing/throughput assertion or perf-regression harness.
- **No fixture / setup-teardown framework** — tests are flat functions.
- **No metrics / tracing API** — nothing to record counters/spans for
  service-style apps.
- **No assertion/validation library** — `assertEquals` exists in the test
  framework, but there is no standalone `validate(value, schema)` for runtime
  input validation (forms, API payloads).

### 4.9 Internationalization & localization
- **No i18n framework** *(neutral)* — no message catalogs, plural rules, or
  `gettext`-style lookup.
- **No locale-aware formatting** *(neutral)* — `datetime::format` and any number
  formatting are locale-blind; no `NumberFormatter` with grouping/currency.

### 4.10 Application-level gaps (mostly heavy, listed for completeness)
These are real product-shaped gaps but each is a large subsystem; they are called
out so the roadmap is honest, not because they are cheap:
- No email (SMTP/MIME) library.
- No caching / rate-limiting library.
- No templating engine beyond the `#html`/`#md` macros (no logic-less string
  template, no `printf`-style formatter with named params beyond `expr_println`).
- No database/ORM layer (only `mongodb` exists, and it lives under
  `lang/compiled`, not shipped as a `lang/libs` library).
- No `mime`/media-type registry as a first-class library (§5.2).

---

## 5. Code Deduplication & Library Extraction

Beyond glue, a large share of the gaps above come from **logic that is duplicated
or locked inside a single library**. Extracting that logic into its own reusable
library is the highest-leverage way to (a) shrink maintenance burden, (b) give
other libraries a single source of truth, and (c) produce composable building
blocks. None of these add runtime cost — they are reorganizations plus thin
adapters.

### 5.1 Extract a shared `io` / `stream` library
- **Problem:** `fs` has `File`, `net` has `Buffer`/`Socket`, `http` has `Body`/
  `ResponseWriter`, `compression`/`archive` use raw byte buffers — five different
  byte-moving types that cannot feed each other. This is the root cause behind
  I2/I5/I8 and the "libraries don't work together" complaint.
- **Proposal:** a `stream` library exposing `Reader`/`Writer`/`Stream` traits
  (the `core` module already defines a `Stream` trait; `std` has serialization
  `Encoder`/`Decoder` interfaces — adopt and extend them) plus a `ByteBuffer`.
  Implement the trait for `fs::File`, `net::Socket`/`Buffer`, `http::Body`/
  `ResponseWriter`, and the compression/archive buffers. Everything else (JSON
  over HTTP, gzip over HTTP, zip-from-any-source) then composes for free.

### 5.2 Extract a `mime` library — ✅ DONE
- **Problem:** `get_mime_type` is private to `FileServer` (§4.2). `http`,
  `server`, `webview`, `fs`, and the media libs all need content-type detection
  but can't reach it.
- **Solution:** Standalone `mime` library (`lang/libs/mime/`) with `get_type(ext)`,
  `extension(path)`, `is_text()`, `is_image()`. `http::FileServer.get_mime_type`
  now delegates to `mime::get_type`. 16 tests cover all lookup paths.

### 5.3 Extract a `rand` library — ✅ DONE
- **Problem:** `uuid` rolls its own `/dev/urandom`/`CryptGenRandom`; `tls` has
  `random_fill`; `bcrypt` generates its own salt; `crypto` has **no** RNG (D3).
  Four random sources, no shared one, and `crypto` consumers can't get secure
  bytes.
- **Solution:** The `osrand` library (`lang/libs/osrand/`) provides shared
  `random_fill(buf, len)`, `random_u32()`, and `random_u64()`. Platform-specific
  code (BCryptGenRandom on Windows, `/dev/urandom` on POSIX) is consolidated here.
  `uuid`, `tls`, and `bcrypt` all delegate to `osrand::random_fill`. The
  duplicated platform-specific code was removed from each library.

### 5.4 Make `universal_parser` reuse `js_parser`'s JS AST (D1)
- **Problem:** `universal_parser` defines its *own* copy of the entire JS node
  family (49 `Js*` structs + its own `JsLexer`/`JsTokenType`), duplicating
  `js_parser` (41 nodes). JSX is the only real addition.
- **Proposal:** `universal_parser` imports `js_parser` and extends it with the
  JSX-specific nodes only. Syntax/lexer fixes apply once. This is the single
  biggest parser-maintenance win.

### 5.5 Share HTML/CSS/MD converter emitters (D5)
- **Problem:** only JS exposes a reusable converter (`js_parser::JsNodeEmitter`)
  consumed by both a CBI and a runtime lib. HTML/CSS/MD emit logic is locked
  inside the `_cbi` plugins, so the runtime `html`/`css`/`md` libs can't reuse
  macro→text conversion.
- **Proposal:** lift each converter into the corresponding `*_parser` (or a small
  shared `convert` lib) so any consumer — CBI plugin *or* runtime library — can
  serialize the AST. Removes the asymmetry in D6.

### 5.6 Generic struct ↔ JSON (de)serialization (§4.3)
- **Problem:** `json` and `std` both have encoder/decoder interfaces, but mapping
  a Chemical `struct` to/from `JsonValue` is hand-written per type. Every web app
  reinvents this.
- **Proposal:** a `json_serde` adapter (built on the existing `Encoder`/`Decoder`
  interfaces + compiler reflection) that auto (de)serializes structs. Pure glue;
  the heaviest part is the reflection binding, which already exists for the
  compiler API. This single addition makes `http`+`json` ergonomic.

### 5.7 Consolidate `path` / `fs` into a typed `Path` (D2)
- **Problem:** `path` and `fs` both expose `basename`/`dirname`/`extension`/`join`/
  `normalize` on raw `*char` buffers, duplicated and untyped.
- **Proposal:** make `path` the single home of a `Path`/`PathBuf` type with
  methods; route `fs`'s path helpers through it. Callers get a real typed API and
  the duplication disappears.

### 5.8 Move `base64` into `encoding` (D4) — ✅ DONE (partial)
- **Problem:** `base64` lives in `crypto` while hex/url-encode live in `encoding`.
  `encoding` users must reach into `crypto`.
- **Solution:** `encoding/chemical.mod` now declares `import crypto`, making
  `crypto::base64_decode`/`crypto::base64_encode` accessible to consumers of the
  `encoding` module. Full relocation deferred; this provides discoverability
  without breaking existing `crypto` import paths.

### 5.9 `archive` reuses `compression`'s deflate (I8)
- **Problem:** `archive` carries its *own private deflate* implementation and the
  ZIP writer is Store-only; `compression` is RLE-only and can't produce gzip.
- **Proposal:** expose a reusable deflate/inflate from `compression` (it already
  exists privately inside `archive`); have `archive` call it and add
  `zip_writer_add_deflate`. Enables gzip-for-HTTP and compressed zip writes from
  shared code.

### 5.10 Shared tokenizer utilities for the five parsers
- **Problem:** each of `html_parser`/`css_parser`/`js_parser`/`md_parser`/
  `universal_parser` re-implements string/number/regex/identifier reading on top
  of the `compiler::Lexer` interface.
- **Proposal:** a small `lexer_utils` library (built on `compiler::Lexer`) with
  shared readers (string escapes, numbers, identifiers, whitespace) used by all
  five. Reduces per-parser boilerplate and drift.

### 5.11 Consolidate color handling
- **Problem:** `css_parser` has `CSSRGBColorData`/`CSSHSL...` etc.; `image` has
  `RGBA8`; `components` emit color literals — three notions of color.
- **Proposal:** a shared `color` type (rgb/hsl/hsv + conversions) in `std` or a
  tiny `color` lib, used by `css_parser`, `image`, and `components`. Low priority
  but removes a recurring mismatch.

---

## 6. What We Can Provide *Without* Impacting Bundle Size or Performance

The constraint is the key insight: **glue, interfaces, and convenience adapters
add little or no generated code and zero runtime overhead** when they don't pull
in heavy dependencies. The highest-leverage, bundle-neutral work (most of it drawn
from §3 integration breaks and §5 extraction proposals):

### 6.1 Shared byte-stream interface (highest leverage)
Adopt `Reader`/`Writer`/`Stream` (§5.1) implemented by `fs`, `net`, `http`,
`compression`, `archive`. A trait + a few impls is bundle-neutral and unlocks
I2/I5/I8.

### 6.2 Integration glue functions (pure wrappers, near-zero cost)
- ✅ `http`: `resw.send_json(value)` using `json` (I2 done). `req.json<T>()` still TODO.
- `http`: `resw.send_page(HtmlPage)` (I3). ← TODO
- ✅ `http`: wire `tls_accept` into `Server` for HTTPS (I1 done).
- `image`/`audio`: ✅ `encode_png(image) -> vector<u8>`, `encode_bmp`, `encode_ppm`,
  `encode_wav(audio) -> vector<u8>` all implemented (I5 done).
- `webview`: a `serve_from(http_server)` / local in-process static server helper (I6). ← TODO
- `server/chemical.mod`: ✅ declare `http` (I4 done).

### 6.3 Ergonomic adapters / conversion methods (no new logic)
- `uuid.to_json()` / `uuid.from_json()` ← TODO
- ✅ `datetime.to_iso8601()` and `datetime.parse(fmt, str)` implemented.
  `datetime.now()` convenience also added.
- ✅ `mime` detection as a shared lib (§5.2) — DONE.
- Unified `Path` type (§5.7) replacing duplicated `*char` free functions (D2). ← TODO

### 6.4 Shared infrastructure primitives (bundle-neutral, high value)
- **`Logger`** library (levels, output sink, no allocation pressure) — the #1 gap. ← TODO
- **`args` parser** (over `environment` + `std` argv) — pure logic. ← TODO
- **Single shared RNG** (`rand` lib, §5.3) — ✅ DONE. `osrand` library provides
  `random_fill`/`random_u32`/`random_u64`, consumed by `uuid`/`tls`/`bcrypt`.
- Fix `Router.apply_middlewares` to actually run (I4/web) — dead code made live. ← TODO

---

## 7. Recommended Effort Allocation

Ranked by **impact ÷ cost**, with the bundle/perf constraint in mind:

| Priority | Work | Why | Cost |
|---|---|---|---|
| **P0** | Shared `Reader`/`Writer`/`Stream` interface (`stream` lib) across fs/net/http/compression/archive | Unlocks I2/I5/I8 and "libraries work together" | Low (traits + impls) |
| ~~P0~~ | ~~`http`↔`json` send_json~~ | ✅ Done — `ResponseWriter.send_json` | — |
| ~~P0~~ | ~~HTTPS-server accept wiring~~ | ✅ Done — `tls_accept` in `handle_conn` | — |
| **P0** | `http`↔`page`, middleware wiring | ✅ `apply_middlewares` wired; `send_page` TODO | Low |
| **P0** | `Logger`, `args` parser, typed `Path`/`read_dir` | Highest-value missing primitives, zero runtime cost | Low–Med |
| ~~P0~~ | ~~`mime` lib~~ | ✅ Done — `lang/libs/mime/` extracted from `FileServer` | — |
| ~~P0~~ | ~~shared `rand` lib~~ | ✅ Done — `osrand` library | — |
| ~~P1~~ | ~~In-memory `encode_*` for `image`/`audio`~~ | ✅ Done — `encode_png`/`bmp`/`ppm`/`wav` | — |
| ~~P1~~ | ~~`datetime.parse`/ISO-8601~~ | ✅ Done — `parse()`, `to_iso8601()`, `now()` | — |
| ~~P2~~ | ~~`http`: middleware live~~ | ✅ Done — `Router.apply_middlewares` wired in `Server` | — |
| **P1** | `webview`+`http` local server | Unblocks serving/embedding media | Low–Med |
| **P1** | `regex` global-match + flags, `uuid` JSON helpers, generic struct↔JSON | Ergonomics, pure logic | Low–Med |
| **P1** | Extract & de-dup: `js_parser`/`universal_parser`, shared converters, `archive` reuses `compression` deflate, `lexer_utils`, `color` | Maintenance leverage + composability | Med |
| **P2** | `crypto`: AES/ChaCha, RSA/ed25519, PBKDF2, public RNG, URL-safe base64 | Real security value, but adds code size | Med–High |
| **P2** | `http`: cookies/sessions, middleware live, websockets, form/multipart parsing, gzip | Completes the web framework | Med–High |
| **P3** | Media formats (JPEG/WebP/GIF, MP3/OGG, OTF/WOFF), audio playback, `font` rasterizer, async runtime | Heavy, large bundle cost, niche | High |

**Recommendation:** spend the next cycle on **P0 + P1**. These are almost entirely
glue, interfaces, and convenience — they make the existing 70 libraries compose
without growing binaries or slowing anything down. The deduplication/extraction
work in §5 is the multiplier: each extracted library (a shared `stream`, `mime`,
`rand`, reusable JS AST, shared converters) is itself a reusable building block
that *other* libraries can compose with, so the ecosystem compounds. Defer the
heavy P2/P3 feature additions (new codecs, ciphers, async runtime, playback) until
the integration layer is solid, since those *do* cost bundle size and are easier
to land on top of a clean shared-IO foundation.

---

## 8. Concrete Quick-Win Task List

1. Add `core`/`std` `Reader` + `Writer` traits; implement for `fs::File`,
   `net::Buffer`, `http::Body`, `http::ResponseWriter` (§5.1). ← TODO
2. ✅ `http`: `ResponseWriter.send_json(v)` via `json` (I2 done). `Body.json::<T>()` still TODO.
3. `http`: `ResponseWriter.send_page(p: HtmlPage)` via `page` (I3). ← TODO
4. ✅ `http`: call `tls_accept` in `Server.handle_conn` when a cert is configured;
   set `ResponseWriter.tls_ctx` (I1 done).
5. ✅ `server/chemical.mod`: add `import http` (I4 done).
6. ✅ `image`/`audio`: add `encode_*() -> vector<u8>` alongside `save_*()` (I5 done).
7. ✅ Extract a `rand` lib; have `uuid`/`tls`/`bcrypt`/`crypto` use it (§5.3, D3 done).
8. ✅ `encoding`: `import crypto` for base64 discoverability (§5.8, D4 done).
9. ✅ `datetime`: add `parse(fmt, str)` and `to_iso8601()` and `now()` (done).
   `uuid`: add `to_json`/`from_json`. ← TODO
10. `universal_parser`: import `js_parser` JS AST instead of copying it (§5.4, D1). ← TODO
11. `path`: introduce a `Path`/`PathBuf` type; route `fs`/`path` helpers through it (§5.7, D2). ← TODO
12. Add a `log` library (levels + sink) and a `cli`/`args` parser library. ← TODO
13. ✅ `http`: make `Router.apply_middlewares` actually run in `Server.handle_conn` (done).
14. `archive`: call `compression`'s deflate so `zip_writer_add_deflate` exists (I8, §5.9). ← TODO
15. ✅ Extract a `mime` library from `FileServer.get_mime_type` (§5.2) — DONE.
16. Add a `json_serde` adapter for generic struct ↔ `JsonValue` mapping (§5.6). ← TODO
17. Extract `lexer_utils` shared by the five parsers (§5.10). ← TODO
18. Share HTML/CSS/MD converter emitters the way JS already does (§5.5, D5/D6). ← TODO

---

*Initially prepared from a full survey of `lang/libs/*` public APIs and dependency graphs.
Updated after completing two batches of P0/P1 items: shared `osrand` RNG,
in-memory image/audio encoders, `datetime` parse/ISO-8601/now, HTTPS server config
wiring, `server` dependency fix, `encoding` base64 discoverability, middleware wiring,
`mime` library extraction, HTTPS accept handshake, `send_json` glue, and 50+ new tests.*
