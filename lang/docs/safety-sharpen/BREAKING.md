# Safe-API / language breaking changes

This release is allowed to break. The migration mapping is in
[`REMOVED.md`](REMOVED.md) and will be expanded per Phase 2 library job.

## Printing / formatting wave (2026-11-14)

- **`print`/`println` now work, and they render through `string`.** They used to
  compile to nothing at all: `@expand` templates that `return`ed an
  `intrinsics::expr_str_block_value(...)` block held an `any` return type, and
  `any` maps to `void` in MIR, so the whole generated block was discarded as a
  "void return". The compiler now evaluates `return <expr>` in a void-returning
  comptime template for its side effects (the general fix), and the console
  functions are plain statement-shaped templates again. `std::print` no longer
  returns `any` — nothing is returned.
- **`eprint`/`eprintln` added.** There was no way to write to standard error
  from safe code (only `unsafe { fprintf(stderr, ...) }`). Both spell a line
  into a `std::string` and write it once.
- **Console writes report failure.** `write_stdout`/`write_stderr` return
  `Result<std::Unit, std::io::IoError>` (a short `fwrite` is no longer silently
  dropped), `std::io::error_from_errno` maps the platform code onto
  `IoErrorKind`, and `try_print`/`try_println`/`try_eprint`/`try_eprintln` are
  the checked forms. `print`/`println` stay best-effort (they discard the
  result).
- **A console line is one write, flushed immediately, in program order.**
  Rendering into a `std::string` first means a line cannot be split by another
  thread, and output is visible while the process runs instead of only at exit.
  On Windows `get_stdout`/`get_stderr` open a *second* CRT stream over the
  process handle, so the CRT is flushed around each line; otherwise `printf`
  and `println` output reached the handle in the wrong order.
- **A raw `*char` cannot be formatted.** `${cstr}` used to call `strlen` on a
  pointer of unknown length, *inside an `UnsafeBlock` the compiler generated
  itself* — a safe formatting call could read past the end of the buffer (a
  one-byte `str` subview rendered the whole underlying string). `core::stream::
  Stream::writeStrNoLen` is removed and the intrinsic rejects a bare `*char`.
  Migration: bind what you are printing to something that carries its length:
  `string_view`, `str` or `string` (`var s = "x"` becomes `var s =
  std::string_view("x")`). A string *literal* is still fine (`${"x"}`), because
  its length is known at compile time.
- **Format specs: `${expr:spec}`.** `${n:x}`, `${n:X}`, `${n:#b}`, `${n:o}`,
  `${n:08}`, `${n:+.5}`, `${s:<8}`, `${s:^8}`, `${s:*>8}`, `${d:.2f}`,
  `${d:10.2f}`, `${d:.3e}`, `${d:g}`, `${d:08.2f}` and string precision
  (`${s:.3}`, truncating) are supported. The spec is validated at compile time
  (invalid flags/types and width/precision > 254 are errors), packed into one
  `u64` constant, and decoded by `std::decode_format_spec`; `spec == 0` is the
  unchanged plain path. The `stream` hook receives the spec:
  `func stream(&self, s : &mut std::StringStream, spec : u64)` — a two-parameter
  hook still compiles but cannot be given a spec.
- **`bool` prints; unsupported types say so.** `std::println(`...${flag}`)`
  used to fail at codegen with `unknown value being used in expressive string`.
  `bool` renders `true`/`false` via `Stream::writeBool`, references format as
  their referent, and any other unsupported type is a compile error naming the
  supported kinds and the `stream` hook.
- **Doubles and floats render faithfully.** The old conversion produced
  `0.333` for `1.0/3.0` (three digits, and a different constant in each sink:
  3 in `string`, 6 on the console), `9223372036854775.807` for `1e30`, `0.000`
  for `1e-20`, and `0.000` for `-0.0`. Now: the shortest form that reproduces
  the value exactly (at most 15 significant digits for a double, 9 for a
  float, so `3.14f` prints `3.14`), trailing zeros trimmed, scientific
  notation only outside `[1e-4, 1e15)`, `-0.0` keeps its sign, and `nan`/`inf`
  spell themselves. `append_double(value, precision)` still prints exactly
  `precision` fraction digits; `precision < 0` selects the shortest form.
  **Output changes**: `pi value : 3.140` becomes `pi value : 3.14`.
- **One formatter.** `CommandLineStream` no longer carries a second, divergent
  double/float conversion; it delegates to `std::string` for both the plain and
  the formatted path.

## Server / ownership wave (2026-11-13)

- **`http::Transport` is gone; a connection is `http::Stream`.** A `variant` over
  `Tcp`/`TlsClient`/`TlsServer` that owns the stream by value. The old shape was
  a tagged struct of three nullable pointers behind `*mut Transport`, aliased by
  the request body *and* the response writer. There is now one owner and one
  destructor; `transport_box_plain`/`transport_box_tls`/`transport_box_tls_server`
  are removed. `Stream` implements `std::io::Read`/`Write`.
