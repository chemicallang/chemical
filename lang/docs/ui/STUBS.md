# Chemical UI — declared stubs

No runtime stub ships without a row here. A row names the packet that removes it.

| Stub | Location | Behaviour | Removed by | Status |
|------|----------|-----------|------------|--------|
| `ui-shot.sh` | `scripts/ui-shot.sh` | Validates arguments, exits **78** (`EX_CONFIG`), writes **no** image. | P1.10 | declared |
| Scaffold markers | `lang/libs/ui*/src/*.ch` | `__ui_scaffold*()` returns the API version; no UI API. | P0.2 (contracts), then P1.1–P1.14 | declared |
| `ui-test.sh` filter | `scripts/ui-test.sh` | Only `scaffold` exists; other filters exit 2. | P0.3b | declared |
| `ui-api-index` exact duplicate | `scripts/ui-api-index.sh` | Records identical duplicates (`UiCtx.set_text_scale` in API.md); conflicting duplicates are fatal. | P0.2 | declared |
| Bench baseline | `lang/tests/ui_bench/baselines/` | P0.3a has no baseline; `--compare` refuses with exit 3. | P0.3b | declared |
| Contract snippets | `lang/tests/ui/contracts/` | README only. | P0.2 | declared |
| Test assets | `lang/tests/ui/assets/` | README only. | P1.5b / P1.9 / P1.10 | declared |

`ui_lint` is a normal library until P5.9 decides the CBI lint-plugin shape.
