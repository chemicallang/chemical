# P0.4 — design spec and exact tokens: report

**Branch:** `feature/ui-v1` (no commit made by this subagent)
**Date:** 2026-11-27
**Status:** all P0.4 commands green; P0.4 marked `done`; parent commit/tag pending.

The production renderer/layout/font stack was **not** implemented. `API.md`
public signatures and `BENCHMARKS.md` budgets were not changed. No external
font, image, or proprietary asset is used by the references.

---

## 1. Token source of truth

`lang/docs/ui/tokens.json` is the single source. It contains:

- all 32 `ColorToken`s for `light`, `dark`, and `high-contrast`;
- comfortable and compact `Space` scales;
- all 7 `Radius` values;
- the 10-role type ramp (size, line, weight, family, letter spacing);
- elevation layers with resolved colors (dark uses black shadows; high contrast
  is border-separated with no shadows);
- `Fast`/`Base`/`Slow`/`Spring` durations plus the three easing curves;
- breakpoints Sm/Md/Lg/Xl, density/control/table/sidebar metrics, page padding;
- focus ring width/offset and border width for each mode;
- `disabled_opacity = 0.4` and the contrast/layout guard values;
- explicit `deviations` for values DESIGN.md does not name.

`lang/libs/ui/src/theme.ch` is generated from `tokens.json`; it implements the
API.md-frozen `ColorToken`, `Space`, `Radius`, `Elevation`, `TextRole`,
`TextStyle`, `Shadow`, `Duration`, `MotionToken`, `ThemeMode`, `Density`,
`FontFamily`, `FontWeight`, and `Theme` accessors/setters. No additional public
API was introduced.

`./scripts/ui-gen-theme.sh --check` regenerates the table and compares it with the
checked-in `theme.ch`. A stale table fails. `./scripts/ui-gen-design.sh --check`
does the same for the reference HTML.

## 2. Exact contrast table

Generated in `contrast.tsv` / `contrast-table.md`; the test recomputes WCAG
relative luminance from the actual `Theme` colors and fails below the gates.

| Mode | Pair | Ratio | Gate |
|---|---|---:|---|
| light | OnSurface/Surface | 17.063:1 | ≥4.5:1 |
| light | OnSurfaceMuted/Surface | 7.243:1 | ≥4.5:1 |
| light | OnSurfaceFaint/Surface | 4.548:1 | ≥4.5:1 |
| light | OnPrimary/Primary | 5.169:1 | ≥4.5:1 |
| light | OnDanger/Danger | 4.829:1 | ≥4.5:1 |
| light | OnSuccess/Success | 5.016:1 | ≥4.5:1 |
| light | OnWarning/Warning | 5.022:1 | ≥4.5:1 |
| light | FocusRing/Surface | 4.940:1 | ≥3.0:1 |
| dark | OnSurface/Surface | 17.895:1 | ≥4.5:1 |
| dark | OnSurfaceMuted/Surface | 12.611:1 | ≥4.5:1 |
| dark | OnSurfaceFaint/Surface | 7.302:1 | ≥4.5:1 |
| dark | OnPrimary/Primary | 7.364:1 | ≥4.5:1 |
| dark | OnDanger/Danger | 6.769:1 | ≥4.5:1 |
| dark | OnSuccess/Success | 10.745:1 | ≥4.5:1 |
| dark | OnWarning/Warning | 11.216:1 | ≥4.5:1 |
| dark | FocusRing/Surface | 10.383:1 | ≥3.0:1 |
| high-contrast | OnSurface/Surface | 21.000:1 | ≥4.5:1 |
| high-contrast | OnSurfaceMuted/Surface | 21.000:1 | ≥4.5:1 |
| high-contrast | OnSurfaceFaint/Surface | 21.000:1 | ≥4.5:1 |
| high-contrast | OnPrimary/Primary | 14.669:1 | ≥4.5:1 |
| high-contrast | OnDanger/Danger | 7.567:1 | ≥4.5:1 |
| high-contrast | OnSuccess/Success | 14.676:1 | ≥4.5:1 |
| high-contrast | OnWarning/Warning | 14.669:1 | ≥4.5:1 |
| high-contrast | FocusRing/Surface | 14.669:1 | ≥3.0:1 |

The design test also checks all 32 exact color values per mode, exact
comfortable/compact spacing, all radii, every type size/line/weight/letter and
Ui/Mono family, the four motion durations, elevation geometry/alpha, and the
disabled opacity. `design_negative_mutation.log` proves a one-byte token change
is detected (`light got=249… want=248…`).

## 3. Reference screens and visual review

Rendered at 1× with headless Chrome 157 (`scripts/ui-render-design.sh`).

- [`reference-settings.png`](../../design/reference-settings.png) — 1280×800.
  Visual review: 240 px sidebar with workspace and account groups, active
  “Profile & settings” row, search with `⌘K`; saved-state banner; profile and
  workspace forms; appearance/theme swatches, density selector, reduced-motion
  switch; notification switches; security rows; danger zone. Everything fits in
  the 800 px viewport; no clipped cards or horizontal overflow; surfaces,
  borders, primary/danger actions, and focus/selection use tokens.
- [`reference-dashboard.png`](../../design/reference-dashboard.png) — 1280×800.
  Visual review: four KPI cards with distinct sparklines and success/danger
  trends; segmented service filter; six-row release table with token status
  badges; two-series SVG request chart with grid lines and legend; two-item
  live activity panel. Table, chart, and activity fit without clipping; no
  placeholder-only region.
