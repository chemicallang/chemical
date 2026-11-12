# Safety Sharpen — Status

Plan: [`../safety-sharpen-plan.md`](../safety-sharpen-plan.md)

Status values: `[ ]` not started, `[~]` in progress, `[x]` done, `[!]` blocked.

| ID | Phase | Area | Job | Status | Owner | Evidence (commit / command / timestamp) |
|---|---|---|---|---|---|---|
| P1.1 | 1 | compiler | release build repair + baseline | [x] | agent | GenericInstantiator guard; release builds; metrics.json; 2026-11-07 |
| P1.2 | 1 | language | keep dyn/DynBox + dynamic-call tests | [x] | agent | existing dyn/DynBox/upcast/auto-trait tests pass in main+safety |
| P1.3 | 1 | language | remove associated types + removed suite | [x] | agent | parser REMOVED-001; negative+--removed 12/12 |
| P1.4 | 1 | language | keep tuple structs/unit; remove external/generic inherent impls | [x] | agent | parser REMOVED-002; tuple-struct tests pass |
| P1.5 | 1 | language | remove nested/slice patterns; keep flat/variant forms | [x] | agent | parser REMOVED-003/004; flat drop tests pass |
| P1.6 | 1 | language | remove Box<[T]> and range slicing | [x] | agent | parser REMOVED-005/006; subview tests pass |
| P1.7 | 1 | compiler | release-gating size consolidation | [x] | agent | release 4,037,816 bytes; -O2 + sections + gc-sections + strip |
| P1.8 | 1 | docs | reconcile docs/skills and freeze new syntax | [x] | agent | AGENTS/skill/docs/REMOVED/BREAKING updated |
| P2.1a | 2 | std | IoError + Read/Write/Seek | [x] | agent | std/src/io.ch; fs::File Read/Write impl; libs 736/736 |
| P2.1 | 2 | std | safe primitives + view lifetimes | [x] | agent | IoError/Read/Write/Seek; CString/CStr; vector try_reserve/try_push/get_opt/get_mut_opt/try_set/try_pop; span/str Option getters; Result/Option methods; view lifetimes are compiler-enforced on &[T]/&str; std check clean |
| P2.2 | 2 | fs/process | filesystem/process/env safe APIs | [x] | agent | fs RAII + safe I/O/directory/metadata helpers; owning process::Child; env mutation unsafe; PathBuf safe methods + raw path funcs marked unsafe; process 127/127, libs 818/818 |
| P2.3 | 2 | net/http/tls | safe I/O, HTTP, TLS | [x] | agent | safe TcpListener/TcpStream + async I/O + loopback; Body read_into/read_all; HeaderMap get_str; TLS client/server/cert/key config; MIREmitter nested field-address fix; server 10/10 |
| P2.4 | 2 | data/media | data/media/crypto/parser fuzz | [x] | agents | encoding/crypto/compression/archive/image/audio/font safe facades + regex/json/mime/cbor safe forms; malformed-input tests; fixed corpus under lang/tests/fuzz/corpus; --fuzz/--cbor wiring |
| P2.5 | 2 | web/ui | window/webview/page/components/router/server | [x] | agent | flat `safe_page_*` escaping/parameters; window/webview owning handles/callbacks; router/components/server safe surfaces; webview 51/51, universal 368/368; !Send negative 2/2; GUI compile-only exception documented |
| P2.6 | 2 | concurrency | threads/async safe APIs | [x] | agent | safe Condvar, ThreadPool submit_fn, internal executor state, Pin/spawn_send tests; `thread::scope` spike blocked on closure HRTB + capture-witness inference + whitelist, so checked `@scoped_callback` remains (P2.6-concurrency.md); TSAN passes |
| P2.7 | 2 | compiler | test/compiler/plugin/build APIs | [x] | agent | compiler::api, test_env::safe, test_report::safe, lab::safe; md_cbi migrated to parser facade; plugins 1305/1305; lab lifetime-generic handle limitation documented |
| P2.8a | 2 | migration | transformer + consumer inventory | [x] | agent | consumers.md (209 in-tree modules, no remote deps); scripts/safety-migrate-removed.py mechanical migration + REMOVED hints; BREAKING policy |
| P2.8 | 2 | release | API audit, docs, migration, gate | [x] | agent | snapshots + unsafe maps + audit allowlist (legacy raw migration debt recorded); ASAN/TSAN green; no-panic sample; release 4,037,816 B; full matrix green except documented GUI/Windows target execution exceptions |

