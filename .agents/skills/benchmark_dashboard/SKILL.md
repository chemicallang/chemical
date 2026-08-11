---
name: Benchmark Dashboard
description: The Chemical compiler benchmark & release analytics system — how the static GitHub Pages dashboard works, the JSON data model (manifest/daily/release records), the bash collection scripts (bench-*.sh), the GitHub Actions workflows (benchmark-daily/release), and the critical pitfalls (hidden upload dirs, worktree submodules, gh-pages-only data). Load when working on the dashboard, benchmark collection, or analytics workflows.
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
`Interpreter`). The site is a shadcn-style static dashboard (light/dark
themes) with six views — Overview, Releases, Daily / Commits, **Failures**
(the core view: aggregates every failing/crashed/timed-out test),
Modules, Backends — plus filters, sortable tables, CSV export, and a
compare-two-releases tool.

---

## 1. Mental model (read this first)

```
main branch                              gh-pages branch (deployed site)
  scripts/bench-*.sh   ──collect──▶  data/manifest.json
  .github/workflows/                     data/daily/<date>.json
    benchmark-daily.yml                  data/releases/<tag>/info.json
    benchmark-release.yml                data/releases/<tag>/<platform>-<arch>.json
                                         index.html, assets/dashboard.js (~1550
                                         lines, 6 views incl. Failures),
                                         assets/styles.css (shadcn tokens,
                                         light/dark themes),
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
                     "tests": { "status": "success|test_failure|test_crash|timeout|build_failure",
                                "reason": "...",
                                "total": 2589, "passed": 2583, "failed": 6,
                                "succeeded": 2583, "duration_ms": 42000,
                                "complete": true,
                                "sequential": { "total": 2066, "passed": 2066, "failed": 0 },
                                "runner": { "total": 523, "passed": 517, "failed": 6 },
                                "failed_tests": ["name of failed test"],
                                "crashed_tests": [ { "name": "...", "exit_code": 139 } ],
                                "timed_out_tests": ["name"] },
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
| `test_crash` | the test executable crashed / run did not complete (truncated output) |
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
| `bench-collect-daily.sh` | Builds the compiler at a commit, benchmarks hello (bare/std), runs `-bm-modules -bm-files`, runs test suites per backend → `data/daily/<date>.json`. Flags: `--commit`, `--date`, `--out`, `--work`, `--skip-llvm`, `--no-build`, `--backends`, `--quick` (skip tests, hello + modules only). |
| `bench-collect-release.sh` | Fetches release + assets from GitHub API, downloads the platform's zips, merges the **release's own test sources** (`git archive <tag> lang/tests`), benchmarks + tests with the release binaries → `data/releases/<tag>/...`. Flags: `--tag` (required), `--repo`, `--platform`, `--arch`, `--out`, `--work`, `--skip-llvm`, `--quick`, `--info-only`. **Gotchas: (1) `RELEASE_BIN` must be `./chemical` — bm_run executes via `timeout <cmd>`, and a bare `chemical` is not found in PATH → exit 127 on every release benchmark/test. (2) If the GitHub API fails/rate-limits, existing `info.json` WITH assets is PRESERVED (never clobber good data with an `unavailable` record).** |
| `scripts/llvm.sh` | Downloads the prebuilt LLVM for the host into `out/host`. **`--tag` is optional: when omitted it auto-resolves the LATEST llvm-prebuilt release via the GitHub API — never hardcode a tag in workflows** (the stale `llvm18` default broke `BUILD_COMPILER=ON` configure, which wants the LLVM version in `CMakeLists.txt`'s `find_package(llvm ${LLVM_VERSION})`). |
| `bench-push-pages.sh` | Rebuilds `data/manifest.json` from the merged data tree and pushes **only `data/`** to `gh-pages`. Site files on the branch are preserved. Flags: `--data`, `--site`, `--branch`, `--no-push`. |

> ℹ️ **There is no backfill workflow anymore** (`.github/workflows/benchmark-backfill.yml`
> was removed — historical release backfilling is no longer needed; the data is
> already collected). `bench-collect-daily.sh` is used by the daily workflow;
> `bench-collect-release.sh` is used by the release workflow.

### Key helpers in `bench-common.sh`
- `bm_run <out_status> <out_duration_ms> <timeout_secs> <log> <cmd...>` — runs
  with timeout + logs; status = success|timeout|failed.
- `bench_hello <backend> <bin> <bare|std> <name> <outvar> <logdir>` — measures
  compilation time of a hello-world; Interpreter → `unavailable`.
- `bench_modules_and_tests <backend> <bin> <log_base> <outvar> <logdir> <quick>`
  — compiles `lang/tests/build.lab` with `-bm-modules -bm-files`, then runs the
  test executable (compiled) or `--arg-interpret` (interpreter).
- `parse_test_output <log>` — parses BOTH test sections (see the critical
  parsing note below) → counts, per-section breakdown, `failed_tests[]`,
  `crashed_tests[]` (non-zero exit codes), `timed_out_tests[]`, `complete`.
  **This is the single most important function in the whole system.**
- `parse_bm_output <log>` — parses `[bm:module] 'name' completed [nano:N]...`
  lines → JSON array (tag/name/nanos/micros/millis/secs).
- `expected_assets <tag>` — era-based expected zip name list (used only for
  `missing_asset` markers; actual assets always come from the API).
- `asset_parts <name>` — `linux-x64.zip` → `linux x64 regular`; `-tcc` suffix →
  `tcc` variant.
- `bm_link_shared_tooling <worktree> <main_root>` — links `lib/` and `out/host`
  from the main checkout into a worktree (see Pitfalls).
- `bm_write_json <file> <json>` — atomic write (tmp + mv).

### ⚠️ Test output has TWO sections (the #1 data-integrity trap)

The test executable prints two INDEPENDENT sections, and a parser that reads
only one of them silently drops real failures:

1. **Sequential inline tests** — `test()`/`print_test_stats()` print
   `Test N [name] succeeded/failed` lines and finally `Total N Passed N Failed N`.
2. **`@test` runner** — every `@test` function runs in its OWN child process
   (posix_spawn + socketpair IPC, `lang/libs/test/posix/launch.ch`). The parent
   prints `Test run summary`, `  Total: N | Passed: N | Failed: N`,
   `  - test_name <id>` + `PASS` / `FAIL` / `[exit_code] FAIL`, then
   `Summary: N tests - X passed, Y failed`.

The final `Total N Passed N Failed N` line only counts the SEQUENTIAL section
(runner results live in child processes and never reach `print_test_stats`).
A parser that treats that line as authoritative therefore MISSES every runner
failure — the real run had 2066 sequential tests passing AND 523 runner tests
with 6 failures, but the old parser reported `failed: 0`. **Never replace the
sequential totals with the combined total; ADD the two sections.**

Also: the `Total ...` summary line has **no trailing newline** — the reader
loop must use `while IFS= read -r line || [ -n "$line" ]` or the last line is
silently dropped (that was a real bug: `complete` flipped to false).

Crash semantics (from `launch.ch`): `state.exitCode` is the raw waitpid status;
a non-zero bracket `[139] FAIL` = child crashed (128+signal) or was SIGKILLed on
timeout (which also logs "Test timed out after 10s"). The parser records these
as `crashed_tests`/`timed_out_tests`. The test executable's `main()` ALWAYS
returns 0 even when tests fail — **status must be derived from parsed counts**
(`failed > 0` → `test_failure`), never from the process exit code.

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
   site stays empty while the workflow "succeeds". Every upload in
   `benchmark-release.yml` must keep `include-hidden-files: true`.
3. **upload-artifact strips the search-path prefix.** Uploading
   `path: .bench-data/` produces an artifact whose root IS `.bench-data/`'s
   contents (`daily/`, `releases/` at root — NOT `.bench-data/daily`). The
   publish jobs copy `merged/daily` + `merged/releases` accordingly. If you
   change the upload path, you MUST update the publish copy paths to match.
4. **The same-commit rule.** A compiler built at commit X can only compile the
   test sources of commit X. Daily builds inside a checkout/worktree at the
   target commit; release benchmarks use `git archive <tag> lang/tests`.
5. **`bench-push-pages.sh` is an OVERLAY, never a wipe.** It copies the local
   data on top of what's already published and rebuilds the manifest from the
   merged tree. An empty publish is a harmless no-op. Never replace this with
   `rm -rf` (that wiped the site's history once).
6. **`--quick` still exists as a SCRIPT flag** (`bench-collect-{daily,release}.sh`)
   that skips test suites + module benchmarks, but NO workflow exposes it
   anymore — daily and release both run full suites. A record with
   `build.status: "skipped"` / empty modules means a quick-mode run happened
   some other way.
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
| `benchmark-daily.yml` | schedule 03:00 UTC + manual | Builds the compiler at HEAD **in the main checkout** (NOT a worktree), runs `bench-collect-daily.sh` (all 3 backends by default), pushes to gh-pages. Inputs: `commit` (default HEAD), `skip_llvm` (default **false** — LLVM included). **No `quick` input anymore** — the test suite is ~20 s so a full run is always done. |
| `benchmark-release.yml` | manual (`release_tag`) + on release published | Matrix of platform jobs mirroring build-and-release (linux x64/arm64, alpine x64/arm64, macos x64/arm64, windows x64/arm64, windows-mingw x64/arm64 + msvcrt). Each downloads the release's assets, merges the tag's own test sources, runs hello + module benchmarks AND the full test suite in ONE collect step (no `quick`/`skip_llvm` inputs — deliberately, to avoid a second workflow launch), then publishes → gh-pages. |
| `build-and-release.yml` | manual + `releaseIt` commits | Builds release binaries for all platforms; `config` input: `Release` → `chemical` repo, `RelWithDebInfo`/`Debug` → **`chemical-debug` repo** with `-debug` suffix zips. `deploy-debug` job aggregates debug zips; it has `if: always() && ...` so one failed platform doesn't skip the whole debug publish. |

---

## 5. The dashboard (`gh-pages` branch — assets/dashboard.js)

Pure static JS (~1550 lines, no framework). **Six views**: Overview, Releases,
Daily / Commits, **Failures**, Modules, Backends. Chart.js is vendored at
`assets/vendor/chart.umd.min.js` — reference it exactly (a 404 there =
`Chart is not defined`; the wrong `assets/chart.umd.min.js` path caused that
once, don't reintroduce it).

### Design system (`assets/styles.css`, ~550 lines)
- shadcn-style HSL tokens in `:root` (light) with `.dark` overrides
  (`--background`, `--foreground`, `--primary`, `--muted`, `--border`,
  `--radius: 0.5rem`, …). `body` sets `background: hsl(var(--background))` and
  the **Inter** font; `.mono` uses JetBrains Mono.
- Components: `.card`, `.tab` (+ `.tab-badge`), `.badge-*` (success/missing/
  timeout/unavailable/loading…), `.btn-*`, `.table-wrap` (scrollable, for
  sortable/filtered tables), `.modal`/`.modal-backdrop`/`.modal-card`,
  `.toast-*`, `.chart-wrap` (fixed-height chart container), `.icon-btn`,
  `.chip` (▲ regression / ⚠ incomplete markers), `.missing` (red text),
  `.dim`, `.mono`, `.num` (right-aligned numbers).
- **Theme:** `#theme-toggle` in the topbar switches `.dark` on
  `<html>`; `applyTheme()`/`toggleTheme()` read/write `localStorage`
  `chem-theme` (default **dark**). A tiny pre-paint `<script>` in `index.html`
  applies the saved theme before first paint (no flash). On toggle the code
  rebuilds every chart because they read colors from CSS vars at render time.