- [`reference-chat.png`](../../design/reference-chat.png) — 1280×800.
  Visual review: channel list with unread marker, DM presence, channel header,
  five-message stream with avatars/reactions/code block, member pane with pinned
  review, composer with attach/emoji/send, and an open command palette
  (`Jump to or run a command…`) with shortcuts. The palette is an intentional
  overlay; the composer remains visible and the page does not scroll.
- [`reference-dashboard-dark.png`](../../design/reference-dashboard-dark.png) — dark query variant
  (`?theme=dark`), confirming the token-table swap and black shadow variants.

The screens accept `?theme=dark|high-contrast` and `?density=compact`; the
light 1× screenshots are the committed reference.

Token sheet ([`tokens.png`](../../design/tokens.png), 1280×3245): all 32 colors in
three mode columns, full type ramp, comfortable/compact spacing bars, control/
table/breakpoint table, radii, elevation cards, focus-ring samples including a
high-contrast black/yellow sample, disabled 40% button, and motion bars. Values
are generated from `tokens.json`; no arbitrary color literal appears in the
templates (checked with a raw `#hex`/`rgba` grep).

## 4. GOLDEN-CHURN

[`GOLDEN-CHURN.md`](../../GOLDEN-CHURN.md) makes `tokens.json` the only source,
requires `ui-gen-theme.sh` + `ui-gen-design.sh` + re-render + contrast tests for
any token edit, and requires a report listing every reference/renderer golden
touched. P2.11 may polish shadows/easing only; no second token set.

## 5. Commands / results

Raw logs: `lang/docs/ui/evidence/P0.4/logs/`.

| Command | Result |
|---|---|
| `./scripts/ui-gen-theme.sh --check` | **PASS** (`theme.ch matches tokens.json`) |
| `./scripts/ui-gen-design.sh --check` | **PASS** (references match tokens.json) |
| `./scripts/ui-test.sh --tcc --filter design` | **PASS** (`ui design PASS`) |
| `./scripts/ui-test.sh --llvm --filter design` | **PASS** |
| mutated light `Surface` byte, then design test | **exit 1**, exact mismatch reported |
| `./scripts/ui-api-index.sh --check` | **PASS** (367 declarations, 0 duplicates) |
| `./scripts/ui-verify.sh --self-test` | **PASS** |
| `./scripts/ui-verify.sh --packet P0.4` | **PASS** |
| `./scripts/ui-render-design.sh` | **PASS** (5 PNGs) |

`ui-test.sh` now accepts `--filter design`; the executable dispatches it to
`design::run()`.

## 6. Files

Added:

- `lang/docs/ui/tokens.json`
- `lang/libs/ui/src/theme.ch` (generated)
- `scripts/ui_gen_theme.py`, `scripts/ui-gen-theme.sh`
- `lang/tests/ui/src/design.ch`
- `lang/docs/ui/design/{reference-settings,reference-dashboard,reference-chat,tokens}.html`
- `lang/docs/ui/design/{reference-settings,reference-dashboard,reference-chat,tokens}.png`
- `lang/docs/ui/design/reference-dashboard-dark.png`
- `lang/docs/ui/design/templates/{settings,dashboard,chat}.html`
- `lang/docs/ui/design/README.md`
- `scripts/ui_gen_design.py`, `scripts/ui-gen-design.sh`, `scripts/ui-render-design.sh`
- `lang/docs/ui/GOLDEN-CHURN.md`
- `lang/docs/ui/evidence/P0.4/REPORT.md`, `contrast.tsv`, `contrast-table.md`, `logs/`

Modified:

- `lang/tests/ui/src/main.ch` — design filter dispatch
- `scripts/ui-test.sh` — `design` filter
- `lang/docs/ui/STUBS.md`, `STATUS.md`, `STATUS.json`, `packets/P0.4.json`

No public API signature change; no production renderer/layout/font code; no
`BENCHMARKS.md` change.

## 7. Deviations and known limitations

Recorded in `tokens.json.deviations`:

- DESIGN.md’s high-contrast table is partial. Missing colors map to the nearest
  high-contrast palette values (surfaces #000000, muted/faint #FFFFFF, secondary
  #000000/on-secondary #FFFFFF, border-strong #FFFFFF, primary hover/pressed
  #FFD400, selection #FFD400, scrim rgba(0,0,0,0.85); on-danger/success/warning
  #000000). High-contrast charts reuse the named palette and require
  patterns/labels; color is never the only signal.
- High-contrast focus ring is 3 px with a 2 px offset; border width is 2 px.
- High-contrast elevation is border-separated with no shadows.
- `Theme.elevation` returns the API.md single `Shadow`; `tokens.json` retains all
  CSS layers. The generated Theme uses the first layer.
- DESIGN.md has no numeric spring stiffness/damping/mass. `Spring` is 220 ms
  with the ease-in-out curve and `parameters: null`; P1.5 must not invent a
  second spring token set.
- DESIGN.md does not specify `TextFeatures`; generated styles use
  ligatures/tabular/slashed-zero false, stylistic set 0, Code tab size 4.
- Dark `BorderFocus` uses the dark `FocusRing` value because DESIGN.md names the
  ring but not a separate dark focus border.

`theme.ch` provides token accessors/setters only; style resolution, paint use,
theme switching, and runtime density/focus consumption remain P1.5. The static
reference screens are composition specs, not renderer goldens (first renderer
goldens P1.9/P1.10). macOS/Windows CI remain BLOCKED as previously recorded.
