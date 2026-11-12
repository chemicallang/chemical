# Safe-API release notes (v0.6.0)

This release is the first safe-API release. It keeps the Phase 1 language
removals and adds the safe library surfaces:

- One I/O error boundary (`std::io::IoError`) and `Read`/`Write`/`Seek`.
- **Safe by default:** every public library name is the safe form; raw-pointer
  and FFI entry points live under `<lib>::raw` / `<lib>::ffi` (no `*_safe` /
  `safe_*` / `::safe::` spellings). See `BREAKING.md` for the rename table.
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
- The ASAN gate instruments the compiler and now runs the actual ASAN build;
  generated test executables are not yet ASAN-linked. Leak detection is
  deliberately disabled for compiler-side process-lifetime arenas.
- Legacy raw-buffer APIs remain as reviewed migration debt. New code uses the
  additive safe surface; raw entry points that retain pointers are `unsafe`,
  and the rest are explicitly enumerated by the API audit.
- The 2026-11-09 API-freeze wave makes the safe names canonical for data/media,
  path/fs/process/environment, net/http/tls, web UI, and compiler/runtime
  libraries, with legacy pointer APIs under explicit `raw`/`ffi` namespaces.
- `span`/`mut_span`/`str` are not frozen for lifetime parameters yet: the
  compiler does not enforce lifetimes carried by user-defined generic structs,
  so a library-only lifetime change would be unsound. This is the remaining
  compiler blocker for the ideal std view API.
