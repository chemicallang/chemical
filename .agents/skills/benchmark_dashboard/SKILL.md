---
name: Benchmark Dashboard
description: The Chemical compiler benchmark & release analytics system — how the static GitHub Pages dashboard works, the JSON data model (manifest/daily/release records), the bash collection scripts (bench-*.sh), the GitHub Actions workflows (benchmark-daily/release/backfill), and the critical pitfalls (hidden upload dirs, worktree submodules, gh-pages-only data). Load when working on the dashboard, benchmark collection, or analytics workflows.
---

# Benchmark & Release Analytics Dashboard — Skill

This skill teaches you everything needed to work on the **Chemical compiler
benchmark/analytics system**: the static GitHub Pages dashboard at
`https://chemicallang.github.io/chemical/`, the bash collection scripts that
produce its data, and the GitHub Actions workflows that run them.

The dashboard tracks, historically, for every **release** and every **daily
commit**: binary sizes per platform, hello-world compile times (bare vs std),
test-suite results (passed/failed/failed-test names), and per-module
compilation times — across three backends (`TCCCompiler`, `Compiler` = LLVM,
`Interpreter`).

---

## 1. Mental model (read this first)

```
main branch                              gh-pages branch (deployed site)
  scripts/bench-*.sh   ──collect──▶  data/manifest.json
  .github/workflows/                     data/daily/<date>.json
    benchmark-daily.yml                  data/releases/<tag>/info.json
    benchmark-release.yml                data/releases/<tag>/<platform>-<arch>.json
    benchmark-backfill.yml               index.html, assets/dashboard.js,
                                         assets/styles.css,
                                         assets/vendor/chart.umd.min.js
```

**THE most important rule: benchmark data and the dashboard site NEVER live on
`main`.** The collection scripts live on `main` (under `scripts/`), but every
measurement and the entire site live on the **`gh-pages` branch**.
`scripts/bench-push-pages.sh` **refuses** to push to `main`/`master`/the
default branch (it exits 3). Never commit benchmark JSON to `main`. Locally,
data is staged in a gitignored `.bench-data/` directory.

- Collection = bash + jq (deterministic, fault-tolerant).
- Data = plain JSON files on `gh-pages`. No database, no server.
- Dashboard = static HTML + JS (`assets/dashboard.js`) + vendored Chart.js
  (`assets/vendor/chart.umd.min.js`). Chart.js must be referenced as
  `assets/vendor/chart.umd.min.js` — NOT `assets/chart.umd.min.js` (that
  wrong path caused a 404 + `Chart is not defined`; fixed already, don't
  reintroduce it).
- Every failure is recorded as a **data point with a status** — collection
  never aborts because one platform/backend/release/commit failed.

---

## 2. The data model (JSON schemas)

### `data/manifest.json` — the index
```json
{
  "generated_at": "2026-08-11T11:35:41Z",
  "daily": ["2026-08-11", "2026-08-10", "..."],
  "releases": { "v0.5.7": ["linux-x64", "linux-alpine-x64", "macos-arm64",
                            "macos-x64", "windows-x64"], "v0.0.1": ["linux-x64"] }
}
```
`releases` maps each tag → the list of `<platform>-<arch>` record files that
exist for it. **This is the source of truth for which platform records exist**
— the dashboard derives its platform selector from this (NOT from record
objects, because those are lazily loaded).

