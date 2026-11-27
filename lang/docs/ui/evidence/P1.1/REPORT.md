# P1.1 — math, geometry, color, and errors: report

**Branch:** `feature/ui-v1` (no commit made by this subagent)
**Status:** all P1.1 commands green; P1.1 marked `done`; parent commit/tag pending.

No Style/Theme resolution, view, layout, or renderer code was added. The P0.4
token table remains generated; `Color` is now defined once, in `geometry.ch`.

---

## 1. Files

Added:

- `lang/libs/ui/src/geometry.ch` — `Point`, `Size`, `Rect`, `Insets`,
  `Transform2D`, `Color` and the P1.1 value methods.
- `lang/libs/ui/src/error.ch` — `ErrorClass`, `ErrorCode`, `RetryHint`,
  `SourceLocation`, `ErrorDetail`, `UiError`, `classify`, and `ok/err/map`.
- `lang/tests/ui/src/geometry.ch` — 10,000-case property tests per group.
- `lang/tests/ui/src/error.ch` — 27-code taxonomy and allocation-free detail tests.
- `lang/docs/ui/evidence/P1.1/REPORT.md` + `logs/`.

Modified:

- `scripts/ui_gen_theme.py` / `lang/libs/ui/src/theme.ch` — removed the generated
  `Color` definition; geometry owns it. `ui-gen-theme.sh --check` still passes.
- `lang/tests/ui/src/main.ch` — `geometry`/`error` filter dispatch.
- `scripts/ui-test.sh` — `--filter geometry` and `--filter error`.
- `lang/docs/ui/API.md` — additive P1.1 signatures and semantics.
- `lang/tests/ui/contracts/src/main.ch` — matching contract declarations.
- `lang/docs/ui/ui_api_index.json` — regenerated baseline (435 declarations,
  up from 367; the increase is method members plus `classify`/`ok`/`err`/`map`).
- `lang/docs/ui/STATUS.md`, `STATUS.json`, `packets/P1.1.json`.

## 2. Geometry/color semantics

- Logical coordinates are floats. `Rect` is half-open: `contains` includes
  `x/y` and excludes `x2/y2`; empty rects never intersect/contain.
- `Rect.new` clamps negative or non-finite extents to zero; `from_ltrb`
  normalizes corners. `intersection` returns `Option.None` for empty/disjoint
  rects; `union` returns the non-empty operand when one side is empty.
- `Transform2D` is Canvas2D order `[a,b,c,d,tx,ty]`; `multiply` returns
  `self ∘ other` (other first). Rotation uses range-reduced Taylor sin/cos so
  the TCC link needs no libm; the 10k composition test uses a 0.02 float tolerance.
- `Point.length`/`distance` use an in-tree Newton sqrt (no libm). Non-finite
  inputs/operations clamp to zero/identity where layout/paint would consume them.
- `Color.from_u32`/`to_u32` use `0xRRGGBBAA` (red high byte). Premultiply,
  unpremultiply, and `over` are integer sRGB-u8 operations using the contract’s
  `div255`. `over` returns exact `dst` for a transparent source and exact `src`
  for an opaque source. Luminance/contrast use a 256-entry sRGB LUT and f64; no
  libm. Low-alpha premultiply round-trip has the unavoidable `255/a + 1` channel
  tolerance.
- `Size.new` clamps negative/non-finite dimensions to zero; `Size.sub` clamps.
  `Insets` is finite-checked. `all`, `symmetric`, `horizontal`, `vertical` have
  the usual edge-wise meanings.

## 3. Error semantics

`UiError.make` is allocation-free: enum/numeric fields, `ErrorDetail` with
`&'static str` message/context, optional source, retry hint. It never creates
`Fatal`; `classify` maps app codes as follows:

- **Expected:** all codes except the internal set below (including `Io`,
  `Timeout`, `OutOfMemory`, `DeviceLost`, `Surface`, `Protocol`).
- **Internal:** `ThreadAffinity`, `StaleNode`, `Internal`, `Other`.
- **Fatal:** only the private `UiError.make_fatal` helper (not public; no P1.1
  call site).
- `ErrorClass.AppBug` remains reserved/unused in P1.1.
- Default retry: `Backoff` for `Io`, `Timeout`, `Busy`, `Cancelled`,
  `WouldBlock`, `DeviceLost`; `Never` otherwise. `with_retry` overrides.
- `detail()` returns the inline `&ErrorDetail`; `source()` returns
  `Option<SourceLocation>`. The source file is a borrowed view and must be
  static/host-owned if debug tooling retains it.
