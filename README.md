# Chemical Compiler Analytics

Historical performance, testing, and release analytics for the Chemical compiler,
served as a static GitHub Pages dashboard.

This README and the dashboard site live **on the `gh-pages` branch** of this
repository. Nothing benchmark-related is stored on `main` — the collection
scripts live in `scripts/` (`bench-*.sh`), and every measurement is written to
the `gh-pages` branch only.

```
gh-pages/                 # the branch this file lives on
  index.html              # dashboard (pure static JS)
  assets/                 # css/js/Chart.js
  README.md               # this file
  data/manifest.json      # index of available data files
  data/daily/<date>.json  # one commit-benchmark point per date
  data/releases/<tag>/info.json
  data/releases/<tag>/<platform>-<arch>.json

main/
  scripts/bench-*.sh      # collection + publishing scripts (bash)
.github/workflows/
  benchmark-daily.yml     # daily commit benchmark (schedule + manual)
  benchmark-release.yml   # release benchmark (manual + on release publish)
  benchmark-backfill.yml  # historical backfill (manual, chunkable)
```

## How it works

Every measurement is a **JSON data point with an explicit status**. Failures are
recorded as data (`build_failure`, `test_failure`, `benchmark_failure`,
`timeout`, `missing_asset`, `unavailable`) — collection never aborts because one
platform/backend/release/commit failed.

The dataset is plain JSON files on the **`gh-pages` branch**. GitHub Pages serves
the branch root; the dashboard fetches `data/*.json` at runtime. There is no
database or server.

### Data layout (on gh-pages)

```
data/manifest.json                     # index of available daily + release files
data/daily/2026-08-11.json             # one commit-benchmark point per date
data/releases/<tag>/info.json          # release metadata + per-asset sizes/status
data/releases/<tag>/<platform>-<arch>.json   # benchmarks + tests for one platform
```

### Backends

All three execution backends are first-class:

| Backend | Meaning | Test command |
|---------|---------|--------------|
| `TCCCompiler` | TinyCC-based compiler | `./scripts/test.sh --tcc` |
| `Compiler` | LLVM-based compiler | `./scripts/test.sh --llvm` |
| `Interpreter` | direct AST interpreter | `./scripts/test.sh --interpret` |

A backend failure never invalidates the others — each record has its own
`build` / `benchmarks` / `modules` / `tests` status.

### The same-commit rule

A compiler built at commit X **can only compile the test sources of commit X**
(tests evolve with the compiler). Every collector therefore runs inside a
checkout/worktree at the exact commit being measured, and release benchmarks use
`git archive <tag> lang/tests`. This is the same rule enforced in
`compile-tests.yml`.

### GitHub Actions runner time limits

Standard runners cap a job at 6 hours. A `TCCCompiler` build takes ~15 minutes
and the LLVM `Compiler` ~20 minutes (prebuilt LLVM is downloaded, never built).
The backfill workflow therefore:

* splits the requested range across **parallel chunk jobs** (`chunks` input),
* each chunk runs with **`--time-budget-min`** (default 300 min) and stops
  before the runner kills it,
* collected points are **skipped on re-run**, so re-dispatching with the same
  inputs resumes where the previous run stopped.

The daily workflow builds one point per run — a single TCC build + test run fits
comfortably inside the limit, and `--skip-llvm` is on by default.

## Collection scripts (`scripts/bench-*.sh`)

All scripts live in `scripts/` and share `bench-common.sh`:

| Script | What it does |
|--------|--------------|
| `bench-collect-daily.sh` | Build the compiler at a commit, run hello-world + module benchmarks, run the test suites for each backend. Emits `data/daily/<date>.json`. |
| `bench-collect-release.sh` | Query the GitHub API for a release's assets (sizes, missing markers), download the platform assets, merge the release's own test sources, benchmark + test with the release binaries. Emits `data/releases/<tag>/...`. |
| `bench-backfill.sh` | Historical backfill: iterates past days (default daily mode) or commits, collecting each with its own test sources. Skips already-collected points (resumable). Supports `--offset-days` + `--time-budget-min` for chunked parallel runs. |
| `bench-push-pages.sh` | Rebuilds `data/manifest.json` and pushes **only `data/`** to the `gh-pages` branch (site files on the branch are preserved). Refuses to push to `main`/`master`/the default branch. |