## Current baseline (safe-API release)

- Release `TCCCompiler`: **4,037,816 bytes** — under the 5,000,000-byte hard gate;
  the 4 MB goal is missed by 37,816 bytes. `safety_sample_compile_ms` 286 ms,
  `safety_sample_max_rss_kb` 35,120 (recorded in `metrics.json`).
- Final TCC gates: main 2478/2478; negative 863/863; removed 12/12; safety
  191/191; async 84/84; libs 882/882; regexp 154/154; cbor 23/23; fuzz 3/3;
  api 4/4; process 128/128; server 10/10; plugins 1305/1305; interpret
  1938/1938; TLS 605/605; webview 51/51; universal 368/368.
- Final LLVM gates: main 2476/2476; negative 863/863; safety 191/191; libs
  882/882; async 84/84; regexp 154/154; process 128/128; server 10/10.
- `MIRTests`: 4749 checks. TSAN: pass. ASAN (actual instrumented compiler;
  leak detection disabled for process-lifetime arenas): libs 882/882, process
  128/128, server 10/10. Safety audit: no growth against
  `lang/docs/safety/audit-baseline.json` (run with `--jobs 1` until the
  parallel safety-report walk is fixed).
- API snapshots are in `api/`, unsafe maps in `unsafe/`, and the reviewed
  legacy-raw allowlist is `API_ALLOWLIST.md`. The safe surface is additive;
  legacy raw-buffer entry points remain as explicit migration debt and are
  counted by `scripts/safety-sharpen-api-audit.sh`.
- GUI runtime execution is display/toolkit-gated; window/webview are
  compile-checked and webview API tests pass in this environment. Windows/macOS
  execution remains a cross-target exception recorded in `TARGETS.md`.
- Release-blocker fixes: private `fs::File`/`http::Body` ownership state,
  unsafe `Client::set_ca_chain`, borrowed-lifetime `TlsServerConfig<'a>`,
  validated `Image`/`Font` safe boundaries, copied page parameter keys,
  corrected ASAN build-dir wiring, deterministic `--jobs 1` audit, and
  per-module audit comparison. Compiler UAFs found by ASAN were fixed in the
  safety-report/comptime-binding paths, the interface `users` registry is now
  mutex-protected during parallel generic instantiation, and LLVM module ASTs
  are retained until the job allocator dies instead of being cleared while
  deferred readers can still touch them. The TCC path keeps its existing
  per-module clear, which passes the full suite.
- Reviewer sign-off for this release: the P2.4/P2.5/P2.6/P2.7 subagent reviews
  plus the integrated gate run above (recorded 2026-11-08).

## API-freeze wave (2026-11-09)

- `c24dd6175` freezes encoding/crypto/compression/archive/image/audio/font/
  regex/json/cbor/uuid/datetime/osrand/mime: canonical safe names at module
  level, raw pointer/out-buffer forms under `::raw`, borrowed/owned views,
  checked constructors, and typed errors.
- `aa68f6a7a` freezes path/fs/process/environment and net/http/tls:
  borrowed `Path`/`PathBuf`, canonical `fs::read`/`write`/`read_dir`,
  `Command`/`Stdio`/`Output`, owning `TcpListener`/`TcpStream`,
  `HttpError` + private `Transport`, and safe `TlsConfig`/`Certificate`/
  `CertificateChain`/`TlsStream` with all mbedTLS-shaped code under `tls::raw`.
- `84dd78dbd` freezes web UI and compiler/runtime surfaces: canonical page
  append/parameter methods with CBI raw methods, `SharedAssetsHandle`, safe
  window timers/clipboard/monitor methods, `webview`/`window` raw namespaces,
  private reporter/executor state, safe compiler arena allocation, and
  `async` channel error types.

