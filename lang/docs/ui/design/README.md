# P0.4 reference screens

These are static composition specs, not renderer output. They use only
[`tokens.json`](../tokens.json) values via inline CSS variables and the system UI
font stack; no external font or image is referenced.

| File | Content |
|---|---|
| `reference-settings.html/.png` | settings navigation, profile/workspace forms, appearance/notifications/security, danger zone |
| `reference-dashboard.html/.png` | KPI cards, release table, request chart, activity, status chips |
| `reference-chat.html/.png` | channel list, message stream, reactions, composer, members, command palette |
| `tokens.html/.png` | color swatches (light/dark/high-contrast), type ramp, spacing/density, radii, elevation, focus ring, motion |

`reference-dashboard-dark.png` is the query-parameter dark variant of the
dashboard (`?theme=dark`). The screens also accept `?theme=high-contrast` and
`?density=compact`.

Regenerate after editing `tokens.json`:

```bash
./scripts/ui-gen-design.sh
./scripts/ui-render-design.sh
./scripts/ui-gen-design.sh --check
```

Templates live in `templates/`; the committed HTML is generated. The PNGs are
1280×800 except `tokens.png` (1280×3245, full token sheet). The token-sheet
renderer uses Chrome at 1×; see `scripts/ui-render-design.sh`.
