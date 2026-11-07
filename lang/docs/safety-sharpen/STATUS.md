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
| P2.1 | 2 | std | safe primitives + view lifetimes | [~] | agent | vector try_push/try_reserve, CString, IoError/Read/Write; view lifetimes pending |
| P2.2 | 2 | fs/process | filesystem/process/env safe APIs | [~] | agent | fs safe read/write/open/create; owning process::Child + &mut operations; environment set/unset/all unsafe; path pending |
| P2.3 | 2 | net/http/tls | safe I/O, HTTP, TLS | [~] | agent | safe net::TcpListener/TcpStream + io traits + loopback test; HTTP/TLS pending |
| P2.4 | 2 | data/media | data/media/crypto/parser fuzz | [ ] | agent | pending |
| P2.5 | 2 | web/ui | window/webview/page/components/router/server | [ ] | agent | pending |
| P2.6 | 2 | concurrency | threads/async safe APIs | [ ] | agent | pending |
| P2.7 | 2 | compiler | test/compiler/plugin/build APIs | [ ] | agent | pending |
| P2.8a | 2 | migration | transformer + consumer inventory | [ ] | agent | pending |
| P2.8 | 2 | release | API audit, docs, migration, gate | [ ] | agent | pending |

## Current baseline

- Release `TCCCompiler`: **4,037,816 bytes** — under the 5,000,000-byte gate (4 MB goal missed by 37,816 bytes).
- Release build repaired: `compiler/generics/GenericInstantiator.h` optional save/restore now guards `DEBUG`.
- Release flags: `-O2`, `-ffunction-sections`, `-fdata-sections`, `-fno-unwind-tables`, `-fno-asynchronous-unwind-tables`, `-Wl,--gc-sections`, `-s`.
- Phase 1 gates: TCC main 2478/2478; LLVM main 2476/2476; negative TCC/LLVM 861/861; removed 12/12; safety TCC/LLVM 190/190; async 83/83; libs 734/734; plugins 1305/1305; interpret 1938/1938; regexp 143/143; process 127/127.
- P2.2 note: `File` uses explicit `fs::close_file` for now; RAII restore is blocked on a real pattern-payload field move (see `BLOCKED.md`).
- Known baseline issue: TCC `--server` fails in generated C at `http::FileServer::serve_http` cleanup (`&(&(&p->headers)->headers)`), unrelated to Phase 1 removals; recorded in `BLOCKED.md` for Phase 2 codegen work.
- Current test counts: see `lang/docs/safety-sharpen/metrics.json` after the first metrics run.

## Phase 1 notes

- `dyn`/`DynBox`, tuple structs, unit tuple, core tuples, flat destructuring, slices/`str`, and generic impls stay.
- Associated types, external/generic inherent impls, nested/slice patterns, `Box<[T]>`, and range-slicing syntax are removal targets.
