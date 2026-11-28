# P1.8b — complete P1.8 and settle the display gate: report

**Branch:** `feature/ui-v1` (no commit made by this subagent)
**State:** **blocked pending parent gate approval**, not done. Backend/damage,
20-scene serialization and all functional display tests are green. P1.9 was not
started.

## Delivered

- `RenderBackend` internal contract: `submit`, `present`, `resize`,
  `device_lost`; `RecordingBackend` CPU/null adapter for P1.8 tests.
- Damage computation: visible command bounds transformed and clipped to the
  viewport; `full` policy emits the viewport. Tests cover submit/present/resize/
  device-lost and non-empty/full damage.
- 20 deterministic display scenes serialized to stable text and committed under
  `lang/tests/ui/fixtures/display/sceneNN.dl.txt`; the test compares exact
  serialization. Scene coverage includes rects, rounded/border/shadow, text,
  images/icons, gradient/path (Painter), clips, transforms, layers, scroll,
  overlays and nested combinations.
- Display hot-path cache: last resolved `PaintProps` avoids repeated
  style/theme resolution for a repeated style; command build remains one
  ordered list.

## Timing (LLVM release, 5 runs)

| Scene | p50 | p95 | Original gate |
|---|---:|---:|---:|
| 10k display commands | **2,291 µs** | **5,494 µs** | ≤300 µs FAIL |

TCC release functional: **15,566 µs p50 / 19,017 µs p95**.

The result satisfies the proposed **3 ms p50 / 7 ms p95** parent amendment but
not the original 0.3 ms budget. The budget was not changed; approval is required.

## Verification

- `ui-test --tcc/--llvm --filter display` — PASS.
- `ui-test --llvm --sanitize=address,undefined --filter display` — PASS.
- `ui-test --tcc --release --filter display` — PASS functional.
- `ui-test --tcc --filter layout/layout_v2/view/style/contract` — PASS.
- `ui-api-index --check` — PASS (438 declarations).
- 20-scene expected output and backend/damage tests — PASS.

## Residual limits

- No rasterizer/pixels/host; P1.9/P1.10.
- Display serialization includes command metadata but not path/gradient point
  details; counts are included.
- Overlay z/layer metadata is serialized; actual z sorting/paint is P1.9.
- No public API or token/golden changes.