### Key facts (each one was a real bug — respect them)
- **Lazy loading:** `init()` fetches `manifest.json`, all daily records, and
  all `info.json` files in parallel. Per-platform record files
  (`data/releases/<tag>/<platform>.json`) are fetched ONLY when needed via
  `ensurePlatformRecord(rec, pf)` — **one platform at a time**. The Releases
  charts call `ensurePlatformRecord(r, selectedPlatformKey)`, NOT
  `ensurePlatformRecords(r)` (fetching all 5 platforms of all 48 releases =
  hundreds of fetches per chart render — the old "loads too slowly"
  complaint). `loadDailyRecord(date)` / `loadPlatformRecord(tag, pf)` cache
  promises (`dailyCache` / `platformCache`).
- **Fetch reliability:** `fetchJson(url)` aborts after 20s
  (`FETCH_TIMEOUT_MS`) so a hung request can't leave the status badge on
  "loading…" forever, **guarded by `typeof AbortController !== "undefined"`**
  for old browsers/WebViews. Failed record loads return `null` instead of
  throwing; every catch path calls `noteLoadError(what, err)` which is
  surfaced in the status badge ("N failed"). Never swallow fetch errors
  silently.
- **Chart reliability:** `makeChart(id, config)` first calls
  `ensureChartWrap(canvas)` which wraps the canvas in a fixed-height
  `.chart-wrap` (charts previously collapsed or varied in height). All data
  passed to Chart.js must go through `num()` / `numOrZero()` (NaN/null/
  undefined → filtered or 0). **Never feed raw record fields to Chart.js** —
  one bad value can blank an entire chart. Charts are cached in the `charts`
  object and destroyed via `destroyChart(id)` before re-creating.
