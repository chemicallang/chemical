# P1.5 — style, tokens, themes: report

**Branch:** `feature/ui-v1` (no commit made by this subagent)
**State:** done. `P1.5b` was not started.

`tokens.json` remains the only token source. No token values changed, so no
golden files changed; `design/tokens.png` remains the reviewed P0.4 token sheet.

## Files

- `lang/libs/ui/src/theme.ch` — regenerated from `tokens.json` via
  `scripts/ui-gen-theme.sh`; full `TextStyle` fields (family name, italic,
  features, tab size), token tables, and internal breakpoint/density/focus
  helpers.
- `scripts/ui_gen_theme.py` — generator updated for the complete `Theme`; no
  hand edits to `theme.ch`.
- `lang/libs/ui/src/style.ch` — token-aware `padding_all` storage, concrete
  `ResolvedStyle`/`style_resolve`, state-style flags/sets and precedence
  resolver, token helper functions. Public `Style` signatures are unchanged.
- `lang/tests/ui/src/style.ch` — theme/setter/text-scale/feature tests, exact
  style merge groups, state precedence, compact resolution, default fast path,
  contrast checks, and bounded intern store.
- `lang/tests/ui/src/main.ch`, `scripts/ui-test.sh` — `style` and `theme`
  filters.
- `lang/docs/ui/evidence/P1.5/REPORT.md`, `logs/`.
- `STATUS.md`, `STATUS.json`, `packets/P1.5.json`.

## Theme completion

`Theme` is a value type with no global mutable theme state. `light()`, `dark()`
and `high_contrast()` read exact generated tables for:

- 32 `ColorToken`s per mode;
- comfortable and compact `Space` scales;
- `Radius`, `Elevation`, `Density`;
- all `TextRole` styles: size, line height, weight, letter spacing, family,
  family stack, italic, features, tab size;
- `MotionToken` durations and `disabled_opacity`;
- `set_color`, `set_space`, `set_radius`, `set_text_style`, `set_density`,
  `set_text_scale`.

`Theme.set_text_scale` scales role sizes and line heights; the multiplier is
preserved (`line_height` remains the ratio). Explicit `Style.text_style`
values are absolute and are not silently rescaled by the theme accessor.

Internal helpers cover breakpoints (`Sm <640`, `Md 640–1023`, `Lg 1024–1439`,
`Xl ≥1440`), comfortable/compact control heights, table row/header heights,
page padding, sidebar widths, focus width/offset, border width, and touch
targets (24/44).

## Style resolution semantics

`ui_internal::style_resolve(style, theme)` resolves token-backed values into a
concrete `ResolvedStyle`: colors, token padding, gap, radius, elevation/shadow,
border, opacity, text role/style/weight/color, box/flex/grid fields, inset,
direction, wrapping and tab size.

- Token-backed `padding_all(Space)` retains the token and resolves against the
  active density (comfortable `SM=8`, compact `SM=6`). Exact `padding(Insets)`
  remains exact.
- `gap` resolves through `Theme.space`, so compact mode changes it without
  rebuilding styles.
- Text resolution starts from `Theme.text(role)`, then applies an explicit
  `Style.text_style` if set, then `Style.weight` if set, then `Style.tab_size`
  if set. Explicit text styles keep their own size; theme role styles honor
  `Theme.text_scale`.
- Field-wise `merge` remains last-set-field-wins; unset overlay fields inherit
  the base. `padding_all` and exact `padding` are the same field group.
- State resolver layering is `hover → focus → pressed → selected → checked →
  invalid → disabled`; later states win on conflicting fields. Disabled also
  multiplies resolved opacity by `Theme.disabled_opacity()` (0.4). This order is
  covered by tests for hover, focus, pressed, invalid/checked, disabled, and
  all flags together.
- The default style is id `0`; merging an empty default style returns id `0`
  and hits the intern table.

### Color tokens (exact from tokens.json)

