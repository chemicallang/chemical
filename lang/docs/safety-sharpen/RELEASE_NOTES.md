# Safe-API release notes (v0.6.0)

This release is the first safe-API release. It keeps the Phase 1 language
removals and adds the safe library surfaces:

- One I/O error boundary (`std::io::IoError`) and `Read`/`Write`/`Seek`.
- Owning handles and RAII for files, processes, TCP streams, and TLS streams.
- Safe path values (`path::PathBuf`), safe `CString`/`CStr`, and fallible
  vector operations.
- Safe network, HTTP body/header, and TLS client/server entry points.
- Data/media/crypto safe facades and malformed-input tests.
- Safe concurrency/test/compiler facades where the language can express the
  bounds; raw CBI/FFI remains Tier C.

Known limits, tracked for Phase 3:

- Full generic/async/interface no-panic effect propagation (P3.1).
- Fallible string/allocation coverage (P3.2).
- remaining move/borrow/HRTB closure gaps (P3.3).
- Windows/macOS runners and GUI/webview execution are environment-gated; the
  per-library target matrix is in `TARGETS.md`.
- Legacy raw-buffer APIs remain under explicit unsafe/allowlisted surfaces.