### Local usage

```bash
# one daily point for today, TCC backend only, skip tests (fast)
bash scripts/bench-collect-daily.sh --skip-llvm --quick

# full daily point (builds + hello + module benchmarks + test suites)
bash scripts/bench-collect-daily.sh --skip-llvm

# release info for a tag (no downloads)
bash scripts/bench-collect-release.sh --info-only --tag v0.5.7

# full release benchmark for the current platform
bash scripts/bench-collect-release.sh --tag v0.5.7 --skip-llvm

# backfill 7 days + all releases, publishing locally only
bash scripts/bench-backfill.sh --days 7 --out /tmp/benchdata
bash scripts/bench-push-pages.sh --data /tmp/benchdata --no-push

# chunked backfill (parallel-friendly): days 1..15 in one call
bash scripts/bench-backfill.sh --days 15 --offset-days 0 --time-budget-min 300
```

## Workflows

### `benchmark-daily.yml`
Runs daily at 03:00 UTC (and manually). Checks out the default branch for the
scripts, then benchmarks the target commit (default HEAD) in a worktree,
downloads libtcc (`scripts/setup.sh`), builds, benchmarks, tests, and pushes to
`gh-pages`. Inputs: `commit`, `skip_llvm` (default true — LLVM is heavy), `quick`.

### `benchmark-release.yml`
Runs manually (`release_tag` input) or automatically on release publish.
Five platform jobs run in parallel: linux (glibc), **alpine (alpine:3.20
container — the musl binaries only run there)**, macos arm64, macos x64,
windows. Each downloads the release assets, merges the release's own test
sources, and benchmarks. A final publish job merges the artifacts and pushes to
`gh-pages`.

### `benchmark-backfill.yml`
Manual dispatch. Populates historical data — `days` of daily points (default
30), optionally all releases. `chunks` splits the range across parallel jobs,
each bounded by `time_budget_min` so no job hits the 6h runner limit.
Re-dispatch with the same inputs to resume.

## Dashboard

Static HTML/CSS/JS with a vendored Chart.js. Views:

- **Overview** — summary stats, latest daily build, latest release assets
- **Releases** — binary size per release/platform, hello-world compile time,
  test results, the full release table with missing assets marked `MISSING`
- **Daily** — hello-world time, test duration, tests failed over dates;
  per-backend checkbox filters; latest-vs-previous ▲/▼ regression chips
- **Modules** — per-module compilation time over dates (from `-bm-modules`)
- **Backends** — TCC vs LLVM vs Interpreter comparison tables and charts

Never hides failures: missing assets, build failures, and failed tests are all
rendered with explicit statuses.

## Design notes

- **Fault tolerance**: every collection step wraps failures into data points
  with status + reason. Nothing aborts the run.
- **Deterministic scripts**: pure bash + jq; ANSI/CR stripping via a single sed
  pass (per-line subprocesses were pathologically slow on multi-thousand-line
  logs).
- **Simple persistent data**: JSON files are the source of truth; the HTML is
  just a consumer.
- **Incremental**: existing dates/tags are skipped, so re-runs resume.
- **Environment awareness**: every record includes platform, arch, libc
  (musl/glibc), commit/release, compiler version, and timestamps. The dashboard
  does not imply cross-environment measurements are equivalent.
- **Never touches main**: `bench-push-pages.sh` refuses to publish to
  `main`/`master`/the default branch. All benchmark data lives on `gh-pages`.

## Status vocabulary

| Status | Meaning |
|--------|---------|
| `success` | measurement completed |
| `test_failure` | tests ran but some failed (still real data) |
| `build_failure` | the compiler/test suite failed to build |
| `benchmark_failure` | a benchmark could not be completed |
| `timeout` | exceeded the time budget |
| `missing_asset` | the release has no such binary for the platform |
| `unavailable` | not measurable in this environment (e.g. interpreter hello) |
