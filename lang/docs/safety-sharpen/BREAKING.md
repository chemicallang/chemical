# Safe-API / language breaking changes

This release is allowed to break. The migration mapping is in
[`REMOVED.md`](REMOVED.md) and will be expanded per Phase 2 library job.

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
