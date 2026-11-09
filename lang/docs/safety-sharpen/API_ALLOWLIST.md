# Phase 2 API allowlist

Tier C is the raw C binding surface (`cstd`, compiler CBI, platform APIs). Tier A
libraries expose safe wrappers; the remaining entries below are legacy
raw-buffer compatibility APIs retained for in-tree callers and are scheduled for
removal/migration. They are not part of the safe API guarantee. Some are
`unsafe func`s; the ordinary raw entries are tracked as migration debt, not as
an endorsement.

Reviewed explicit raw accessors (allowed):

- `std::span::data`, `std::mut_span::data` — view construction/inspection.
- `std::CString::as_ptr` — audited C interop.
- `net::TcpStream::release_raw` — ownership transfer to TLS.
- `Window::native_handle` / `WebView` raw platform accessors, if present.
- `tls::ssl_*`, `tls::mbedtls_*`, certificate/key primitives — moved to the
  `tls::raw` migration target; the safe `tls::TlsStream`, `TlsServerStream`,
  `CertificateChain`, `PrivateKey`, and `TlsServerConfig` wrappers are the
  supported surface.

Run `scripts/safety-sharpen-api-audit.sh` for the current count by library.

## Reviewed legacy raw inventory (2026-11-08)

These are the deliberate raw-buffer/FFI compatibility entry points retained for
in-tree callers. They are not part of the safe surface and are counted by
`scripts/safety-sharpen-api-audit.sh`; new user code uses the safe facade.

| Library | raw-pointer public funcs | public unsafe funcs/decls |
|---|---:|---:|
| std | 70 | 23 |
| core | 0 | 0 |
| fs | 66 | 0 |
| path | 8 | 8 |
| process | 0 | 0 |
| environment | 0 | 6 |
| net | 18 | 3 |
| http | 6 | 4 |
| tls | 143 | 7 |
| encoding | 13 | 0 |
| crypto | 22 | 0 |
| compression | 4 | 0 |
| archive | 41 | 0 |
| image | 33 | 0 |
| audio | 11 | 0 |
| font | 10 | 0 |
| regex | 2 | 2 |
| json | 6 | 0 |
| mime | 4 | 0 |
| datetime | 0 | 0 |
| uuid | 0 | 0 |
| osrand | 1 | 1 |
| cbor | 3 | 0 |
| window | 79 | 1 |
| webview | 52 | 2 |
| page | 13 | 1 |
| components | 0 | 0 |
| router | 2 | 0 |
| server | 1 | 0 |

`process`, `datetime`, `uuid`, `components`, and `core` have zero ordinary
public raw-pointer functions in the audited surface.
