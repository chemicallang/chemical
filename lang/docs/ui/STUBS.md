# Chemical UI — declared stubs

No runtime stub ships without a row here. A row names the packet that removes it.

| Stub | Location | Behaviour | Removed by | Status |
|------|----------|-----------|------------|--------|
| `ui-shot.sh` | `scripts/ui-shot.sh` | Validates arguments, exits **78** (`EX_CONFIG`), writes **no** image. | P1.10 | declared |
| Scaffold markers | `lang/libs/ui*/src/*.ch` | `__ui_scaffold*()` returns the API version; no UI API. | P0.2 (contracts), then P1.1–P1.14 | declared |
| `ui_test_core` | `lang/tests/ui/src/core.ch` | Test-only tree, settle, snapshots, golden compare; not the production `UiHarness`. | P6.1 | declared |
| Bench scaffold counters | `lang/tests/ui_bench/src/main.ch` | `renderer:"none"`, deterministic counters, no frame timing. | P1.10 | declared |
| Golden fixture | `lang/tests/ui/fixtures/pixels/` | 128×128 checker proves image encode/decode; not a renderer golden. | P1.9/P1.10 (first real goldens) | declared |
| Minimal lint | `scripts/ui-lint.sh` | `UI-KEY-001`, `UI-PANIC-001`, `UI-RAW-001` only. | P5.9 | declared |
| API-index identical duplicate | `scripts/ui-api-index.sh` | Exact duplicate declarations are recorded; conflicting duplicates are fatal. | P0.2 fixed the API duplicate; keep the detector | declared |

`ui_lint` is a normal library until P5.9 decides the CBI lint-plugin shape.
`ui-shot` writes no fake image; its 78 exit is the only shipped runtime stub.
`ui-bench --update-baseline` requires `CHEMICAL_UI_UPDATE_BASELINE=1` and is a
parent-approved data change, not a normal test action.