**Std view lifetime status:** `span<'a, T>` / `mut_span<'a, T>` (`0ea3998cf`),
`CStr<'a>` (`63188b8bc`), the postfix-lifetime `str 'a` (`c8df07c17`), and
`core::Pin<'a, T>` (`c01dd4da3`) now carry their borrow lifetime. The
vector-to-span constructor is the frozen construction form; MIR view escape
checks fire (`60ff5ece7`). Span cursors cast the element to `*T` while reading
split predicates so nested generic instantiations keep a concrete element type
instead of lowering to an opaque local. `Option::take` is the non-panicking
`Option<T>` form with `take_unchecked` for checked callers (`6ed7d0360`).
Static-lifetime escapes are rejected for all view shapes; TCC/LLVM main, libs,
negative, safety, async, and web suites are green.

**Container accessor names (frozen):** `vector`, `span`, `mut_span`, `string`,
`str`, and `string_view` now expose non-panicking `get` (`Option<&T>` for
vector/span, `Option<char>` for strings) and checked panicking `at` forms;
`vector` also has `get_copy`, `at_ref`, and `at_mut`, while `span`/`mut_span`
keep explicit raw `at`/`at_ref`/`at_mut` forms. Maps expose
`get(key) : Option<&Value>`. Raw `get_ptr` accessors remain explicitly
allowlisted. String search is `find`/`find_last : Option<size_t>`; the explicit
legacy `find_or_npos`/`find_last_or_npos` forms remain for in-tree migration.
`SpanView<T>` supplies `vector::as_span`/`as_mut_span` without eager
specialization. These names are the frozen release surface.

**Safe-by-default naming wave (2026-11-12).** The public surface no longer
exposes a parallel `*_safe` / `safe_*` / `::safe::` spelling: the plain name is
the safe API and raw-pointer/FFI forms moved under `raw` (or a `*_native` free
function that takes/returns the `Native*` platform type).
- `image`, `audio`, `font`: module-level raw codec/accessor functions moved to
  `image::raw` / `audio::raw` / `font::raw`; the private `*_safe_*` helpers were
  renamed to `decode_*` / `encode_*_bytes` / `save_to_path`. The canonical
  surface is `Image.load/from_*/save`, `Audio.load/from_*/save`, and
  `Font.from_path/load/metrics`.
- `page`: `safe_page_*` aliases removed; `escape_js`, `escape_json`,
  `append_html_escaped`, `append_js_escaped`, `set_parameter`,
  `get_parameter_owned`, and `PageParam` are the canonical names.
- `window`/`webview`: the safe owning handles are `window::Window` /
  `webview::WebView`; the platform state structs are `NativeWindow` /
  `NativeWebView`, and the raw `webview::create/open_url/open_html` helpers are
  `*_native`.
- `router`, `server`, `components`, `test_env`, `test_report`, `lab`: the
  `safe` sub-namespace was folded into the module namespace (`router::set_request`,
  `server::serve_static`, `test_env::TestEnvHandle`, `test_report::TestOutcomeSnapshot`,
  `lab::run`, ...).
- `osrand::random_u32_safe`/`random_u64_safe` -> `random_u32`/`random_u64`.
- Audit tooling: `scripts/safety-sharpen-api-audit.sh` now reports
  `RAW_NONRAW` (public raw-pointer functions declared outside a
  `raw`/`ffi`/`sys` namespace) with a string/char-literal-aware brace scanner.
  `image`, `audio`, `font`, `fs`, `path`, `archive`, `encoding`, `crypto`,
  `compression`, `json`, `mime`, `cbor`, and `router` are at zero; the
  remaining nonzero entries are the reviewed allowlist. The snapshot script now
  fails closed when `rg` is missing instead of silently emitting empty files.