| Token | Light | Dark | High contrast |
|---|---|---|---|
| `Transparent` | `rgba(0,0,0,0)` | `rgba(0,0,0,0)` | `rgba(0,0,0,0)` |
| `Surface` | `#F8FAFC` | `#0B1220` | `#000000` |
| `SurfaceRaised` | `#FFFFFF` | `#111827` | `#000000` |
| `SurfaceSunken` | `#F1F5F9` | `#020617` | `#000000` |
| `SurfaceOverlay` | `#FFFFFF` | `#111827` | `#000000` |
| `OnSurface` | `#0F172A` | `#F8FAFC` | `#FFFFFF` |
| `OnSurfaceMuted` | `#475569` | `#CBD5E1` | `#FFFFFF` |
| `OnSurfaceFaint` | `#64748B` | `#94A3B8` | `#FFFFFF` |
| `Primary` | `#2563EB` | `#60A5FA` | `#FFD400` |
| `OnPrimary` | `#FFFFFF` | `#0B1220` | `#000000` |
| `PrimaryHover` | `#1D4ED8` | `#93C5FD` | `#FFD400` |
| `PrimaryPressed` | `#1E40AF` | `#3B82F6` | `#FFD400` |
| `Secondary` | `#E2E8F0` | `#1F2937` | `#000000` |
| `OnSecondary` | `#0F172A` | `#F8FAFC` | `#FFFFFF` |
| `Danger` | `#DC2626` | `#F87171` | `#FF6B6B` |
| `OnDanger` | `#FFFFFF` | `#0B1220` | `#000000` |
| `Success` | `#15803D` | `#4ADE80` | `#69F0AE` |
| `OnSuccess` | `#FFFFFF` | `#0B1220` | `#000000` |
| `Warning` | `#B45309` | `#FBBF24` | `#FFD400` |
| `OnWarning` | `#FFFFFF` | `#0B1220` | `#000000` |
| `Border` | `#E2E8F0` | `#1F2937` | `#FFFFFF` |
| `BorderStrong` | `#CBD5E1` | `#334155` | `#FFFFFF` |
| `BorderFocus` | `#2563EB` | `#93C5FD` | `#FFD400` |
| `FocusRing` | `#2563EB` | `#93C5FD` | `#FFD400` |
| `Selection` | `#DBEAFE` | `#1E3A8A` | `#FFD400` |
| `Scrim` | `rgba(15,23,42,0.45)` | `rgba(0,0,0,0.6)` | `rgba(0,0,0,0.85)` |
| `Chart1` | `#2563EB` | `#60A5FA` | `#FFD400` |
| `Chart2` | `#0D9488` | `#2DD4BF` | `#69F0AE` |
| `Chart3` | `#D97706` | `#FBBF24` | `#FF6B6B` |
| `Chart4` | `#7C3AED` | `#A78BFA` | `#FFFFFF` |
| `Chart5` | `#DB2777` | `#F472B6` | `#FFD400` |
| `Chart6` | `#64748B` | `#94A3B8` | `#69F0AE` |

### Spacing, radii, typography, motion, elevation

| Group | Exact value |
|---|---|
| Space.None (comfortable) | 0px |
| Space.XXS (comfortable) | 2px |
| Space.XS (comfortable) | 4px |
| Space.SM (comfortable) | 8px |
| Space.MD (comfortable) | 12px |
| Space.LG (comfortable) | 16px |
| Space.XL (comfortable) | 24px |
| Space.XXL (comfortable) | 32px |
| Space.XXXL (comfortable) | 48px |
| Space.None (compact) | 0px |
| Space.XXS (compact) | 2px |
| Space.XS (compact) | 4px |
| Space.SM (compact) | 6px |
| Space.MD (compact) | 10px |
| Space.LG (compact) | 14px |
| Space.XL (compact) | 20px |
| Space.XXL (compact) | 28px |
| Space.XXXL (compact) | 40px |
| Radius.None | 0px |
| Radius.XS | 2px |
| Radius.SM | 4px |
| Radius.MD | 6px |
| Radius.LG | 8px |
| Radius.XL | 12px |
| Radius.Full | 999px |
| TextRole.Display | 40px / 48px, weight 600, letter -0.01em, Ui |
| TextRole.H1 | 32px / 40px, weight 700, letter -0.01em, Ui |
| TextRole.H2 | 24px / 32px, weight 600, letter -0.01em, Ui |
| TextRole.H3 | 20px / 28px, weight 600, letter 0em, Ui |
| TextRole.H4 | 16px / 24px, weight 600, letter 0em, Ui |
| TextRole.Body | 14px / 20px, weight 400, letter 0em, Ui |
| TextRole.BodyStrong | 14px / 20px, weight 600, letter 0em, Ui |
| TextRole.Label | 13px / 18px, weight 500, letter 0em, Ui |
| TextRole.Caption | 12px / 16px, weight 400, letter 0.01em, Ui |
| TextRole.Code | 13px / 20px, weight 400, letter 0em, Mono |
| MotionToken.Fast | 120ms |
| MotionToken.Base | 160ms |
| MotionToken.Slow | 240ms |
| MotionToken.Spring | 220ms |
| disabled_opacity | 0.4 |

### Elevation first layer used by Theme (single Shadow)