- **Async race tokens:** every async chart/compare render takes
  `const token = chartToken()` and checks `if (chartIsStale(token)) return;`
  after each `await` — a slow fetch must never overwrite a newer render
  (happens on rapid filter/theme switches). `renderCompare` uses `compareSeq`
  and `openReleaseModal`'s async fill uses `modalSeq` for the same reason.
- **Release modal opens INSTANTLY:** `openReleaseModal(tag)` renders the
  summary + assets table + a `<div id="rel-modal-platforms">loading…</div>`
  immediately, then `await ensurePlatformRecords(r)` and fills platform data
  into that container, guarded by `if (!box || myModal !== modalSeq) return;`
  (modal closed or a newer modal replaced it). Do NOT revert to awaiting all
  platform fetches before showing the modal (old slow behavior).
- **Sortable tables without duplicate listeners:** `bindSortable(tableEl)`
  registers one click handler per `th.sortable` only if the element isn't
  already in the `boundSortables` WeakSet (re-renders used to stack
  listeners). Sort state persists per table in `state.sort`; the daily table
  re-applies the current sort after filtering.
- **Variant selector:** the "Binary size over time" line chart reads the SAME
  `#rel-variant-select` as the bar chart (`assetNameFor(pl, ar, variant)`). It
  was hardcoded to `"regular"` once — keep them in sync.