- **`http::ResponseWriter` no longer appears in the handler contract.** A handler
  receives the request and returns a response; the framework owns the connection
  and encodes it. `Handler` is
  `std.send_sync_function<(req : http.Request) => http.Response>` — the request is
  moved into the handler, so the callback carries no lifetime and no raw pointer.
  Now enforced in one place instead of being re-derived per driver:
  `Content-Length` is always exact, a second write cannot desynchronise the
  framing, the status line carries the real reason phrase (`404 Not Found`, not
  `404 OK`), and keep-alive/pipelining is decided per message instead of always
  emitting `Connection: close`.
- **`http::Body` was split by direction.** `http::RequestBody` is a buffered
  reader the handler consumes (`size`, `remaining`, `view`, `text`, `read`,
  `read_to_end`, `read_to_string`); `http::ResponseBody` is a payload the
  framework writes (`Empty`/`Bytes`/`Text`/`File`), with `File` streaming from
  disk without copying. Request bodies are framed and buffered *before* dispatch,
  bounded by `ServerConfig.max_body_bytes`; a streaming variant is additive
  later. This is what makes `Handler` lifetime-free.
- **`server::Server` is bound explicitly and consumed by serving.**
  `Server.bind(cfg) -> Result<Server, NetError>` reports a bind failure at the
  call site (it used to be swallowed inside `start`), `local_addr()` returns the
  address actually bound (so tests can use port 0), `serve(self)`/`serve_async(self)`
  consume the server, and `handle()` returns a `ShutdownHandle` that stays valid
  after the server is consumed. `serve(port)`/`serve_async(port)`/`shutdown()`
  are removed: the port lives in `cfg.addr`, and stopping goes through the
  handle. The stop flag is private.
- **`ServerConfig` gained `body_timeout_secs`** (default 30). A body may be large
  and slow while a head must arrive quickly, so the body deadline is no longer
  derived from `header_timeout_secs`. A peer that stalls longer is rejected with
  `400`.
- **A by-value `self` parameter is a move.** `func consume(self)` transfers
  ownership to the callee, which destroys it exactly once; the caller no longer
  drops its own copy. Before the fix every consuming method double-freed its
  receiver. Moving a by-value `self` out of a pointer/reference is rejected
  (`SAFE-MOVE-004`). This unblocks `Box::into_inner`/`Rc::try_unwrap`/
  `Arc::try_unwrap`-shaped APIs and a consuming `Server::serve(self)`.
- **`http::Client` owns its trust store.** `set_ca_chain` now takes the chain by
  value (`client.set_ca_chain(chain)`), and the system CA bundle is loaded once
  per client instead of per request. Previously the config kept a raw pointer to
  a chain that had already been freed before the handshake (use-after-free on
  every HTTPS request that used the system bundle).
- **`http::Client` request methods take `&self`** (`get`, `post`, `put`,
  `patch`, `delete`, `head`, and their `_view` forms). The trust store lives
  behind a `Mutex`, so one client can be shared by a worker pool without
  `Arc<Mutex<Client>>`. Responses are buffered, so `res.body.text()` /
  `res.body.size()` replace the streaming reader. `var client = Client()` call
  sites are unchanged.
- **`http::HttpResult` and the whole `http::raw` async surface are deleted**
  (`get_async`/`post_async`/`put_async`/`patch_async`/`delete_async`/`head_async`/
  `request_async`, `serve_coro`, `make_body`, `body_read`, `read_chunk_async`,
  `body_read_async`, `read_to_string_async`, `header_get_cstr`). They returned
  caller-owned `*mut HttpResult` boxes and were `spawn_blocking` behind an
  `async` keyword. Real async client/server I/O arrives with
  `AsyncRead`/`AsyncWrite`. `http` also no longer depends on the `json` library
  (`response_json_text` takes pre-serialized text).
- **`HeaderMap` is a real table**: `insert` replaces instead of duplicating,
  `append` is the explicit multi-value form, and `len`/`contains`/`remove`/
  `clear`/`iter()` exist (a client can finally enumerate response headers).
  `QueryMap::get` returns `Option<string_view>`, so `?a=` is distinguishable from
  an absent parameter.
- **`std::atomic` operations take `&self`** (`load`, `store`, `fetch_*`,
  `compare_exchange_*`). An atomic that required `&mut self` could not be used
  for shared state at all, which is the only reason to have one.
- **`net`'s owning socket API is public**: `TcpListener.bind`/`accept` and
  `TcpStream.connect`/`read_bytes`/`write_bytes` were package-private, which is
  why `http` had to hand-roll a transport over `net::raw` descriptors.
  `net::Buffer` gained `append(&[u8])`/`view()`/`take()`/`reserve()` and a bulk
  copy (it appended byte by byte). `SocketAddr` renders octets in the right
  order.
