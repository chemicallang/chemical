# P0.3a — scaffolding, scripts, manifests: report

**Branch:** `feature/ui-v1` (created from `main`; no commit made by this subagent)
**Scope:** EXECUTION.md §2 P0.3a. No UI API/components were implemented.
**Protected docs untouched:** `API.md`, `CONTRACTS.md`, `BENCHMARKS.md` were not edited.

---

## 1. Packages

All are valid `library` packages with one compile-only marker and no UI API.

| Package | `chemical.mod` imports | Marker |
|---|---|---|
| `ui` | `std` | `public const API_VERSION : u32 = 1`; `__ui_scaffold()` |
| `ui_kit`, `ui_host`, `ui_native`, `ui_web`, `ui_debug`, `ui_test`, `ui_cli`, `ui_gpu` | `std`, `ui` | `__ui_scaffold_<name>()` returns `API_VERSION` |
| `ui_icons`, `ui_fonts`, `ui_lint` | `std` | `__ui_scaffold_<name>()` returns `1u` |

Notes:

- Chemical does **not** namespace public top-level functions by module in this
  tree (verified: two packages with the same `__ui_scaffold` produced a duplicate
  symbol). Each package therefore uses a unique scaffold symbol; the requested
  `__ui_scaffold` exists in `ui` and as a prefix in every other package.
- `ui_icons`/`ui_fonts`/`ui_lint` intentionally do not import `ui`, matching the
  dependency table in `API.md` §0.
- `ui_lint` is a normal library now; P5.9 may convert it to the CBI lint-plugin
  shape.

---

## 2. Test and bench suites

- `lang/tests/ui/` — `application ui_tests`, imports every package, checks each
  scaffold marker, prints `ui scaffold PASS`, exit 0. Placeholders:
  `contracts/README.md`, `assets/README.md`, `fixtures/bad_packet.json`,
  `fixtures/good_packet.json`.
- `lang/tests/ui_bench/` — `application ui_bench`, `--scenario empty`,
  `--json`, deterministic counters; `scenes/empty.json`; no baseline.
- `lang/tests/ui/build` and `lang/tests/ui_bench/build` are covered by the
  existing `lang/tests/*/build` ignore rule.

---

## 3. Scripts and schemas

Scripts (all executable, `--help`, documented exit codes):

| Script | What P0.3a does |
|---|---|
| `scripts/ui-test.sh` | Builds/runs `lang/tests/ui` with TCC/LLVM; `--filter scaffold`, `--no-build`, `--sanitize` (LLVM-only; TCC exits 2), `--emit-c`, `--cc-sanitize`, `--update-goldens`, `--json`. |
| `scripts/ui-bench.sh` | Builds/runs `ui_bench`; `--scenario empty`, `--compare` (exit 3 without baseline), `--json`, `--tcc/--llvm`. Fails when a numeric counter is >2% over baseline. |
| `scripts/ui-shot.sh` | Validates `--headless/--theme/--scale/--out`; exits **78** and writes no image before P1.10. |
| `scripts/ui-verify.sh` | Validates packet manifests/artifacts/evidence/benchmark keys, optional hashes/commit, STATUS.json consistency; `--self-test`; `--write-status`; `--json`. |
| `scripts/ui-api-index.sh` | Extracts 352 public declarations from `API.md` Chemical fences into stable sorted JSON; identical repeat is recorded, conflicting duplicate key is fatal. |

Schemas/manifests:

- `lang/docs/ui/packets/schema.json` + `P0.1`, `P0.2`, `P0.3a`, `P0.3b`,
  `P0.4`, `P1.1`–`P1.14` (including `P1.5b`).
- `lang/docs/ui/evidence/schema.json`
- `lang/docs/ui/STATUS.schema.json`
- `lang/docs/ui/STATUS.json` generated from `STATUS.md`
- `lang/docs/ui/STUBS.md`
- `lang/docs/ui/ui_api_index.json`
- `.github/workflows/ui.yml` (contracts, Linux smoke, macOS/Windows BLOCKED
  smoke jobs, contract-snippet placeholder). Jobs are not deleted; the two
  runner-limited jobs use `continue-on-error` with comments and are recorded
  BLOCKED below.

API index detail: `API.md` currently repeats `UiCtx.set_text_scale` identically
at lines 961 and 999. The script records that under `identical_duplicates` and
fails on conflicting duplicates; P0.2 should resolve the source duplicate.

---

## 4. Acceptance commands and results

Raw logs: `lang/docs/ui/evidence/P0.3a/logs/`.