- **Status labels:** `STATUS_LABEL` maps raw statuses (`missing_asset`, etc.)
  to human text so the UI says "missing asset", not cryptic "failed".
- **Platform list source:** the Releases `<select>` options come from
  `releasePlatformsForChart()` which reads `state.manifest.releases` (the
  manifest) — NOT `state.releases[].platforms` (those are empty until lazily
  loaded; using them broke the dropdown once).
- **Size units:** binary sizes render in **MB** (`fmtSize`, the `MB` helper
  `b/1048576`, `barOpts("MB")`). The "All releases" table uses the
  `.rel-table` CSS class (fixed column alignment + MISSING labels).
- **Prefs persist:** theme, backend selections, release variant, and compare
  selections round-trip through `savePrefs`/`loadPrefs` under the
  `chem-dashboard-prefs` localStorage key. New user-tweakable state should be
  added to `state` + persisted here, not hardcoded.
- **Test results are the dashboard's primary focus.** Overview, Daily,
  Failures, and both modals render test status badges, the
  sequential-vs-runner breakdown, and failing-test names (crash exit codes,
  timeout markers). Do not let benchmark timings push test results out of
  sight.

### Views & feature inventory (all live in `assets/dashboard.js`)
- **Overview** (`renderOverview`, `renderOverviewCharts`): stat cards
  (releases tracked, daily points, tests run, failing tests), latest daily
  build table, latest release sizes, hello-world compile-time charts.
  `loadLatestReleaseTests(rel)` lazily pulls the latest release's platform
  tests for the card.