- **A route handler is a stored `Send + Sync + 'static` callback.** `web::Handler`
  is now `std.send_sync_function<(req : &mut http.Request, res : &mut
  http.ResponseWriter) => void>`. Registering a handler whose capture is a
  borrow, an `Rc`, or other non-`Send`/non-`Sync` state is rejected with
  `SAFE-SEND-002`, because the router keeps the handler for the server's
  lifetime and may invoke it concurrently from the worker pool.
  Migration: share state through `std::arc(...)` (and `.clone()` per handler),
  or capture owned values.
- **`web::Middleware`, `Router.use_middleware` and `Router.apply_middlewares`
  are removed.** A middleware returned a new handler capturing the previous one,
  which cannot preserve an owned `Send + Sync` witness. Handler composition
  (middleware) needs an owned-composition design and is a follow-up; register
  the composed behaviour as one handler for now.
- **New `std::send_sync_function<T>`**, the stored-callback witness
  (`%capture<T, default_function_instance, SendSyncStaticCapture>`). It proves
  `Send + Sync + 'static` at creation, so it satisfies `T : Send` and
  `T : Sync` bounds; `std::send_function<T>` stays `Send`-but-not-`Sync`
  (it may contain interior mutability that is safe to move but not to share).
  Auto-trait derivation now follows callback type aliases, so both types report
  their real bounds (before, an aliased callback was judged by the erased
  instance struct with its raw pointers and satisfied neither bound).

## Safe by default (2026-11-12)

Every public library name is now the **safe** form. There is no parallel
`*_safe` / `safe_*` / `::safe::` spelling of the same operation; raw-pointer and
FFI forms live under an explicit `raw`/`ffi` namespace (or, where a platform
file already owns that namespace, a `*_native` free function whose signature
uses the `Native*` type).

| Old | New |
|---|---|
| `image::parse_png(*u8, size_t)` | `image::raw::parse_png(...)` |
| `image::image_width(*mut Image)` | `image::raw::image_width(...)` (prefer `img.width()`) |
| `image::load_png(*char)` | `image::raw::load_png(...)` (prefer `Image.load(&Path)`) |
| internal `image_safe_*` helpers | `image` private `decode_*` / `encode_*_bytes` |
| `audio::audio_volume(*mut Audio, f)` | `audio::raw::audio_volume(...)` (prefer `audio.volume(f)`) |
| `font::font_load(*char)`, `font::font_ascent(*mut Font)` | `font::raw::*` (prefer `Font.from_path`, `font.metrics(size)`) |
| `page::safe_page_escape_js` | `page::escape_js` |
| `page::safe_page_escape_json` | `page::escape_json` |
| `page::safe_page_append_html` | `page.append_html` |
| `page::safe_page_append_js` | `page.append_js` |
| `page::safe_page_append_html_escaped` | `page::append_html_escaped` |
| `page::safe_page_append_js_escaped` | `page::append_js_escaped` |
| `page::safe_page_set_parameter` | `page::set_parameter` (or `page.add_parameter<T>`) |
| `page::safe_page_get_parameter_safe` | `page::get_parameter_owned` |
| `page::SafePageParam` | `page::PageParam` |
| `window::safe::Window` | `window::Window` |
| `window::Window` (platform state) | `window::NativeWindow` |
| `webview::safe::WebView` | `webview::WebView` |
| `webview::WebView` (platform state) | `webview::NativeWebView` |
| `webview::create/open_url/open_html` | `webview::create_native/open_url_native/open_html_native` |
| `router::safe::*` | `router::*` (`RequestInfo`, `set_request`, `get_request`, `query`) |
| `server::safe::*` | `server::*` (`serve_static`, `serve_static_async`) |
| `components::safe::*` | `components::*` (`theme_scope`, `escape_text`, `escape_attr`) |
| `test_env::safe::TestEnvHandle` | `test_env::TestEnvHandle` (`handle(env)`, `env.handle()`) |
| `test_env::safe_handle()` | `env.handle()` |
| `test_report::safe::*` / `SafeTestOutcome` | `test_report::*` / `test_report::TestOutcomeSnapshot` |
| `lab::safe::*`, `lab::safe_new_module` | `lab::*` (`run`, `new_module`, `ModuleId`, `LabError`) |
| `osrand::random_u32_safe` / `random_u64_safe` | `osrand::random_u32` / `random_u64` |

Rule going forward: a new public safe API takes the plain module-level or
method name; the raw counterpart goes into `<lib>::raw`/`<lib>::ffi` and is
documented as unchecked. Do not add a `safe` qualifier to a library name again.

## Phase 1 (language surface)