### `data/daily/<date>.json` — one build+test point per day
```json
{
  "type": "daily", "date": "2026-08-11", "generated_at": "...",
  "platform": { "os": "linux", "arch": "x64", "libc": "glibc" },
  "commit": { "sha": "...", "short": "69355469bf12", "subject": "fix workflows",
              "author_date": "..." },
  "compiler_version": { "TCCCompiler": "Chemical v0.5.7...", "Compiler": "..." },
  "backends": {
    "TCCCompiler": { "build": { "status": "success|failed|unavailable",
                                "reason": "..." },
                     "benchmarks": [ { "name": "hello_bare",
                                       "status": "success", "duration_ms": 123,
                                       "reason": null },
                                     { "name": "hello_std", ... } ],
                     "modules": [ { "tag": "bm:module", "name": "main",
                                    "nanos": 0, "micros": 0, "millis": 12,
                                    "secs": 0 } ],
                     "files": [ { "tag": "Lexer", "name": "some.ch", ... } ],
                     "tests": { "status": "success|test_failure",
                                "total": 500, "passed": 498, "failed": 2,
                                "duration_ms": 42000,
                                "failed_tests": ["name of failed test"] },
                     "tests_build_ms": 12345,
                     "tests_build_status": "success" },
    "Compiler": { ... }, "Interpreter": { ... }
  }
}
```
- `backends` is keyed by **canonical names** `TCCCompiler`, `Compiler`,
  `Interpreter` (lowercase `tcc`/`llvm`/`interpret` appear only in
  script-internal file names and are renamed).
- `modules` holds only `tag == "bm:module"` entries (from `-bm-modules`);
  `files` holds per-file phase entries (Lexer/Parser/SymRes:*/2cTranslation:*,
  from `-bm-files`), capped to the 50 slowest per phase.

### `data/releases/<tag>/info.json` — release metadata + FULL asset inventory
```json
{
  "type": "release_info", "tag": "v0.5.7",
  "published_at": "...", "commit_sha": "...", "name": "...",
  "generated_at": "...", "status": "success",
  "assets": {
    "linux-x64.zip":     { "size_bytes": 77287717,
                           "url": "https://github.com/...",
                           "status": "success" },
    "linux-arm64.zip":   { "status": "missing_asset",
                           "size_bytes": null, "url": null },
    ...
  }
}
```
- Assets come from the **actual GitHub API asset list** (every real asset),
  then era-expected-but-missing assets are added with `status: "missing_asset"`.
- ⚠️ Old releases (v0.0.1…) used different names (`linux-x86-64.zip` not
  `linux-x64.zip`) — that's why the dashboard shows `—` for columns that
  didn't exist yet. Do not "fix" old data by guessing; fetch the API.

### `data/releases/<tag>/<platform>-<arch>.json` — per-platform benchmark+test
```json
{
  "type": "release_platform", "tag": "v0.5.7", "platform": "linux",
  "arch": "x64", "libc": "glibc", "generated_at": "...",
  "status": "success", "reason": null,
  "assets": { ...same full inventory as info.json... },
  "backends": { "TCCCompiler": { same shape as daily backends }, ... }
}
```

### Status vocabulary (used everywhere — never invent new ones without reason)
| status | meaning |
|---|---|
| `success` | measurement completed |
| `test_failure` | tests ran but some failed (still real data) |
| `build_failure` | compiler/test suite failed to build |
| `benchmark_failure` | benchmark couldn't complete |
| `timeout` | exceeded the time budget |
| `missing_asset` | release has no such binary for the platform |
| `unavailable` | not measurable in this environment (e.g. Interpreter hello world) |
| `skipped` | explicitly skipped (e.g. `--quick` mode) |

---

## 3. Collection scripts (`scripts/bench-*.sh` on `main`)

All share `scripts/bench-common.sh` (source it; it defines `DATA_ROOT`,
helpers, and pure-bash versions of GNU utilities for macOS/BSD compat — e.g.
`ver_ge` replaces `sort -V`, `bm_now_ms` replaces `date +%s%N`).