- **Releases** (`renderReleases`, `renderReleaseTable`, `renderReleaseCharts`,
  `renderCompare`, `openReleaseModal`): filter bar (platform / backend /
  variant / search), binary-size line chart, bare-vs-std hello charts,
  release test results, the All-releases table, a **Compare two releases**
  card (side-by-side asset sizes + TCC tests for the selected platform), and
  the full-release modal (instant open, async platform fill).
- **Daily / Commits** (`renderDaily`, `renderDailyTable`, `renderDailyCharts`,
  `openDailyModal`): backend checkboxes, date-range inputs (`dailyFrom` /
  `dailyTo`), status filter (all / any failure / success), search by
  commit/subject. `filteredDaily()` applies all of it; the table rows open a
  per-day modal with per-backend build/benchmark/test detail.
- **Failures** (`collectFailures`, `renderFailures`, `filteredFailures`,
  `renderFailureTable`, `renderFailureCharts`, `openFailureModal`): the
  dashboard's core view. `collectFailures()` aggregates every
  failing/crashed/timed-out test across ALL daily records into a map keyed by
  test name (kind = failed | crash | timeout). The tab shows a live count
  badge (`#failures-count`). Filters: backend (all/TCC/Compiler/Interpreter),
  min-occurrence (≥1/2/3/5/10), search. Clicking a row opens a modal listing
  every occurrence (date, backend, day status, crash exit code, day
  pass/fail).
- **Modules** (`renderModules`, `renderModuleChart`, `moduleNames`): per-module
  compile times from daily records' `modules[]`; pick a module + backend →
  trend chart.
- **Backends** (`renderBackends`, `renderBackendCharts`): backend comparison
  table + build/hello/test history charts.
- **Shared helpers:** `esc(s)` (HTML-escape EVERYTHING that comes from data —
  XSS guard), `csvEscape` + `exportTableCsv(tableEl, filename)` (CSV export on
  every table, shows a toast), `toast(msg)`, `setStatus(text)`, `num`/
  `numOrZero`, `fmtSize`/`fmtMs`/`fmtDate`/`fmtDateTime`, `statusBadge`,
  `testStatusBadge(t)`, `failedTestsHtml(t, limit)`, `regressChip(current,
  prev)` (▲/▼ vs previous point), `helloOf(rec, name)`, `backendOf(dailyRec,
  backend)`, `assetParts(name)` (zip name → platform/arch/variant),
  `assetNameFor(platform, arch, variant)`, `openModal`/`closeModal`,
  `switchView(name)`, `chartColors()` (reads CSS vars → Chart.js colors),
  `hslAlpha(hex, alpha)`, `baseOpts`/`baseLineOpts`/`barOpts`/`tooltipOpts`,
  `hashStr(s)` (stable color cycling).
- **Keyboard:** `/` focuses the active search box; `Esc` closes modals.

### The `state` object (top of file — add new filters here)
```js
const state = {
  manifest: null, daily: [], releases: [],
  selectedBackends: new Set(BACKENDS),        // daily view
  theme: "dark",                              // persisted
  relPlatform: null, relBackend: "TCCCompiler", relVariant: "regular",
  relSearch: "", relCompareA: null, relCompareB: null,
  dailyFrom: "", dailyTo: "", dailySearch: "", dailyStatus: "all",
  modName: null, modBackend: "TCCCompiler",
  failBackend: "all", failSearch: "", failMin: 1,
  sort: {},                                   // per-table sort state
};
```

### Editing the dashboard (site files are NOT on main)
1. Fetch the current files:
   `git fetch origin gh-pages` then
   `git show origin/gh-pages:assets/dashboard.js > /tmp/dashboard.js`
   (same for `styles.css`, `index.html`).
2. Edit, validate with `node --check` (JS) — and run the harness (below).
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