- Associated types and associated-type equality constraints are removed.
- External/generic inherent `impl Type { ... }` blocks are removed.
- Nested tuple/array destructuring and array/slice patterns in conditions are removed.
- Slice-view patterns are removed; use `size()`/`get(i)`.
- `Box<[T]>` and `std::boxed_slice<T>` are removed; use `vector<T>` or `[T]` views.
- Range-slicing syntax is removed; use `subview`/`split_at`.
- `dyn`/`DynBox`, tuple structs, unit tuple, core tuples, flat destructuring,
  slices/`str`, and generic impls remain.

## Release artifact

The release `TCCCompiler` is now built with size-oriented flags (`-O2`,
function/data sections, gc-sections, stripped) and is under the 5 MB hard
ceiling. See `metrics.json`.

## Phase 2 (safe APIs)

- `std::io::IoError`, `IoErrorKind`, `Read`, `Write`, and `Seek` are the shared
  I/O boundary. Library-specific errors convert with `fs::io_error`,
  `process::io_error`, `environment::io_error`, `net::io_error`, and
  `tls::io_error` where available.
- `fs::File` owns its descriptor again (`@delete`); `fs::close_file` is
  idempotent. Safe path-based helpers (`open_file`, `create_file`, `read_file`,
  `write_file`, `*_safe` directory/metadata helpers) take `&str`/`[u8]` and
  return `Result<_, IoError>`. Legacy raw-buffer functions remain as reviewed
  migration debt for in-tree callers; the ones that retain caller pointers are
  `unsafe`.
- `process::spawn` returns an owning `Result<Child, ProcessError>`.
  `wait`/`try_wait`/`kill_process`/`is_running`/`write_stdin`/`close_stdin`/
  `child_pid` take `&mut Child`; `Child` owns the platform handles.
- `environment::set`, `unset`, `all`, `set_async`, and `unset_async` are
  `unsafe` because they mutate process-global state.
- `path::PathBuf` (`path::Path` alias) is the safe path value; `basename`,
  `dirname`, `extension`, `stem`, `join`, `normalize`, `is_absolute`, and
  `has_root` are methods. The legacy raw-buffer path functions are `unsafe`.
- `net::TcpListener`/`net::TcpStream` are owning safe handles with
  `bind_tcp`/`connect_tcp`, `Read`/`Write`, and safe async
  `read_async`/`write_async`. `TcpStream::release_raw` is the explicit unsafe
  ownership-transfer accessor used by TLS.
- `http::Body::read_into`/`read_all`, `HeaderMap::get_str`,
  `RequestBuilder::header_str`, and `RequestBuilder::set_body_str` are the safe
  HTTP body/header surface. `Body` ownership fields are internal; legacy raw
  body/header calls remain. `Client::set_ca_chain` is unsafe because the chain
  must outlive every request.
- `tls::TlsStream` (verified client), `tls::TlsServerStream`, `CertificateChain`,
  `PrivateKey`, `TlsServerConfig`, `load_certificate`,
  `load_rsa_private_key`, `load_ec_private_key`, `rsa_private_key_from_components`,
  `connect`, `connect_insecure`, and `accept` are the safe TLS surface.
  `TlsServerConfig<'a>` borrows the certificate chain and private key for the
  whole `accept` call. Raw mbedTLS-shaped functions remain for the HTTP/TLS
  internals and tests.
- `std::CString`/`CStr`, `vector::try_reserve`/`try_push`/`get_opt`/`get_mut_opt`/
  `try_set`/`try_pop`, `span::get_opt`, `mut_span::get_mut_opt`, and
  `string::get_opt` are the additive safe primitive surface. Legacy raw pointer
  accessors remain allowlisted.

## API-freeze wave (2026-11-09)

- Data/media libraries now expose canonical safe module-level APIs; legacy
  pointer/out-buffer/C-string functions moved under `<lib>::raw`.
- `fs`, `path`, `process`, `environment`, `net`, `http`, and `tls` now expose
  owning values and `Result` errors; raw descriptor/mbedTLS/platform entry
  points moved under `::raw`.
- Safe path APIs take `&Path`; `fs::read/write/read_dir`, `Command`/`Stdio`,
  `TcpListener`/`TcpStream`, `HttpError`/`Transport`, and
  `TlsConfig`/`Certificate`/`TlsStream` are the supported surfaces.
- Web/UI safe handles are canonical; page append/parameter methods and window
  methods are safe, while CBI/platform pointer hooks use explicit raw methods.
- Compiler/lab/test/async safe facades are canonical; raw ABI hooks are under
  `compiler::ffi`, `lab::raw`, `test_env::ffi`, `test_report::ffi`, and
  `async::raw`.
- `span`/`mut_span`/`str` lifetime normalization is **not** part of this freeze:
  the compiler does not yet enforce lifetimes carried by user-defined generic
  structs, so adding the parameters alone would not make the view sound.