| Mode | Low | Medium | High |
|---|---|---|---|
| light | 0/1/2/0 rgba(15,23,42,0.08) | 0/4/8/0 rgba(15,23,42,0.08) | 0/12/24/0 rgba(15,23,42,0.14) |
| dark | 0/1/2/0 rgba(0,0,0,0.35) | 0/4/8/0 rgba(0,0,0,0.45) | 0/12/24/0 rgba(0,0,0,0.55) |
| high-contrast | none | none | none |

### Focus and density helpers

| Helper | Exact values |
|---|---|
| `breakpoint_for_width` | 0–639 Sm, 640–1023 Md, 1024–1439 Lg, 1440+ Xl |
| `control_height(Comfortable, 0/1/2)` | 32 / 36 / 40 |
| `control_height(Compact, 0/1/2)` | 28 / 32 / 36 |
| `table_row_height` | 36 comfortable / 32 compact |
| `table_header_height` | 40 comfortable / 36 compact |
| `page_padding` | 16 Sm / 24 Md / 32 Lg / 32 Xl |
| `sidebar_width` | 240 Lg / 280 Xl |
| `focus_width` | 2 light / 2 dark / 3 high-contrast |
| `focus_offset` | 2 all modes |
| `border_width` | 1 light/dark / 2 high-contrast |
| touch targets | min 24 / preferred 44 |

## Contrast table

Computed with the P0.4 sRGB LUT and WCAG relative-luminance formula.

| Pair | Light | Dark | High contrast | Gate |
|---|---:|---:|---:|---:|
| `OnSurface / Surface` | 17.063 | 17.895 | 21.000 | ≥4.5 |
| `OnSurfaceMuted / Surface` | 7.243 | 12.611 | 21.000 | ≥4.5 |
| `OnSurfaceFaint / Surface` | 4.548 | 7.302 | 21.000 | ≥4.5 |
| `OnPrimary / Primary` | 5.169 | 7.364 | 14.669 | ≥4.5 |
| `OnDanger / Danger` | 4.829 | 6.769 | 7.567 | ≥4.5 |
| `OnSuccess / Success` | 5.016 | 10.745 | 14.676 | ≥4.5 |
| `OnWarning / Warning` | 5.022 | 11.216 | 14.669 | ≥4.5 |
| `FocusRing / Surface` | 4.940 | 10.383 | 14.669 | ≥3.0 |

`OnSurfaceFaint / Surface` in light mode is the tightest body-text gate at
4.548:1.

## Verification

| Command | Result |
|---|---|
| `ui-test.sh --tcc --filter style` | **PASS** |
| `ui-test.sh --llvm --filter style` | **PASS** |
| `ui-test.sh --llvm --sanitize=address,undefined --filter style` | **PASS** |
| `ui-test.sh --tcc --filter theme` | **PASS** |
| `ui-test.sh --llvm --filter theme` | **PASS** |
| `ui-test.sh --llvm --sanitize=address,undefined --filter theme` | **PASS** |
| `ui-test.sh --tcc/--llvm --filter design` | **PASS** |
| `ui-test.sh --tcc/--llvm --filter view` | **PASS** |
| `ui-test.sh --llvm --sanitize=address,undefined --filter view` | **PASS** |
| `ui-test.sh --tcc/--llvm --release --filter view` | **PASS** (amended P1.3 gate) |
| `ui-test.sh --tcc/--llvm --filter contract` | **PASS** |
| `ui-test.sh --tcc --filter keys/arena` | **PASS** |
| `ui-gen-theme.sh --check` | **PASS** |
| `ui-gen-design.sh --check` | **PASS** |
| `ui-api-index.sh --check` | **PASS** (438 declarations) |

## Deviations / residual limits

- No token edits were made; `tokens.json` remains authoritative and no golden
  churn was triggered.
- P0.4 did not specify `TextFeatures`; generated role styles use
  ligatures/tabular/slashed-zero false, stylistic set 0, empty raw tags, and
  tab size 4. Only one relevant token rule (Code tab size 4) existed.
- `Theme.elevation` still returns the API's single `Shadow`; tokens retain every
  CSS layer and the generated theme uses the first layer.
- High-contrast missing tokens and 3 px focus geometry follow the recorded
  `tokens.json` deviations.
- `Theme.set_space` updates both comfortable and compact scales because the API
  has one custom-space setter; `set_density` selects which scale is read.
- Raw color/size lint detection (`ui audit`) remains a later mechanism; this
  packet provides the token-only APIs and concrete resolution tables.
- No layout, paint, text shaping, or P1.5b font metrics work was added.
