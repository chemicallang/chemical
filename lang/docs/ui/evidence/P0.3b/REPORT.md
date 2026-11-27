# P0.3b — test core, bench baselines, minimal lint: report

**Branch:** `feature/ui-v1` (no commit made by this subagent)
**Status:** P0.3b commands are green; P0.3b is marked `done`; parent commit/tag pending.
No production renderer/layout/font stack and no public API change. `BENCHMARKS.md`
was not modified.

---

## 1. Test core (`lang/tests/ui/src/core.ch`)

Test-only `ui_test_core` (P6.1 replaces it with the production `UiHarness`):

- stable generational `NodeId`, FNV `Key` lookup (`key`, `by_key`);
- parent/child (`add_node`, `child_at`), bounds, text, semantics, test id;
- reorder with keyed-state preservation; stale `NodeId` returns `None`;
- `find_all` query by role/text/test-id;
- focus and keyed scroll state;
- `frame`, `advance`, `settle(max_frames)` with `CoreError.Timeout` on a
  non-settling tree;
- `geometry_snapshot()` / `semantics_snapshot()` deterministic JSON strings;
- `assert_golden(actual, path, opts)` via `image` (pixel diff, `.expected.png`,
  `.actual.png`, `.diff.png`); `CHEMICAL_UI_UPDATE_GOLDENS=1` writes only when
  explicitly set and prints `updated golden`.

`ui-test.sh` now dispatches `scaffold|core|golden|*` and passes `--filter` to the
test executable. `--no-build` uses the existing executable; `--filter contract`
builds/runs the frozen contract module; `--contract-check-only` remains for the
front-end-only gate.

Tests fail with named diagnostics, not silent skips or panics.

---

## 2. Deterministic pixel fixture and golden proof

Committed: `lang/tests/ui/fixtures/pixels/checker_128.png` (128×128 RGBA
checker/gradient, written through `image`).

- PNG SHA-256: `424a8b518c53efad05c0ad964382f5dbc395298b2da19c3963b6208fa2d01000`
- Decoded RGBA SHA-256: `2deb38260d75f07d80d888a6ba74b6ea2dd88eb4a09e2ff7bdeb39b86ca2aa38`
- 65,737 bytes.

Golden tests:

1. decode + re-encode the fixture, compare PNG SHA-256;
2. decode and compare the RGBA SHA-256;
3. pass compare against the committed fixture;
4. deliberately corrupt a build-dir copy by one pixel, require `GoldenMismatch`
   with `diff_pixels > 0`, and assert `.expected.png`, `.actual.png`,
   `.diff.png` exist.

Artifact proof: `logs/golden_diff_artifacts.txt` lists all four files; the
committed fixture is never modified by the failure test.

`CHEMICAL_UI_UPDATE_GOLDENS=1` update path writes `build/golden_update.png`, prints
`updated golden`, and the test asserts the `updated` flag.

---

## 3. Bench harness and scenes

Scenes: `lang/tests/ui_bench/scenes/{empty,static-1k,list-100k}.json` with the
`BENCHMARKS.md` §4 field set and a 64-hex `scene_hash`. Schema:
`scenes/schema.json` (unknown fields and unknown scenario fail).

The scaffold runner (**not** a renderer) emits only deterministic counters,
`renderer:"none"`, `notes:"scaffold counters; no real timing"`. No frame time is
claimed.

Baseline: `lang/tests/ui_bench/baselines/reference-linux-x86_64.json` with one
entry per scene (`scene_hash`, counters). Generated in a stable TCC run:

| Scene | nodes | commands | allocs | frames |
|---|---:|---:|---:|---:|
| empty | 0 | 0 | 0 | 1 |
| static-1k | 1,000 | 1,000 | 0 | 61 |
| list-100k | 100,000 | 100,000 | 0 | 2,000 |

`ui-bench.sh`:
- validates scene against `scenes/schema.json`;
- compares counters against the baseline, failing >2% and on scene-hash change;
- refuses `--compare` without a baseline (exit 3);
- `--update-baseline` requires `CHEMICAL_UI_UPDATE_BASELINE=1` and is documented
  as a parent-approved data change in `STUBS.md`/`ui_bench/README.md`.

---

## 4. Minimal lint