| Command | Result |
|---|---|
| `./scripts/ui-verify.sh --self-test` | **PASS** (bad packet red, good packet green) |
| `./scripts/ui-test.sh --tcc --filter scaffold` | **PASS**, `ui scaffold PASS` |
| `./scripts/ui-test.sh --llvm --filter scaffold` | **PASS**, `ui scaffold PASS` |
| `./scripts/ui-test.sh --tcc --filter scaffold --json` | **PASS**, valid JSON |
| `./scripts/ui-bench.sh --scenario empty --tcc` | **PASS** |
| `./scripts/ui-bench.sh --scenario empty --llvm` | **PASS** |
| `./scripts/ui-bench.sh --scenario empty --tcc --compare` (no baseline) | **exit 3**, refused as specified |
| `./scripts/ui-bench.sh --scenario empty --tcc --compare` (temp baseline) | **PASS** |
| `./scripts/ui-bench.sh --scenario empty --tcc --compare` (temp 0-frame baseline) | **exit 1**, regression caught |
| `./scripts/ui-api-index.sh` twice + `cmp` | **identical**, 352 declarations |
| `./scripts/ui-shot.sh --headless ...` | **exit 78**, no output file |
| `./scripts/ui-verify.sh --packet P0.1` | **PASS** |
| `./scripts/ui-verify.sh --packet P0.3a` | **PASS** |
| `./scripts/ui-verify.sh --packet P1.1` (todo, no evidence required) | **PASS** |
| `./scripts/build.sh --tcc -j 12` | **PASS**, new packages do not break the compiler build |
| Direct `TCCCompiler lang/tests/ui/chemical.mod ...` + run | **PASS**, `ui scaffold PASS` |
| `./scripts/ui-test.sh --tcc --emit-c --cc-sanitize` | **PASS** (C emitted then cc ASAN/UBSAN) |
| `./scripts/ui-test.sh --llvm --sanitize=address,undefined` | **exit 3, BLOCKED** (prebuilt LLVM has no compiler-rt) |

Self-test proof: `--self-test` runs the verifier on
`lang/tests/ui/fixtures/bad_packet.json` (invalid state + missing artifact and
evidence) and requires nonzero; it runs the known-good
`fixtures/good_packet.json` (state `review`, existing artifacts/evidence) and
requires zero. Both are in the log `logs/verify_self_test.log`.

---

## 5. Scaffolds and the packet that removes them

| Scaffold | Removed by |
|---|---|
| `__ui_scaffold*` markers, empty packages | P0.2 (contract declarations), then P1.1–P1.14 |
| `lang/tests/ui/contracts/` README only | P0.2 |
| `lang/tests/ui/assets/` README only | P1.5b / P1.9 / P1.10 |
| `ui-test.sh` only accepts `scaffold` | P0.3b |
| `ui-bench` only `empty`, no baseline | P0.3b |
| `ui-shot.sh` exit 78 | P1.10 |
| `ui-api-index` scaffold schema + identical duplicate | P0.2 |
| Contract-snippet CI job placeholder | P0.2 |

`STUBS.md` records each row. `ui_shot` is the only shipped runtime stub; it is
declared here and exits without writing an image.

---

## 6. BLOCKED / limitations

- **macOS CI runner:** BLOCKED for P0.1 (no host probe) and no Cocoa host exists.
  `.github/workflows/ui.yml` keeps `scaffold-macos` as a visible job with
  `continue-on-error: true` and a comment. Do not remove; P3.4/P6.5 make it hard.
- **Windows CI runner:** BLOCKED for P0.1 (no Win32 run); the job is visible with
  `continue-on-error: true` and a comment. P3.3/P6.5 make it hard.
- **TCC sanitizers:** `ui-test.sh --sanitize=...` rejects TCC (exit 2); the C path
  uses `--emit-c --cc-sanitize` instead. No TCC instrumentation is claimed.
- **LLVM ASAN/UBSAN:** BLOCKED on this host. The prebuilt LLVM under
  `out/host/lib/clang` has no `libclang_rt.asan*.a`; `ui-test.sh` detects this and
  exits 3 with a BLOCKED message instead of a linker failure. The C-emitted
  `--cc-sanitize` path passes (see `logs/test_emit_c_asan.log`). P6.3/P6.5 must
  provide compiler-rt or a system LLVM with it.
- **Evidence hashes/commit:** P0.3a has no commit yet; `STATUS.json` says
  `review` and the packet manifest declares no commit/hashes. The parent should
  run `scripts/ui-verify.sh --write-status` after updating `STATUS.md` at commit.
- **API.md duplicate:** the identical `UiCtx.set_text_scale` repeat is recorded,
  not edited (API.md is frozen until P0.2). P0.2 owns the fix.
- **No real benchmark:** `empty` emits deterministic scaffold counters, not
  wall-clock data. P0.3b adds scenes and baselines.
- **`ui-test.sh --json`** prints build/run output to stderr and one JSON object to
  stdout; logs are in the build directory.

---

## 7. Self-review

- Every package is a valid library; the test app imports all of them and runs on
  TCC and LLVM.
- No public UI API beyond `API_VERSION` and the scaffold markers.
- No skipped/disabled test: the scaffold test actually runs and is the acceptance
  target; the bench app actually runs; `--self-test` actually rejects the bad
  packet.
- No protected document was edited (`git status` shows only new UI scaffolding,
  scripts, schemas, evidence, and the workflow).
- No build artifacts are staged/tracked; `lang/tests/*/build` and
  `lang/tests/ui*/build` are ignored.
- `ui-shot` writes no fake image; `--cc-sanitize` was fixed to use raw `write`
  rather than stdio, because the scaffold test must be sanitizer-clean without
  changing the compiler.

---

## Parent audit note (ASAN)

The prebuilt LLVM under `out/host` ships `lib/clang/22` headers but no sanitizer
runtime archives. The host system clang 21 has them. For local verification we
symlinked `out/host/lib/clang/22/lib/linux/libclang_rt.*` to the system clang 21
runtime, after which `./scripts/ui-test.sh --llvm --sanitize=address,undefined`
passes. This is an environment setup, not a source change; CI must install a
matching runtime (P6.3). A fresh checkout without the symlink still reports
`BLOCKED` from `ui-test.sh`, which is the correct visible state.
