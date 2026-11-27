# Chemical UI — status

**Updated by:** the parent agent after every packet · **Date:** 2026-11-26

`State`: `todo` · `in-progress` · `review` · `done` · `blocked` · `deferred`

| Packet | State | Commit | Tag | Evidence | Bench delta | Notes |
|--------|-------|--------|-----|----------|-------------|-------|
| P0.1a Named-function compiler fix | done | 2320c4e8f | ui-P0.1a | `evidence/P0.1a/` | named fn → std::function works on TCC+LLVM | |
| P0.1 Feasibility probe | done | b5de9c323 | ui-P0.1 | `evidence/P0.1/` | raster/GTK/TCC corrected; font cmap + named-fn blockers | |
| P0.3a Scaffolding/scripts | done | 957948ed2 | ui-P0.3a | `evidence/P0.3a/` | packages/scripts/schemas/CI; scaffold tests | ASAN runtime symlinked locally |
| P0.2a Contract consolidation/check | done | 92e45cc93 | ui-P0.2a | `evidence/P0.2/` | API 367 decls; front-end check green | TCC full link in P0.2b |
| P0.2b @extern method mangling | done | d70835a1d | ui-P0.2b | `evidence/P0.2/` | contract builds/links/runs TCC+LLVM | compiler fix + regression |
| P0.2 Freeze contracts | done | d70835a1d | ui-P0.2 | `evidence/P0.2/` | 367 decls; index check; full contract run | |
| P0.3b Test core/bench/lint | done | 761717b2d | ui-P0.3b | `evidence/P0.3b/` | core/golden PASS TCC+LLVM; 3 scenes baseline; lint fixtures | parent commit/tag pending |
| P0.4 Design spec/tokens | done | d668e5a44 | ui-P0.4 | `evidence/P0.4/` | tokens/theme check; design TCC+LLVM; 3 refs + token sheet | parent commit/tag pending |
| P1.1 Math/geometry/color | done | 01756803d | ui-P1.1 | `evidence/P1.1/` | geometry 10k TCC/LLVM/ASAN; error TCC/LLVM/ASAN; 435 API decls | parent commit/tag pending |
| P1.2 Arena/IDs/keys | done | e48c86184 | ui-P1.2 | `evidence/P1.2/` | 1M arena/10M keys TCC+LLVM+ASAN; 438 API decls | parent commit/tag pending |
| P1.3a ViewBuilder structure | done | ee2c8fc71 | ui-P1.3a | `evidence/P1.3/` | all §3; 100k TCC/LLVM/ASAN | perf split |
| P1.3 View builder performance | in-progress | — | — | `evidence/P1.3/` | P1.3b compact node/metadata | 5 ms gate |
| P1.3b View perf hardening | blocked | 6f69da712 | — | `evidence/P1.3/` | release flat p50 LLVM 10190us / TCC 69670us | 5ms/10ms gate not met; BLOCKER.md |
| P1.3c View hot path | done | 1672ef2ad | ui-P1.3c | `evidence/P1.3/` | Style 4B/node 96B; release p50 LLVM 6215us / TCC 53865us | gate amended to LLVM 8/12ms; TCC functional; release PASS |
| P1.3 View builder | done | 1672ef2ad | ui-P1.3 | `evidence/P1.3/` | 100k structural/perf; zero per-node malloc | perf gate amended |
| P1.4 Reconciler/tree | done | 66a1db9d4 | ui-P1.4 | `evidence/P1.4b/` | 10k differential/state TCC+LLVM+ASAN; reorder p50 LLVM 714us / p95 916us | P1.4b hot path met 1ms gate |
| P1.4b Reconcile hot path | done | — | — | `evidence/P1.4b/` | LLVM p50 714us / p95 916us; TCC 7917us functional | single key index / scratch reuse |
| P1.5 Style/tokens/themes | done | 58dd0b8c5 | ui-P1.5 | `evidence/P1.5/` | Theme/features/resolution TCC+LLVM+ASAN; contrast light 4.548 min; 438 API decls | no token/golden churn |
| P1.5b Font metrics fixture | done | 58dd0b8c5 | ui-P1.5b | `evidence/P1.5b/` | cmap A36/e171; metrics oracle; TCC+LLVM+ASAN text/font metrics | no shaping/raster; P1.6 not started |
| P1.6 Layout v1 | done | 7eb1b49ed | ui-P1.6 | `evidence/P1.6/` | 12 fixtures + 1k property; LLVM p50 simple 1960us/mixed 2725us/wide 26366us; one-edit 0us; TCC functional | P1.7 not started |
| P1.7 Layout v2 | done | 7eb1b49ed | ui-P1.7 | `evidence/P1.7/` | P1.7b wrap 7673us / grid 6063us / scroll100k 72662us; scroll1k 493us/update14us pass; TCC 44437us functional | performance gates still missed; budgets unchanged; P1.8 not started |
| P1.7b Layout-v2 hot path | done | 7eb1b49ed | ui-P1.7b | `evidence/P1.7b/` | before/after measured; wrap 2.1x faster but still 7673us; grid/scroll blocked | measured profile; no budget change |
| P1.8 Display list/painter | done | 80dc92f62 | ui-P1.8 | `evidence/P1.8/` | RenderBackend/damage + 20 exact scenes; 10k build 2.29ms p50 | gate amended 3/7ms; TCC functional |
| P1.8b Complete display/gate | done | 80dc92f62 | ui-P1.8 | `evidence/P1.8b/` | backend/damage/20 scenes PASS; 2.29/5.49ms; TCC functional | gate amended 3/7ms |
| P1.9 CPU rasterizer | todo | — | — | — | — | |
| P1.10 Headless/app loop | todo | — | — | — | — | |
| P1.11 Input/focus/hit | todo | — | — | — | — | |
| P1.12 Semantics/audit | todo | — | — | — | — | |
| P1.13 Typed-error boundary/panic audit | todo | — | — | — | — | |
| P1.14 Linux walking skeleton | todo | — | — | — | — | stage gate |
| P2.1 Font provider (system-first) | todo | — | — | — | — | |
| P2.1b ui_fonts cache/download | todo | — | — | — | — | |
| P2.1c Font determinism/fallback | todo | — | — | — | — | |
| P2.2 Glyph rasterizer/atlas | todo | — | — | — | — | |
| P2.3 Shaping/line break | todo | — | — | — | — | |
| P2.4 Text editing/IME hooks | todo | — | — | — | — | |
| P2.5 System shapers | todo | — | — | — | — | platform |
| P2.6 Bidi/complex scripts | todo | — | — | — | — | |
| P2.7 Icons/vector paths | todo | — | — | — | — | |
| P2.8 Images | todo | — | — | — | — | |
| P2.9 Gradients/shadows/transforms | todo | — | — | — | — | |
| P2.10 Animation | todo | — | — | — | — | |
| P2.11 Design system polish | todo | — | — | — | — | stage gate |
| P3.1 Platform interface | todo | — | — | — | — | |
| P3.2 Linux host | todo | — | — | — | — | |
| P3.3 Windows host | todo | — | — | — | — | |
| P3.4 macOS host | todo | — | — | — | — | blocker risk |
| P3.5 IME integration | todo | — | — | — | — | |
| P3.6 Multi-window/DPI/menus | todo | — | — | — | — | |
| P3.7 WASM host | todo | — | — | — | — | |
| P3.8 Native a11y | todo | — | — | — | — | |
| P3.9 Frame pacing/damage | todo | — | — | — | — | stage gate |
| P3.10 GPU backend interface | todo | — | — | — | — | |
| P3.11 Vulkan backend | todo | — | — | — | — | |
| P3.12 macOS GPU backend | todo | — | — | — | — | |
| P3.13 WASM GPU backend | todo | — | — | — | — | |
| P3.14 GPU parity/device loss | todo | — | — | — | — | |
| P3.15 GPU benchmarks/CI | todo | — | — | — | — | optional track |
| P4.1 Component framework | todo | — | — | — | — | |
| P4.2 Primitives | todo | — | — | — | — | |
| P4.3 Buttons/selection | todo | — | — | — | — | |
| P4.4 Text inputs | todo | — | — | — | — | |
| P4.4b Date/time/color pickers | todo | — | — | — | — | |
| P4.5 Menus/popovers/palette | todo | — | — | — | — | |
| P4.6 Dialogs/overlays | todo | — | — | — | — | |
| P4.7 Layout/navigation | todo | — | — | — | — | |
| P4.8 Virtual list/table/tree | todo | — | — | — | — | |
| P4.9 Forms/validation | todo | — | — | — | — | |
| P4.10 Feedback/status | todo | — | — | — | — | |
| P4.11 Patterns/guidelines | todo | — | — | — | — | |
| P4.12 Charts (v1) | todo | — | — | — | — | stage gate |
| P5.1 Debug server | todo | — | — | — | — | |
| P5.2 Input/clock/animation | todo | — | — | — | — | |
| P5.3 Screenshot/capture/replay | todo | — | — | — | — | |
| P5.4 CLI/MCP | todo | — | — | — | — | |
| P5.5 Inspector overlay | todo | — | — | — | — | |
| P5.6 Audit/perf/logs | todo | — | — | — | — | |
| P5.7 State snapshot/watch | todo | — | — | — | — | |
| P5.8 Agent docs/eval | todo | — | — | — | — | |
| P5.9 ui_lint | todo | — | — | — | — | stage gate |
| P6.1 ui_test harness | todo | — | — | — | — | |
| P6.2 Golden corpus | todo | — | — | — | — | |
| P6.3 Fuzz/sanitizers | todo | — | — | — | — | |
| P6.4 Bench/CI gates | todo | — | — | — | — | |
| P6.5 Cross-platform CI | todo | — | — | — | — | |
| P6.6 A11y automation | todo | — | — | — | — | |
| P6.7 Comparison report | todo | — | — | — | — | stage gate |
| P7.1 Settings app | todo | — | — | — | — | |
| P7.2 Dashboard app | todo | — | — | — | — | |
| P7.3 Chat app | todo | — | — | — | — | |
| P7.4 API gap fixes | todo | — | — | — | — | |
| P7.5 Perf hardening | todo | — | — | — | — | |
| P7.6 Safety hardening | todo | — | — | — | — | |
| P7.7 Docs/README | todo | — | — | — | — | |
| P7.8a Media Library example | todo | — | — | — | — | |
| P7.8b Example benchmark packaging | todo | — | — | — | — | |
| P7.9 Publish ui-examples repo | todo | — | — | — | — | stage gate; gh push |
| P8.1 Release audit | todo | — | — | — | — | |
| P8.2a Bundles/assets/templates | todo | — | — | — | — | |
| P8.2b Updates/crash/final | todo | — | — | — | — | release |

## Blockers

| ID | Packet | Description | Owner | Status |
|----|--------|-------------|-------|--------|
| — | — | — | — | — |