- `Result` helpers `ok`, `err`, `map` are the minimal additions; no
  `unwrap`/`expect`/`panic` convenience.

## 4. Property-test results

`lang/tests/ui/src/geometry.ch` uses a deterministic xorshift64 seed and asserts
numeric invariants over **10,000 cases per group**:

- rectangle contains/intersection/union/area relationships, half-open edges,
  and empty-rect behavior;
- transform composition (`A∘B` vs apply B then A), identity/rotation;
- color u32 round-trip, premultiply/unpremultiply tolerance, source-over
  transparent/opaque endpoints, lerp endpoints, black/white luminance/contrast;
- NaN/Inf/negative clamps for Point/Size/Rect and `from_ltrb` normalization.

`lang/tests/ui/src/error.ch` checks all **27 ErrorCode values**, class/retry
mapping, `Fatal` non-constructibility through the app API, static detail
retention after a local scope, source/retry copying, and `ok`/`err`/`map`.
It also asserts compact `UiError`/`ErrorDetail` sizes; no allocation path exists.

## 5. Commands / results

Raw logs: `lang/docs/ui/evidence/P1.1/logs/`.

| Command | Result |
|---|---|
| `./scripts/ui-gen-theme.sh --check` | **PASS** |
| `./scripts/ui-gen-design.sh --check` | **PASS** |
| `./scripts/ui-test.sh --tcc --filter geometry` | **PASS** (`ui geometry PASS`) |
| `./scripts/ui-test.sh --llvm --filter geometry` | **PASS** |
| `./scripts/ui-test.sh --tcc --filter error` | **PASS** (`ui error PASS`) |
| `./scripts/ui-test.sh --llvm --filter error` | **PASS** |
| `./scripts/ui-test.sh --llvm --sanitize=address,undefined --filter geometry` | **PASS** |
| `./scripts/ui-test.sh --llvm --sanitize=address,undefined --filter error` | **PASS** |
| `./scripts/ui-api-index.sh --check` | **PASS** (435 declarations; 0 duplicates) |
| `./scripts/ui-test.sh --tcc/--llvm --filter contract` | **PASS** |
| `./scripts/ui-test.sh --tcc/--llvm --filter design` | **PASS** (Color move regression) |
| `./scripts/ui-verify.sh --self-test` | **PASS** |
| `./scripts/ui-verify.sh --packet P1.1` | **PASS** |

The ASAN runtime is present under `out/host/lib/clang/22/lib/linux` and both
instrumented filters report no errors.

## 6. API additions (additive P1.1 amendment)

- Point: `zero`, `new`, `add`, `sub`, `scale`, `length`, `distance`, `lerp`,
  `is_finite`.
- Size: `zero`, `new`, `add`, `sub`, `max`, `min`, `area`, `is_finite`.
- Rect: `zero`, `new`, `from_ltrb`, `x2`, `y2`, `contains`, `intersects`,
  `intersection`, `union`, `inset`, `offset`, `area`, `is_finite`.
- Insets: `zero`, `all`, `symmetric`, `horizontal`, `vertical`, `is_finite`.
- Transform2D: `identity`, `translate`, `scale`, `rotate`, `multiply`,
  `transform_point`, `is_finite`.
- Color: `new`, `rgba`, `from_u32`, `to_u32`, `with_alpha`, `premultiply`,
  `unpremultiply`, `lerp`, `over`, `relative_luminance`, `contrast_ratio`,
  `is_opaque`, `to_premul_u32`.
- UiError: `retry_hint`, `detail` (`&ErrorDetail`), `source`, `with_source`,
  `with_retry`; free `classify`; `ok`, `err`, `map`.

No existing signature was changed or removed. The contract module mirrors every
addition; the API index baseline was intentionally regenerated.

## 7. Residual limitations / blockers

- Low-alpha premultiply/unpremultiply cannot reconstruct every channel exactly;
  tests assert the mathematically unavoidable `255/a + 1` bound. Premultiplied
  blending itself is exact integer `div255`.
- `Transform2D.rotate` is a Taylor approximation (no libm) rather than a
  platform `sin/cos`; accuracy is ample for the layout/paint tolerance but is not
  a bit-exact libm guarantee.
- `UiError` source file views are borrowed; only static/host-owned paths may be
  retained by debug tooling. The inline detail message/context are `&'static`.
- `Fatal` has no public constructor; the private runtime helper exists but no
  P1.1 path calls it. `ErrorClass.AppBug` is reserved for a later app-misuse path.
- macOS/Windows CI remain BLOCKED as previously recorded. No renderer/layout
  behavior is claimed in this packet.