- Gates: main 2485/2485, libs 882/882, process 127/127 (window, Windows
  host), server 10/10, api 4/4, safety 191/191, negative 867/867, plugins
  1305/1305; webview test module, universal_webview, and `universal_test`
  compile-check clean. (One repeat main-suite compile hung in symres; an
  immediate `--no-run` rebuild plus direct `tests-tcc.exe` run was clean. Treat
  it as the known intermittent parallel-symres deadlock, not a suite failure.)

**Server / ownership wave (2026-11-13).** First slice of the http/net shape
wave, in the planned order.

- **C1 - consuming `self` is a move (compiler).** `MIRLowerer::append_receiver_arg`
  now emits `MoveOut` + `mark_moved` for a by-value aggregate receiver (Copy
  exempt) and rejects a move out of a pointer/reference projection with
  `SAFE-MOVE-004`. Before the fix, `func consume(self)` destroyed the receiver
  twice (callee copy + caller copy), a silent double free in a safe program. A
  receiver that is a field/element (`o.inner.consume()`) or a call temporary
  (`make().consume()`) now clears the exact parent/element/statement drop flag,
  so the parent never destroys the moved subobject. Tests: `lang/tests/safety`
  (local/temporary/field single-drop, field hand-off) and
  `lang/tests/negative/src/move_semantics.ch` (pointer shape rejected, by-value
  accepted). This unblocks `Box::into_inner(self)`,
  `Rc/Arc::try_unwrap(self)`, `TlsStream::into_box(self)`, and a consuming
  `Server::serve(self)`.
- **C2 - `http::Client` owns its trust store (http/tls).** `set_ca_chain` takes
  the chain by value; the system bundle is loaded and cached once per client
  (`ensure_ca`), and only for TLS hops that verify the peer. The old
  `Client::tls_config` attached a *local* chain and returned the config, freeing
  the CA chain before the handshake: a use-after-free on every HTTPS request
  that used the system bundle. `Client` request methods now take `&mut self`.
  This also removes the per-request system-bundle read/PEM parse.
- Verified: main 2485/2485, libs 882/882, server 10/10, safety 193/193,
  negative 869/869; tls/http/http-client modules compile (`lang/tests/tls`
  needs the POSIX/python harness and is not run on this host).
- Compiler gaps found while doing C2 (repro + impact in `BLOCKED.md`):
  `Option<&'a Struct>` cannot instantiate (blocks a lifetime-typed
  `TlsConfig<'a>`; the trust store was made owned instead), `&'a self` receivers
  are rejected, async interface methods lose their future (blocks
  `AsyncRead`/`AsyncWrite`, i.e. C7), and `using` names are unresolved inside
  generic instantiation.

**Compiler-blocked API follow-ups:** the consuming forms
`Box::into_inner(self)`, `Rc::try_unwrap(self)`, `Arc::try_unwrap(self)` and
`map::get_mut -> Option<&mut Value>` are not shipped yet. The receiver copy is
fixed (C1 above), but pattern-binding `Option<&mut T>` from an external generic
still loses mutability, so `map::get_mut` needs the compiler fix first. The
library-level consuming methods (`Image.load`-style builders and the TLS
`into_box`/owner-transfer helpers) can now be migrated to `self` consumers; the
`&mut self` wrappers stay until each call site is updated. The current `&mut
self` extraction and map `get` surface remain stable until then.

**`vector::as_span`:** the canonical methods are now provided by the generic
`SpanView<T>` interface impl (`as_span`/`as_mut_span`). A trait impl is
instantiated only when called; putting the same methods directly on
`vector<T>` made every vector instantiation eagerly specialize `span<T>` and
`span_split<T>`, growing the safety sample by ~14% compile time and ~3 MB RSS.
The trait form keeps the API while restoring the prior metrics. The C backend
also recovers a concrete sret return from the callee symbol when a nested
generic call reaches MIR with an opaque return type, preventing `void*`
result locals and missing hidden result pointers in generated C.

## Phase 1 notes

- `dyn`/`DynBox`, tuple structs, unit tuple, core tuples, flat destructuring, slices/`str`, and generic impls stay.
- Associated types, external/generic inherent impls, nested/slice patterns, `Box<[T]>`, and range-slicing syntax are removal targets.