| Script | Purpose |
|---|---|
| `bench-collect-daily.sh` | Builds the compiler at a commit, benchmarks hello (bare/std), runs `-bm-modules -bm-files`, runs test suites per backend → `data/daily/<date>.json`. Flags: `--commit`, `--work`, `--skip-llvm`, `--backends`, `--quick`, `--out`. |
| `bench-collect-release.sh` | Fetches release + assets from GitHub API, downloads the platform's zips, merges the **release's own test sources** (`git archive <tag> lang/tests`), benchmarks + tests with the release binaries → `data/releases/<tag>/...`. Flags: `--tag` (required), `--platform`, `--arch`, `--info-only`, `--quick`, `--skip-llvm`, `--out`. |
| `bench-backfill.sh` | Historical backfill: iterates past days (or commits) and all releases, skipping already-collected points (resumable). Flags: `--days`, `--mode daily|commits`, `--offset-days`, `--time-budget-min`, `--limit`, `--releases-only/--no-releases`, `--skip-llvm/--no-skip-llvm`, `--quick`. |
| `bench-push-pages.sh` | Rebuilds `data/manifest.json` from the merged data tree and pushes **only `data/`** to `gh-pages`. Site files on the branch are preserved. Flags: `--data`, `--site`, `--branch`, `--no-push`. |
| `backfill-release-assets.sh` | One-time repair: rewrites every release's `info.json` with the complete API asset inventory. |

### Key helpers in `bench-common.sh`
- `bm_run <out_status> <out_duration_ms> <timeout_secs> <log> <cmd...>` — runs
  with timeout + logs; status = success|timeout|failed.
- `bench_hello <backend> <bin> <bare|std> <name> <outvar> <logdir>` — measures
  compilation time of a hello-world; Interpreter → `unavailable`.
- `bench_modules_and_tests <backend> <bin> <log_base> <outvar> <logdir> <quick>`
  — compiles `lang/tests/build.lab` with `-bm-modules -bm-files`, then runs the
  test executable (compiled) or `--arg-interpret` (interpreter).
- `parse_test_output <log>` — parses `Test N [name] succeeded/failed` lines and
  the `Total N Passed N Failed N` summary → counts + `failed_tests[]`.
- `parse_bm_output <log>` — parses `[bm:module] 'name' completed [nano:N]...`
  lines → JSON array (tag/name/nanos/micros/millis/secs).
- `expected_assets <tag>` — era-based expected zip name list (used only for
  `missing_asset` markers; actual assets always come from the API).
- `asset_parts <name>` — `linux-x64.zip` → `linux x64 regular`; `-tcc` suffix →
  `tcc` variant.
- `bm_link_shared_tooling <worktree> <main_root>` — links `lib/` and `out/host`
  from the main checkout into a worktree (see Pitfalls).
- `bm_write_json <file> <json>` — atomic write (tmp + mv).

### ⚠️ Pitfalls (learned the hard way — respect these)

1. **Worktrees don't materialize submodules.** `git worktree add` creates an
   **empty `lib/lsp-framework/` stub** (submodule not checked out), so
   `cmake -S . -B cmake-build-debug` fails with "does not contain a
   CMakeLists.txt" and **every build records `failed`**. `bm_link_shared_tooling`
   fixes this by `rm -rf`'ing the worktree's `lib/` and symlinking the main
   checkout's `lib/` (and `out/host` for LLVM). If you add build logic to a
   worktree path, always call it.
2. **Hidden directories are dropped by `upload-artifact@v4`.** The staging dir
   `.bench-data` starts with a dot; without `include-hidden-files: true` on
   every upload, the artifact contains NOTHING ("No files were found") and the
   site stays empty while the workflow "succeeds". Both `benchmark-backfill.yml`
   and `benchmark-release.yml` uploads must keep `include-hidden-files: true`.
3. **upload-artifact strips the search-path prefix.** Uploading
   `path: .bench-data/` produces an artifact whose root IS `.bench-data/`'s
   contents (`daily/`, `releases/` at root — NOT `.bench-data/daily`). The
   publish jobs copy `merged/daily` + `merged/releases` accordingly. If you
   change the upload path, you MUST update the publish copy paths to match.
4. **The same-commit rule.** A compiler built at commit X can only compile the
   test sources of commit X. Daily/backfill build inside a checkout/worktree at
   the target commit; release benchmarks use `git archive <tag> lang/tests`.