### Validation: build a Node harness against real data (do this for any change)
The dashboard JS is pure enough to unit-test in Node. The pattern used for the
redesign/stability passes: check out the gh-pages branch to a temp worktree
(site + data), then run `dashboard.js` in a `vm` context with stubs for
`document`, `localStorage`, `fetch` (read files from the worktree),
`getComputedStyle`, `Chart`, `AbortController`, `URL`/`Blob` — then exercise
pure functions and render functions against the REAL data files and assert on
the produced `innerHTML`/return values. A typical harness asserts:
- every view's render function completes without throwing (`renderOverview`,
  `renderReleases`, `renderDaily`, `renderFailures`, `renderModules`,
  `renderBackends`),
- `collectFailures()` aggregates correctly; `filteredFailures()`
  (backend/min/search) and `filteredDaily()` (date range/status/search) filter
  correctly,
- `csvEscape`/`exportTableCsv` escape correctly, `assetParts`/`assetNameFor`
  map names correctly, `num()`/`numOrZero()` reject NaN/undefined,
- `renderCompare` renders a table for two different tags, `bindSortable` adds
  exactly one listener per header across re-renders,
- prefs save/load round-trip.
Harnesses live in /tmp (throwaway); the essential thing is the *pattern*:
stub the DOM, load real data, assert on rendered output. `node --check` alone
catches syntax, NOT strict-mode ReferenceErrors or race bugs — the harness
caught `renderCompare`'s undeclared `html` (strict-mode crash) and the
AbortController-missing-in-old-env crash.

### Live verification
After pushing, the GitHub Pages CDN can serve stale files for ~a minute.
Verify with raw.githubusercontent (bypasses CDN cache) or
`curl -s ...?cachebust` — and confirm the deployed file is byte-identical to
what you tested (`sha256sum` both). Browser agents have flaked on stale cached
pages; curl + the Node harness are the reliable checks.

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
  existing `makeChart`/`baseLineOpts` patterns, wrap it in `ensureChartWrap`,
  guard all values with `numOrZero`, take a `chartToken()` and check
  `chartIsStale` after any await, and validate with `node --check` + the Node
  harness."
- **New column:** "Add a 'macOS ARM (TCC)' column to the All releases table in
  `renderReleases()` using `assetNameFor('macos','arm64','tcc')`, keeping the
  `.rel-table` column alignment, then update the manifest if the
  expected-asset list needs it."
- **New data source:** "Collect <X> in `bench-collect-daily.sh` and store it on
  the daily record under a new key, then surface it in the Daily view. Run
  `bash scripts/bench-collect-daily.sh --skip-llvm --quick` locally to verify
  the record shape before wiring the UI."
- **New filter:** "Add a <X> filter to the Failures view. Add its key to
  `state` (e.g. `failX`), wire the control in `renderFailures`, apply it in
  `filteredFailures()`, persist it in `savePrefs`/`loadPrefs` under
  `chem-dashboard-prefs`, and add a harness assertion for the filtered
  output."
- **Fix a workflow:** "In `benchmark-<x>.yml`, when the <y> job fails, the <z>
  publish job is skipped. Add `always()` to <z>'s `if:` (keep the existing
  dispatch guard), and make the artifact download fail loudly with
  `if-no-files-found: error` if nothing exists."
- **Add a CSV column:** "Add a <X> column to the Failures table and include it
  in the CSV export via `exportTableCsv`. Make sure the value is `esc()`-ed in
  the HTML and `csvEscape`-ed in the CSV, and add a harness assertion."
- **Theme-aware chart:** "Make the new <X> chart use `chartColors()`/`cssVar`
  so it re-colors on theme toggle like the others, and rebuild it in the
  theme-toggle handler alongside the existing charts."
- **Full pipeline sanity:** "Verify the benchmark pipeline end to end: dispatch
  the daily workflow, then check https://chemicallang.github.io/chemical/data/
  for today's record and confirm the dashboard renders it (Overview + Daily +
  Backends tabs), reporting any console errors."
