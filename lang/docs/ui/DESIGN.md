# Chemical UI — design system v1

**Status:** normative · **Date:** 2026-11-27
**Companions:** [`API.md`](API.md) §4/§8, [`CONTRACTS.md`](CONTRACTS.md) §4,
[`EXECUTION.md`](EXECUTION.md) P0.4/P1.5/P2.11.

P1.5/P1.6 use these exact values. P2.11 may polish shadows/easing but may not
invent a second token set. A token change after P1.5 requires a
`GOLDEN-CHURN.md` listing every updated golden.

---

## 1. Color tokens

### Light

| Token | Value | Use |
|-------|-------|-----|
| `Surface` (app background) | `#F8FAFC` | page |
| `SurfaceRaised` | `#FFFFFF` | cards, panels |
| `SurfaceSunken` | `#F1F5F9` | wells, table headers |
| `SurfaceOverlay` | `#FFFFFF` | popovers/dialogs (with elevation) |
| `OnSurface` | `#0F172A` | primary text |
| `OnSurfaceMuted` | `#475569` | secondary text |
| `OnSurfaceFaint` | `#64748B` | placeholders, captions |
| `Primary` | `#2563EB` | primary action |
| `PrimaryHover` | `#1D4ED8` | hover |
| `PrimaryPressed` | `#1E40AF` | pressed |
| `OnPrimary` | `#FFFFFF` | text on primary |
| `Secondary` | `#E2E8F0` | secondary action |
| `OnSecondary` | `#0F172A` | text on secondary |
| `Danger` | `#DC2626` | destructive |
| `OnDanger` | `#FFFFFF` | text on danger |
| `Success` | `#15803D` | success |
| `OnSuccess` | `#FFFFFF` | text on success |
| `Warning` | `#B45309` | warning |
| `OnWarning` | `#FFFFFF` | text on warning |
| `Border` | `#E2E8F0` | default border |
| `BorderStrong` | `#CBD5E1` | emphasized border |
| `BorderFocus` | `#2563EB` | focus ring |
| `FocusRing` | `#2563EB` | 2 px ring, 2 px offset |
| `Selection` | `#DBEAFE` | text selection |
| `Scrim` | `rgba(15,23,42,0.45)` | modal backdrop |
| `Chart1..6` | `#2563EB #0D9488 #D97706 #7C3AED #DB2777 #64748B` | data viz |

### Dark

| Token | Value |
|-------|-------|
| `Surface` | `#0B1220` |
| `SurfaceRaised` | `#111827` |
| `SurfaceSunken` | `#020617` |
| `SurfaceOverlay` | `#111827` |
| `OnSurface` | `#F8FAFC` |
| `OnSurfaceMuted` | `#CBD5E1` |
| `OnSurfaceFaint` | `#94A3B8` |
| `Primary` | `#60A5FA` |
| `PrimaryHover` | `#93C5FD` |
| `PrimaryPressed` | `#3B82F6` |
| `OnPrimary` | `#0B1220` |
| `Secondary` | `#1F2937` |
| `OnSecondary` | `#F8FAFC` |
| `Danger` | `#F87171` |
| `OnDanger` | `#0B1220` |
| `Success` | `#4ADE80` |
| `OnSuccess` | `#0B1220` |
| `Warning` | `#FBBF24` |
| `OnWarning` | `#0B1220` |
| `Border` | `#1F2937` |
| `BorderStrong` | `#334155` |
| `FocusRing` | `#93C5FD` |
| `Selection` | `#1E3A8A` |
| `Scrim` | `rgba(0,0,0,0.6)` |
| `Chart1..6` | `#60A5FA #2DD4BF #FBBF24 #A78BFA #F472B6 #94A3B8` |

### High contrast

| Token | Value |
|-------|-------|
| Surface | `#000000` |
| SurfaceRaised | `#000000` |
| SurfaceSunken | `#000000` |
| OnSurface | `#FFFFFF` |
| OnSurfaceMuted | `#FFFFFF` |
| Primary | `#FFD400` |
| OnPrimary | `#000000` |
| Border | `#FFFFFF` (2 px) |
| FocusRing | `#FFD400` (3 px) |
| Danger | `#FF6B6B` |
| Success | `#69F0AE` |
| Warning | `#FFD400` |

Color is never the only signal; icons/text accompany success/warning/danger.

Contrast gates: body ≥4.5:1; large text (≥24 px or ≥19 px bold) ≥3:1; focus
ring ≥3:1 against adjacent colors. Audit fails otherwise.

---

## 2. Spacing

| Token | Comfortable | Compact | Use |
|-------|------------:|--------:|-----|
| `None` | 0 | 0 | reset |
| `XXS` | 2 | 2 | hairline |
| `XS` | 4 | 4 | icon gap |
| `SM` | 8 | 6 | tight padding |
| `MD` | 12 | 10 | control gap |
| `LG` | 16 | 14 | card padding |
| `XL` | 24 | 20 | section gap |
| `XXL` | 32 | 28 | page gap |
| `XXXL` | 48 | 40 | hero gap |

