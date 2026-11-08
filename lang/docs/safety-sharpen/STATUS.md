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

## Current baseline (Phase 2 release candidate)

- Release `TCCCompiler`: **4,037,816 bytes** — under the 5,000,000-byte hard gate;
  the 4 MB goal is missed by 37,816 bytes. `safety_sample_compile_ms` 250 ms,
  `safety_sample_max_rss_kb` 32,944 (recorded in `metrics.json`).
- Final TCC gates: main 2478/2478; negative 863/863; removed 12/12; safety
  191/191; async 84/84; libs 868/868; regexp 153/153; cbor 23/23; fuzz 3/3;
  api 4/4; process 127/127; server 10/10; plugins 1305/1305; interpret
  1938/1938; TLS 605/605; webview 51/51; universal 368/368.
- Final LLVM gates: main 2476/2476; negative 863/863; safety 191/191; libs
  868/868; async 84/84; regexp 153/153; process 127/127; server 10/10.
- `MIRTests`: 4749 checks. TSAN: pass. ASAN/leak: libs 868/868, process
  127/127, server 10/10. Safety audit: no growth against
  `lang/docs/safety/audit-baseline.json`.
- API snapshots are in `api/`, unsafe maps in `unsafe/`, and the reviewed
  legacy-raw allowlist is `API_ALLOWLIST.md`. The safe surface is additive;
  legacy raw-buffer entry points remain as explicit migration debt and are
  counted by `scripts/safety-sharpen-api-audit.sh`.
- GUI runtime execution is display/toolkit-gated; window/webview are
  compile-checked and webview API tests pass in this environment. Windows/macOS
  execution remains a cross-target exception recorded in `TARGETS.md`.
- Reviewer sign-off for this release candidate: the P2.4/P2.5/P2.6/P2.7
  subagent reviews plus the integrated gate run above (recorded 2026-11-08).

## Phase 1 notes

- `dyn`/`DynBox`, tuple structs, unit tuple, core tuples, flat destructuring, slices/`str`, and generic impls stay.
- Associated types, external/generic inherent impls, nested/slice patterns, `Box<[T]>`, and range-slicing syntax are removal targets.