`scripts/ui-lint.sh`:

| Rule | Scope | Behaviour |
|---|---|---|
| `UI-KEY-001` | UI test/scaffold and `lang/compiled/ui_*` | list/table loop emitting a visible node without `b.key`/`key_of`/`key_pair` |
| `UI-PANIC-001` | `lang/libs/ui*`, `lang/compiled/ui_*` | `panic`/`unwrap`/`expect`/`unreachable`, excluding contract/test/probe modules |
| `UI-RAW-001` | `lang/compiled/ui_*` | `ui::raw`/raw-pointer use without an `unsafe` host-boundary comment |

Fixtures:
`lang/tests/ui/fixtures/lint/clean/` (pass) and `bad_key/`, `bad_panic/`,
`bad_raw/` (each fails its rule).

---

## 5. Commands and results

Raw logs: `lang/docs/ui/evidence/P0.3b/logs/`.

| Command | Result |
|---|---|
| `ui-test.sh --tcc --filter core` | **PASS** (`ui core PASS`) |
| `ui-test.sh --llvm --filter core` | **PASS** |
| `ui-test.sh --tcc --filter golden` | **PASS** (`ui golden PASS`) |
| `ui-test.sh --llvm --filter golden` | **PASS** |
| `ui-test.sh --tcc --filter scaffold` | **PASS** |
| `ui-bench.sh --scenario empty --tcc/--llvm --compare` | **PASS** |
| `ui-bench.sh --scenario static-1k --tcc/--llvm --compare` | **PASS** |
| `ui-bench.sh --scenario list-100k --tcc/--llvm --compare` | **PASS** |
| forced baseline `nodes=900` vs candidate 1000 | **exit 1**, regression caught |
| unknown scenario | **exit 2** |
| scene with an unknown field | **exit 2** |
| `ui-lint.sh --fixtures` | **PASS** (clean pass, all three bad fail) |
| `ui-lint.sh` repo scan | **PASS** (0 findings) |
| `ui-api-index.sh --check` | **PASS** |
| `ui-verify.sh --self-test` | **PASS** |
| `ui-verify.sh --packet P0.3b` | **PASS** |

---

## 6. Files

Added:

- `lang/tests/ui/src/core.ch`
- `lang/tests/ui/fixtures/pixels/checker_128.png` + README
- `lang/tests/ui/fixtures/lint/{clean,bad_key,bad_panic,bad_raw}/view.ch`
- `lang/tests/ui/README.md`
- `lang/tests/ui_bench/scenes/{empty,static-1k,list-100k}.json`, `scenes/schema.json`
- `lang/tests/ui_bench/baselines/reference-linux-x86_64.json`
- `lang/tests/ui_bench/README.md`
- `scripts/ui-lint.sh`
- `lang/docs/ui/evidence/P0.3b/REPORT.md` + `logs/`

Modified:

- `lang/tests/ui/src/main.ch` — filter dispatcher + core/golden tests
- `lang/tests/ui/chemical.mod` — image/crypto/fs/path/environment imports
- `lang/tests/ui_bench/src/main.ch` — three scenes, scene hash, scaffold counters
- `scripts/ui-test.sh` — core/golden dispatch, contract build/run
- `scripts/ui-bench.sh` — schema validation, profile baseline, `--update-baseline`
- `lang/docs/ui/STUBS.md`, `STATUS.md`, `STATUS.json`, `packets/P0.3b.json`

No public API/ABI changes. No commit.

---

## 7. Known limitations / stubs

- `ui_test_core` is a **test-only** model; it does not render, lay out, shape
  text, or run the production frame loop. P6.1 replaces it.
- Golden fixture proves the **image path**, not rasterizer quality; first real
  renderer goldens are P1.9/P1.10.
- Bench counters are scaffold-only and `renderer:none`; real timing starts at
  P1.10. The baseline is a deterministic counter baseline, not a perf claim.
- Lint implements only `UI-KEY-001`, `UI-PANIC-001`, `UI-RAW-001`; P5.9 expands
  the rule set. `ui_lint` remains a normal library.
- `ui-shot` still exits 78, no image, until P1.10.
- macOS/Windows CI remain BLOCKED as previously recorded.

All of the above is recorded in `STUBS.md` with the removing packet.
