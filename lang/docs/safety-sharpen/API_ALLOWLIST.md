# Phase 2 API allowlist

Tier C is the raw C binding surface (`cstd`, compiler CBI, platform APIs). Tier A
libraries expose safe wrappers; the entries below are the reviewed remainder.

**Safe by default (2026-11-12):** the plain public name of a library API is the
safe one. Raw-pointer/FFI entry points live in `<lib>::raw` / `<lib>::ffi` (or a
`*_native` free function whose signature uses the `Native*` platform type). A
name must never contain `safe`: there is no `foo_load` + `foo_safe_load`, and no
parallel `::safe::` namespace.

`scripts/safety-sharpen-api-audit.sh` reports `RAW_NONRAW`, the count of public
raw-pointer functions declared **outside** `raw`/`ffi`/`sys`, and `SAFE_NAMED`,
the count of public names still carrying a `safe` qualifier. `RAW_NONRAW` is
zero for 21 of 29 libraries; `SAFE_NAMED` is zero everywhere.

Reviewed explicit raw accessors (allowed):

- `std::span::data`, `std::mut_span::data` -- view construction/inspection.
- `std::CString::as_ptr` -- audited C interop.
- `Box`/`Rc`/`Arc`/`UnsafeCell`/`MaybeUninit`/`ManuallyDrop`/`NonNull`
  `as_ptr`/`as_mut_ptr` and the `*_unchecked` constructors -- inherently-unsafe
  primitives that must stay public under an `unsafe func` for the safe wrappers
  to be built.
- `std` platform bindings (`std::posix`/`std::win` pthread/Win32 declarations)
  and the hashing entry points that take `*char` (C-compatible call surface).
- `net::Buffer::append_bytes` (raw write into an owned buffer) and the
  `async_recv`/`async_send` platform context hand-off.
- `http` router internals (`match_pattern(..., *mut vector)`) and
  `create_file_server(*char)`.
- `tls::CertificateChain::raw_handle` / `from_raw_borrowed` -- explicit unsafe
  ownership hand-off; `TlsStream`/`TlsServerStream`/`CertificateChain` are the
  supported surface.
- `Window::native_handle` -- the only raw platform accessor on the safe handle.
- `webview::create_native` / `open_url_native` / `open_html_native`, plus
  `WebView::dispatch`/`native_handle` (`webview::WebView` is the safe handle,
  `webview::NativeWebView` the platform state).
- `page::attach_shared`, `HtmlPage::add_parameter_object_raw`,
  `HtmlPage::get_parameter_object_ptr` -- CBI hand-off used by `page::raw`.
- `server::main(argc, argv : **char)` -- the conventional C entry point.
- `lab::raw`, `compiler::ffi`, `compiler::api` arena allocation, `atomic::raw`,
  `core::async` raw protocol, parser/CBI internals (`*_parser`, `*_cbi`,
  `*_ide`).

Run `scripts/safety-sharpen-api-audit.sh` for the current count by library.

## Reviewed inventory (2026-11-12)

| Library | raw-pointer public funcs | of which outside `raw` | public unsafe funcs/decls |
|---|---:|---:|---:|
| std | 70 | 70 | 23 |
| core | 0 | 0 | 0 |
| fs | 66 | 0 | 0 |
| path | 8 | 0 | 8 |
| process | 0 | 0 | 0 |
| environment | 0 | 0 | 6 |
| net | 18 | 11 | 3 |
| http | 6 | 3 | 4 |
| tls | 143 | 6 | 7 |
| encoding | 13 | 0 | 0 |
| crypto | 22 | 0 | 0 |
| compression | 4 | 0 | 0 |
| archive | 41 | 0 | 0 |
| image | 33 | 0 | 0 |
| audio | 11 | 0 | 0 |
| font | 10 | 0 | 0 |
| regex | 2 | 0 | 2 |
| json | 6 | 0 | 0 |
| mime | 4 | 0 | 0 |
| datetime | 0 | 0 | 0 |
| uuid | 0 | 0 | 0 |
| osrand | 1 | 0 | 1 |
| cbor | 3 | 0 | 0 |
| window | 79 | 1 | 1 |
| webview | 52 | 5 | 2 |
| page | 13 | 5 | 1 |
| components | 0 | 0 | 0 |
| router | 2 | 0 | 0 |
| server | 1 | 1 | 0 |

`process`, `datetime`, `uuid`, `components`, and `core` have zero ordinary
public raw-pointer functions in the audited surface. `image`, `audio`, and
`font` expose all raw codec/accessor entry points under `<lib>::raw` only.
