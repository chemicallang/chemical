# Chemical UI — status

**Updated by:** the parent agent after every packet · **Date:** 2026-11-26

`State`: `todo` · `in-progress` · `review` · `done` · `blocked` · `deferred`

| Packet | State | Commit | Tag | Evidence | Bench delta | Notes |
|--------|-------|--------|-----|----------|-------------|-------|
| P0.1 Feasibility probe | todo | — | — | — | — | |
| P0.2 Freeze contracts | todo | — | — | — | — | |
| P0.3 Scaffolding | todo | — | — | — | — | |
| P1.1 Math/geometry/color | todo | — | — | — | — | |
| P1.2 Arena/IDs/keys | todo | — | — | — | — | |
| P1.3 View builder | todo | — | — | — | — | |
| P1.4 Reconciler/tree | todo | — | — | — | — | |
| P1.5 Style/tokens/themes | todo | — | — | — | — | |
| P1.6 Layout v1 | todo | — | — | — | — | |
| P1.7 Layout v2 | todo | — | — | — | — | |
| P1.8 Display list/painter | todo | — | — | — | — | |
| P1.9 CPU rasterizer | todo | — | — | — | — | |
| P1.10 Headless/app loop | todo | — | — | — | — | |
| P1.11 Input/focus/hit | todo | — | — | — | — | |
| P1.12 Semantics/audit | todo | — | — | — | — | |
| P1.13 Typed-error boundary/panic audit | todo | — | — | — | — | stage gate |
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
| P4.5 Menus/popovers/palette | todo | — | — | — | — | |
| P4.6 Dialogs/overlays | todo | — | — | — | — | |
| P4.7 Layout/navigation | todo | — | — | — | — | |
| P4.8 Virtual list/table/tree | todo | — | — | — | — | |
| P4.9 Forms/validation | todo | — | — | — | — | |
| P4.10 Feedback/status | todo | — | — | — | — | |
| P4.11 Patterns/guidelines | todo | — | — | — | — | stage gate |
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
| P7.3 Chat/IDE app | todo | — | — | — | — | |
| P7.4 API gap fixes | todo | — | — | — | — | |
| P7.5 Perf hardening | todo | — | — | — | — | |
| P7.6 Safety hardening | todo | — | — | — | — | |
| P7.7 Docs/README | todo | — | — | — | — | |
| P7.8 Media Library example | todo | — | — | — | — | |
| P7.9 Publish ui-examples repo | todo | — | — | — | — | stage gate; gh push |
| P8.1 Release audit | todo | — | — | — | — | |
| P8.2 Packaging/final | todo | — | — | — | — | release |

## Blockers

| ID | Packet | Description | Owner | Status |
|----|--------|-------------|-------|--------|
| — | — | — | — | — |
