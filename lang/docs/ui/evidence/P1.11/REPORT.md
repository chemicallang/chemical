# P1.11 — input, focus, hit testing: report

**Date:** 2026-11-28 · **Branch:** `feature/ui-v1` · **State:** green.
No commit; P1.12 not started.

## 1. Implemented

- `lang/libs/ui/src/display.ch`
  - `HitRecord` for interactive/scrollable nodes captured during display build;
    includes node/key/local + device bounds, cumulative transform, clip,
    z/order, tab index, focusable/disabled/scrollable and parent.
  - Focus and hover rings are emitted into the display list (2px focus ring,
    1px hover border), not only stored as state.
- `lang/libs/ui/src/input.ch`
  - `hit_test`, `hit_test_focusable`, `hit_focusable_by_key`,
    `focus_next`, `scroll_ancestor`, `scroll_by_hit`.
  - Exact inverse-transform point test with clip intersection and stable
    paint-order tie-break; stale/forged NodeIds are validated against layout.
- `lang/libs/ui/src/app.ch`
  - Public P1.11 event surface: `EventKind`, `KeyCode`, `Modifiers`, pointer/
    scroll/key/text/focus payloads, `UiEvent`, and `Payload` variants.
  - Deterministic normalized event queue and headless dispatch: pointer
    enter/leave/move/press/release, click/double-click/context, wheel, key
    down/up/text, focus/blur. `Application.update` receives events; `UiCtx`
    exposes focus, event fields and scroll helpers.
  - Click focuses the nearest focusable target; Tab/Shift-Tab skip disabled
    nodes and wrap; Escape clears focus. Pointer capture is held across
    press/release.
- `lang/libs/ui/src/view.ch` — `ViewBuilder.disabled` metadata; hit/focus
  skips disabled nodes (also honors semantics disabled state).

## 2. Results

| Check | Result |
|---|---|
| 10k-node hit test, LLVM release | **p50 412 µs / p95 430 µs** (gate ≤500 µs) |
| 10k scroll offset update, LLVM release | **p50 0 µs / p95 0 µs** |
| Event→update, LLVM release | **max observed 0.341 µs** (gate ≤1 ms) |
| TCC functional | hit 10k **2260 µs** p50; scroll **1 µs**; event max 0.521 µs |
| Click / double-click / Tab / Shift-Tab / Escape / wheel | PASS |
| Disabled focus skip, overlay topmost, transform hit, clip rejection, stale key | PASS |
| Focus/hover PNG exact fixture | PASS |

The 10k hit scene uses 10,000 focusable nodes; only interactive/scroll nodes
are recorded, so the display build does not pay a record cost for decorative
children. Event order is preserved by a FIFO queue; every queued event is
dispatched exactly once.

## 3. Focus/hover visual

`lang/tests/ui/fixtures/input/focus_hover.png` is 400×240, committed with
SHA-256:

```text
2c34734d6c6863f9f08018f73612a75cd4cdd887e3e18e19d02550525aaff189
```

I inspected it at full size. The focused white box has a distinct **blue 2px
focus ring**; the hovered red box has a subtle gray hover border; the disabled
green box has no focus ring. The clipped scroll surface is visible below the
row, and there are no stray rings on noninteractive/background nodes, no
fringes, and no seams. The focus and hover states are visually distinct and
match the expected state sheet.

## 4. Verification

- `scripts/ui-test.sh --tcc --filter input`: PASS
- `scripts/ui-test.sh --llvm --filter input`: PASS
- `scripts/ui-test.sh --llvm --sanitize=address,undefined --filter input`: PASS
- `scripts/ui-test.sh --llvm --release --filter input`: PASS
- `scripts/ui-test.sh --tcc --release --filter input`: PASS
- P1.6 layout, P1.7 layout_v2, view, style, theme, display, raster, headless:
  PASS TCC/LLVM as applicable.
- P1.8 display release: **p50 2539 µs / p95 6499 µs** (3/7 ms gate PASS).
- P1.9 raster release: solid **1012/1806 µs**, AA **4879/4970 µs**,
  clear+fill **322/655 µs**, damage **2/6 µs** (gates PASS).
- `ui-api-index --check`: 438 declarations, unchanged.
- `ui-verify`: self-test and P1.11 packet validation PASS.

## 5. Files

- `lang/libs/ui/src/input.ch` (new)
- `lang/libs/ui/src/app.ch` (event types/queue/dispatch; UiCtx focus/event/scroll)
- `lang/libs/ui/src/display.ch` (hit records, focus/hover rings)
- `lang/libs/ui/src/view.ch` (`disabled` metadata)
- `lang/tests/ui/src/input.ch`
- `lang/tests/ui/src/main.ch`, `chemical.mod`, `scripts/ui-test.sh`
- `lang/tests/ui/fixtures/input/{focus_hover.png,SHA256SUMS}`
- `lang/docs/ui/evidence/P1.11/REPORT.md` + `logs/`

## 6. Limitations

- No IME composition/commit, drag API, multi-window, semantics tree/audit,
  text editing, animation-owned focus, or modal focus trap. Those remain
  P3.5/P4/P7/P1.12.
- Wheel tuning is discrete; trackpad inertia/phase is carried in the payload
  but no inertial animation is owned by the runtime.
- Focus fallback on a removed node clears focus after the next display build;
  the full nearest-focusable-ancestor restore policy remains a P1.11 follow-up
  if parent requires it.
- `UiEvent` payloads are normalized at the boundary; the headless test queue
  is deterministic and test-facing, not a public host input source.
- No wall-clock sleeps; the vm uses the fixed headless clock.