8-pt grid; 2/4 pt only for optical alignment. Form rows use `MD`; card padding
`LG`; page padding `XL`; section gap `XXL`. Compact values (6/10/14/20/28/40) are
audit-allowed as the compact scale; do not mix comfortable and compact values on
one screen.

---

## 3. Radii and borders

| Token | Value | Use |
|-------|------:|-----|
| `None` | 0 | table cells, separators |
| `XS` | 2 | badges |
| `SM` | 4 | inputs, small buttons |
| `MD` | 6 | buttons, cards |
| `LG` | 8 | panels, dialogs |
| `XL` | 12 | sheets |
| `Full` | 999 | pills, avatars |

Border width: 1 px default; 2 px focus/selected; 0 when elevation separates.
Do not mix >2 radii on one screen.

---

## 4. Typography

Font: system UI stack by default; optional Inter 42 KB Latin subset.
Mono: platform mono (Cascadia/ SF Mono/ DejaVu Sans Mono/ ui-monospace).

| Role | Size / line | Weight | Use |
|------|------------:|-------:|-----|
| `Display` | 40 / 48 | 600 | marketing/empty state |
| `H1` | 32 / 40 | 700 | page title (one) |
| `H2` | 24 / 32 | 600 | section |
| `H3` | 20 / 28 | 600 | card title |
| `H4` | 16 / 24 | 600 | subsection |
| `Body` | 14 / 20 | 400 | default |
| `BodyStrong` | 14 / 20 | 600 | emphasis |
| `Label` | 13 / 18 | 500 | form labels |
| `Caption` | 12 / 16 | 400 | hints/meta |
| `Code` | 13 / 20 | 400 | code, mono |

- Max content width 1200 px; prose ≤65ch; table cells ≤75ch.
- Headings: exactly one H1; no skipped levels.
- Letter spacing: 0 for body; -0.01em for ≥24 px; 0.01em for captions.
- Underline links; never rely on color alone.
- 200% text must reflow, not clip.

---

## 5. Elevation and shadows

| Token | Value |
|-------|-------|
| `Low` | `0 1px 2px rgba(15,23,42,.08), 0 1px 1px rgba(15,23,42,.04)` |
| `Medium` | `0 4px 8px rgba(15,23,42,.08), 0 2px 4px rgba(15,23,42,.06)` |
| `High` | `0 12px 24px rgba(15,23,42,.14), 0 4px 8px rgba(15,23,42,.08)` |

Dark mode uses black shadows at 0.35/0.45/0.55 opacity. Elevation is not a
substitute for borders in dense tables. ≤3 elevations per screen.

---

## 6. Motion

| Token | Duration | Use |
|-------|---------:|-----|
| `Fast` | 120 ms | hover/press, small fades |
| `Base` | 160 ms | popover, tab, toggle |
| `Slow` | 240 ms | dialog/sheet, page |
| `Spring` | 220 ms | drag settle, selection |

Easings: `ease-out` = `cubic-bezier(0,0,0.2,1)`; `ease-in` =
`cubic-bezier(0.4,0,1,1)`; `ease-in-out` = `cubic-bezier(0.4,0,0.2,1)`.
Reduced motion collapses to end state immediately. Do not animate width/height
when text must re-shape; use transform/opacity.

---

## 7. Breakpoints and density

- `Sm` <640; `Md` 640–1023; `Lg` 1024–1439; `Xl` ≥1440.
- Comfortable control height: 32 px (small), 36 px (medium), 40 px (large);
  compact subtracts 4 px. Minimum touch target 24×24, preferred 44×44.
- Page padding: 16 (Sm), 24 (Md), 32 (Lg+).
- Sidebar: 240 px (Lg), 280 px (Xl); collapses under `Md`.
- Tables: row 36 px comfortable / 32 px compact; header 40/36.

---

## 8. Component state rules

| State | Visual |
|-------|--------|
| Default | token base |
| Hover | background `PrimaryHover` or surface +4% luminance; 120 ms |
| Focus-visible | 2 px `FocusRing` + 2 px offset; never removed |
| Pressed | `PrimaryPressed` / darker surface; 0.98 scale optional |
| Disabled | 40% opacity, no pointer events, no focus |
| Loading | keep layout size; spinner replaces icon; label stays |
| Error | `Danger` border + icon + text; never color-only |
| Selected | `Selection`/primary tint + check/indicator |
| Invalid | error text linked via `described_by`; focus first invalid |

Every interactive component ships all applicable states.

---

## 9. Aesthetic audit rules

`ui audit`/`ui_lint` enforce, per screen:

- spacing on the scale (2/4/8/12/16/24/32/48); no arbitrary 13/17/23 px;
- body contrast ≥4.5:1; large ≥3:1;
- targets ≥24×24, preferred 44×44;
- ≤75ch line length; content ≤1200 px;
- one H1; heading order valid;
- ≤3 elevations, ≤2 radii per screen;
- status not color-only;
- empty/loading/error/disabled states for every data view;
- responsive at 640/1024/1440; no horizontal overflow;
- motion 120–240 ms via `MotionToken`; spring parameters fixed in `Theme`;
  reduced-motion path;
- focus always visible, never clipped;
- density comfortable/compact tested at 100%/200%.

These are objective; subjective beauty is reviewed by the parent using the
screenshot gallery and the four example apps.