5. **`bench-push-pages.sh` is an OVERLAY, never a wipe.** It copies the local
   data on top of what's already published and rebuilds the manifest from the
   merged tree. An empty publish is a harmless no-op. Never replace this with
   `rm -rf` (that wiped the site's history once).
6. **`--quick` skips test suites and module benchmarks.** Release records with
   `build.status: "skipped"` / empty modules mean the run used quick mode.
   Full runs are the default now (`quick: false` in the workflow inputs).
7. **The backfill release loop is `--info-only`** because a single Linux runner
   cannot execute macOS/Windows/Alpine binaries. Per-platform release tests
   come from `benchmark-release.yml` (dispatched per tag on native runners).
8. **macOS/BSD compat:** never use GNU-only tools (`sort -V`, `date +%s%N`) in
   these scripts — use `ver_ge`/`bm_now_ms` from `bench-common.sh`.
9. **Line endings:** `.github/workflows/*.yml` files use CRLF. Keep CRLF
   (`grep -c $'\r' file` should be non-zero) when editing them, or GitHub may
   mis-parse. Use `python3 -c "import yaml; yaml.safe_load(...)"` to validate.
10. **GitHub Actions skips `needs`-dependent jobs on failure.** If a deploy job
    must run even when a platform build failed, its `if:` needs `always()`
    (e.g. the `deploy-debug` job publishes surviving platform builds to
    `chemical-debug` even when alpine-arm64 fails).
11. **`env` context is invalid in job-level `if:`.** Only `github.*` and
    `inputs.*` are allowed there. Step-level `if:` may use `env`.
12. **build-and-release's alpine job** runs in an `alpine:3.20` container and
    clones the repo with plain git (JS actions like checkout can't run in
    musl) into `tagrepo` + `tooling` subdirs; scripts are run from
    `tooling/scripts`. Alpine currently FAILS to build on arm64 — a known
    problem; one platform failing must not block publishing the others.

---

## 4. Workflows (`.github/workflows/`)

| Workflow | Trigger | What it does |
|---|---|---|
| `benchmark-daily.yml` | schedule 03:00 UTC + manual | Builds the compiler at HEAD **in the main checkout** (NOT a worktree), runs `bench-collect-daily.sh` (all 3 backends by default), pushes to gh-pages. Inputs: `commit`, `skip_llvm` (default **false** — LLVM included), `quick`. |
| `benchmark-release.yml` | manual (`release_tag`) + on release published | 5 parallel platform jobs (linux, **alpine** musl, macos-arm64, macos-x64, windows) download the release's assets, merge the tag's test sources, run full benchmarks + tests, publish → gh-pages. |
| `benchmark-backfill.yml` | manual | Historical backfill: `days` (default 30), `chunks` parallel jobs, `time_budget_min` (300) to stay under the 6h runner cap. Re-dispatch to resume (collected points skipped). |
| `build-and-release.yml` | manual + `releaseIt` commits | Builds release binaries for all platforms; `config` input: `Release` → `chemical` repo, `RelWithDebInfo`/`Debug` → **`chemical-debug` repo** with `-debug` suffix zips. `deploy-debug` job aggregates debug zips; it has `if: always() && ...` so one failed platform doesn't skip the whole debug publish. |

---

## 5. The dashboard (`gh-pages` branch — assets/dashboard.js)

Pure static JS (~750 lines). Views: Overview, Releases, Daily/Commits, Modules,
Backends. Key facts:

- **Lazy loading:** `init()` fetches `manifest.json`, all daily records, and
  all `info.json` files in parallel. Per-platform record files
  (`data/releases/<tag>/<platform>.json`) are fetched ONLY when needed via
  `ensurePlatformRecords(rec)` (Releases charts + release modal).
  `loadDailyRecord(date)` / `loadPlatformRecord(tag, pf)` cache promises.
  Do NOT revert to eager-loading all platform records (that was the "loads too
  slowly" complaint).
- **Platform list source:** the Releases `<select>` options come from
  `releasePlatformsForChart()` which reads `state.manifest.releases` (the
  manifest) — NOT `state.releases[].platforms` (those are empty until lazily
  loaded; using them broke the dropdown once).
- **Size units:** binary sizes render in **MB** (`fmtSize`; charts use the `MB`
  helper `b/1048576` and `barOpts("MB")`). The "All releases" table uses the
  `.rel-table` CSS class (fixed column alignment + `MISSING` labels with
  tooltips).
- Helpers: `backendOf(dailyRec, backend)`, `helloOf(rec, name)` (finds a
  benchmark by name), `assetParts(name)` (zip name → platform/arch/variant),
  `statusBadge(status)` (explicit status rendering), `regressChip()` (▲/▼ vs
  previous point).
- Charts: `makeChart(id, config)` wraps `new Chart(...)`; Chart.js comes from
  `assets/vendor/chart.umd.min.js` (a 404 there = `Chart is not defined`).
- Colors: `COLORS` map for the 3 backends; `colorFor(s)` cycles for platforms.

### Editing the dashboard (site files are NOT on main)
1. Fetch the current file: `git fetch origin gh-pages` then
   `git show origin/gh-pages:assets/dashboard.js > /tmp/dashboard.js`
   (same for `styles.css`, `index.html`).
2. Edit, validate with `node --check` / `python3` as appropriate.
3. Publish via a temp worktree (the push-pages script only pushes data, not
   site files):
   ```bash
   WT=$(mktemp -d)
   git worktree add --detach "$WT" origin/gh-pages
   cd "$WT" && git checkout -q -B my-update
   cp /tmp/dashboard.js assets/dashboard.js
   git add assets/dashboard.js && git commit -q -m "Dashboard: ..."
   cd /home/.../chemical && git push origin my-update:gh-pages
   git worktree remove --force "$WT" && git branch -D my-update
   ```
   ⚠️ This does NOT create a stash, and it leaves `main` untouched. Never
   `git stash` before such operations, and never `git stash pop` afterward —
   the stash stack belongs to the user's own work.

---

## 6. Local workflow (test before shipping)

```bash
# collect a single daily point (fast, TCC only, no tests)
bash scripts/bench-collect-daily.sh --skip-llvm --quick --out /tmp/benchdata

# release info only (no downloads)
bash scripts/bench-collect-release.sh --info-only --tag v0.5.7 --out /tmp/benchdata

# inspect the produced record
python3 -m json.tool /tmp/benchdata/daily/$(date -u +%Y-%m-%d).json

# publish locally without pushing (validates manifest build)
bash scripts/bench-push-pages.sh --data /tmp/benchdata --no-push

# validation commands
bash -n scripts/bench-*.sh
python3 -c "import yaml; yaml.safe_load(open('.github/workflows/benchmark-daily.yml'))"
node --check assets/dashboard.js
```
To verify the live site: use a browser agent against
`https://chemicallang.github.io/chemical/` and check console errors + each tab.

---

## 7. Suggested prompts for feature work (4–5 lines)

- **New chart:** "Add a chart to the dashboard's Overview showing test pass rate
  over the last 30 daily records, one line per backend, in `renderOverviewCharts()`.
  It reads `state.daily[].backends[<backend>].tests.passed/total`. Follow the
  existing `makeChart`/`baseLineOpts` patterns and validate with `node --check`."
- **New column:** "Add a 'macOS ARM (TCC)' column to the All releases table in
  `renderReleases()` using `assetNameFor('macos','arm64','tcc')`, keeping the
  `.rel-table` column alignment, then update the manifest/backfill if the
  expected-asset list needs it."
- **New data source:** "Collect <X> in `bench-collect-daily.sh` and store it on
  the daily record under a new key, then surface it in the Daily view. Run
  `bash scripts/bench-collect-daily.sh --skip-llvm --quick` locally to verify
  the record shape before wiring the UI."
- **Fix a workflow:** "In `benchmark-<x>.yml`, when the <y> job fails, the <z>
  publish job is skipped. Add `always()` to <z>'s `if:` (keep the existing
  dispatch guard), and make the artifact download fail loudly with
  `if-no-files-found: error` if nothing exists."
- **Full pipeline sanity:** "Verify the benchmark pipeline end to end: dispatch
  the daily workflow, then check https://chemicallang.github.io/chemical/data/
  for today's record and confirm the dashboard renders it (Overview + Daily +
  Backends tabs), reporting any console errors."
