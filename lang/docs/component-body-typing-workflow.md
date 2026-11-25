# Component Body Typing — Delivery Workflow

**Status:** companion to `component-body-typing-plan.md` (v3) · approved scope 2026-11-25
**Approved scope:** all phases 0–5 in one branch, browser-spelled `dom`, all gates, native WebView test + CI, **JS library mode**, **strict library diagnostics**, LSP deferred.
**Local environment note (2026-11-25):** the machine has the GTK/WebKit runtime libraries but no dev headers, no `xvfb-run`, and no passwordless `sudo`. The native `--universal` suite therefore cannot build locally; the test and CI job are still required, and local verification uses the JS/DOM/fixture gates. CI runs the WebView job.
**Purpose:** how the feature is built, tested, measured and reviewed. The plan says *what*; this says *how*, and records the holes found while reviewing the first draft of the workflow.

---

## 1. Principles

1. **One phase, one PR, one gate.** A PR that does not ship a gate that fails without it is not done.
2. **The gate is the contract.** A red gate is fixed or reverted; it is never weakened, skipped, or "temporarily" disabled.
3. **Measure, do not assert.** Every size/performance claim has a command, a baseline and a machine.
4. **Generic compiler, specific plugin.** No `dom`/`universal` string in `compiler/`; no AST reads in backends; no plugin logic in C++.
5. **Fixtures are code.** Small, named, cleaned up after the run; no generated build artifacts checked in.
6. **The plan is reviewed before the code; the code is reviewed against the plan.** Two plan reviews are already done; each phase gets one design/code review.
7. **Counts are not failures in v1.** An untyped edge is a measured number, not an error, until the strict phase. The count itself is tracked so it cannot grow silently.

---

## 2. Per-phase workflow

| step | what happens | artifact |
|---|---|---|
| **Ready** | dependencies green; the gate command runs today; the fixture name and baseline exist | a one-paragraph phase note |
| **Implement** | only the phase's deliverable list; no opportunistic refactors | PR/commit |
| **Gate** | run the phase gate + the existing suites it can affect; save the log | gate log |
| **Perf** | before/after row for build time, plugin build, bundle bytes, index counts | measurement table row |
| **Review** | one reviewer checks the checklist in §8, the gate failing without the change, and the false-positive test | review notes |
| **Merge** | one commit per phase; plan/doc changes separate; goldens separate | commit |
| **Rollback** | revert the phase commit; the feature is additive and each phase is reversible | revert |

**Commit shape:** `component typing: <phase> — <one line>`; fixtures/gates in the same commit as the behavior they pin; generated goldens in their own commit; never mix a refactor with a behavior change.

---

## 3. Test strategy

### 3.1 The layers

| layer | what it catches | where |
|---|---|---|
| **metadata/parser** | `index_with` parse, repeated/comma values, bad value, `clear()` reparse, `addIndexer` copy | parser unit test or a `--check` fixture + log assertion |
| **CBI probe** | enum sync, hook registration, one call per marked module, job lifecycle | a tiny `index_probe` CBI fixture; `-v` dump |
| **index unit** | duplicate ambient names, namespace flattening, internal-namespace leak, zero-arg detection, generated `init`/`drop` filtering | plugin tests or a fixture that asserts the diagnostic |
| **compile-time fixtures** | 001/002/006/008, counted 005, built-ins, `$__uni_*`, untyped props, user provider | `lang/tests/web/fixtures/body_*`, `index_*` |
| **two-job integration** | server skips the check, client runs it, index does not leak | a `--web` fixture with a bad body in both logs |
| **DOM harness** | the `dom` rename and the browser surface still work under Node | `scripts/web-dom-test.sh` |
| **WebView** | the allowed `document`/`window` surface works after SSR/hydration; no false positives in the corpus | `lang/tests/universal_webview/src/tests_document.ch` (new); `--universal` |
| **E2E (Playwright)** | the real browser after hydration | `lang/compiled/components-e2e` (existing) |
| **performance** | build time, plugin build, bundle bytes, index calls/memory | `scripts/web-typing-perf.sh` (new) |
| **determinism** | two client builds produce identical assets/logs | repeat step in the body/index scripts |
| **LSP smoke** | the editor parses `index_with` and does not crash (no diagnostics yet) | a small LSP test or a documented limitation |

### 3.2 Matrix

| feature / rule | unit | fixture | two-job | WebView | perf |
|---|---|---|---|---|---|
| `index_with` parse | ✓ | ✓ | | | |
| enum sync / hooks | ✓ probe | ✓ | ✓ | | |
| index duplicates/flattening | ✓ | ✓ | | | |
| `props.X` (001) | | ✓ neg/pos | ✓ | ✓ corpus | |
| direct ambient member (002) | | ✓ neg/pos | ✓ | ✓ allowed surface | |
| unknown host (008) | | ✓ neg/pos | ✓ | ✓ built-ins | |
| hook placement (006) | | ✓ neg/pos | ✓ | ✓ corpus | |
| counted edge (005) | | ✓ pos | | ✓ | |
| `dom` rename/surface | | ✓ | | ✓ | ✓ bundle |
| server skip / client run | | | ✓ | | |
| index isolation | ✓ | ✓ | ✓ | | |
| cache/enum stale | ✓ | ✓ | | | ✓ plugin |

**Negative** fixtures must fail with the exact code and message; **positive** fixtures must compile with zero body diagnostics and, where relevant, run in the WebView. A rule with only a positive test is not done.

---

## 4. Holes found in the first workflow draft, and the fixes

| id | hole | risk | fix | phase |
|---|---|---|---|---|
| H1 | CI never runs `--universal` (WebView), so a checker false positive in `components` is only seen locally | ships a broken library | add `.github/workflows/universal.yml` (xvfb + GTK/WebKit + `./scripts/test.sh --tcc --universal`), path-filtered + nightly | 3 |
| H2 | no fast CI gate for the new compile-time rules | the rules rot | add `web-index-test.sh`, `web-body-check-test.sh`, `web-two-side-test.sh` to `web-stack.yml` | 2–3 |
| H3 | no test that the server job skips the check and the client runs it | server build breaks, or the check never runs | `web-two-side-test.sh`: bad body → client log has 002, server log does not; good body → both build | 2 |
| H4 | no index-isolation test across the server/client jobs | stale `document` from another job | probe prints `job_begin`/`index_module`; assert empty at start and one provider on the client | 2 |
| H5 | enum sync / stale cached plugin not gated | crash or silently missing hooks | `scripts/check-cbi-enum-sync.sh`; probe built with `-frecompile-plugins` | 1 |
| H6 | `ModuleFileData::clear()` not covered | LSP reparse keeps stale `indexers` | add `indexers` to `clear()`; a parse-twice test | 0 |
| H7 | `addIndexer` view lifetime not covered | use-after-free in generated `build.lab` | copy in `addIndexer`; converter-path fixture | 0 |
| H8 | no positive test for counted/deep chains and built-ins | over-reporting (`Math`, `console`, `classList`) | `fixtures/body_ok_chains`: `document.body.classList`, `Math.round`, `console.log`, `window.$__uni_*`, `sessionStorage`, `props.children`; must build | 3 |
| H9 | no duplicate/internal-namespace/user-provider tests | a user `Document` hijacks `document`, or a legit provider conflicts | `body_two_providers`, `body_internal_namespace`, `body_user_provider`, `body_user_document_unmarked` | 3 |
| H10 | no WebView test that the allowed `document` surface actually works | checker allows something the runtime/SSR cannot do | `tests_document.ch` with a `DocumentFixture` (`document.title`, `document.activeElement` in an effect) and `#universal_test` assertions | 3 |
| H11 | no performance baseline | build-time/bundle regressions go unseen | `web-typing-perf.sh`; rows in the plan §9 | 1/3 |
| H12 | no boundary audit | compiler special-cases `dom`; backend reads AST | `scripts/component-typing-audit.sh` (grep `dom`/`universal` in `compiler/`, enum sync) | 1 |
| H13 | diagnostics not pinned precisely | message quality regresses | each negative fixture asserts component, file, name and suggestion; dedup count | 3 |
| H14 | no determinism test | non-deterministic index order or diagnostics | build the client twice; diff assets and logs | 3 |
| H15 | WebView flakiness | red CI on timing | `xvfb-run`, hard timeout, `t.sleep` only for real timers, no retries-to-pass; quarantine with an owner | 3 |
| H16 | LSP behavior unstated | users expect editor diagnostics | document "not wired in v1"; add a parse smoke | 0 |
| H17 | `dom` rename has no migration gate | stale snake_case users/docs | grep for old names in `lang/libs`, docs, fixtures; update `web-getting-started.md` | 3 |
| H18 | native build of `components` not tested after `import dom if client` | native consumers break | build a components consumer without `--web`; must succeed and not lower `dom` | 2 |
| H19 | `Library + --emit-js` not tested | silent non-library | shell test: rejected with the explicit diagnostic; Interpretation is not JS | 0 |
| H20 | fixture build artifacts leak | dirty tree, accidental commits | every script `trap` cleans its `build/`; tests write to `lang/compiled/`; a `git status --ignored` check | all |
| H21 | `props` shadowing/destructuring not tested | false positive/negative | `body_props_shadow`, `body_props_destructure`; document the count | 3 |
| H22 | `${}`/JSX attribute contexts not tested | a `document.x` hides | `body_jsx_contexts`: `document.title` in an attribute and a child; assert 002/allow | 3 |
| H23 | 0-byte claim untestable | hidden bundle growth | unique debug marker + gz comparison; not a grep for `document` | 3 |
| H24 | plugin hooks not registered in `universal_cbi/build.lab` | the index never runs | registration is a Phase 1 deliverable and part of the probe | 1 |
| H25 | no test for a server-only build with no client job | the check is assumed to run | native-only fixture; no diagnostic; build succeeds | 2 |
| H26 | JS library mode has no test | `--emit-js` on a Library is broken or silently produces nothing | a `library` fixture: public function, no `main`; `--emit-js <dir>`; Node imports `./<module>.mjs`, calls it; `ship_files` present; no `.so` | 0 |
| H27 | strict library is promised but not gated | the corpus is rejected, or the strict flag is never enabled | Phase 5 turns `WEB-BODY-005` into an error only after the corpus scan is empty; the gate fails on any new counted edge | 5 |
| H28 | WebView cannot run on this machine (no headers/xvfb/sudo) | the "run it" decision is unverifiable locally | implement `tests_document.ch` + `universal.yml`; local gates are the fixtures/DOM harness; CI is the WebView judge | 3 |
| H29 | Phase 4 server-side index is under-tested | stale `dom.o`, LLVM accepts `chx_dom_*`, server/client diagnostics disagree | `web-types-only-test.sh` + a two-side diagnostic comparison + cache-bypass test; enforce the caller-site refusal in both C and LLVM | 4 |
| H30 | JS library reachability roots are wrong | a public function is omitted from the library | roots = public functions + address-taken + vtable + async poll pairs + global initializers; a Node import test per public function | 0 |
| H31 | `--web` compiles a shared `#universal` component in both passes and fails (`symbol is not a component`, `@extern` storm) | the real library cannot be checked; Phase 2 used a client-only component to proceed | **Fixed:** `AnnotationController` now owns the names plugins register (the definition no longer points into the first pass's destroyed `build.lab` JIT state), and `scripts/web-two-pass-test.sh` gates a shared component rendered by the server and body-checked by the client. Server-side declarations-only indexing remains Phase 4b. | 4 |
| H32 | `page`/`fs` client builds discard `Result` cleanup calls | the client job rejects `page` before any body check | consume (`var _close… =`) the POSIX and Windows `fs` cleanup Results; `page`'s `write_text_file` discard; already committed as a prerequisite | pre-2 |
| H33 | a successful CBI job is rebuilt on the second `--web` pass | crash / stale plugin state | reuse a `Success` CBI job across passes; side-dependent behavior must use `JobBegin` at runtime, not plugin compile-time branching | pre-2 |

---

## 5. New gates and fixtures (inventory)

### Scripts (new)

| script | command | what it asserts |
|---|---|---|
| `scripts/check-cbi-enum-sync.sh` | `./scripts/check-cbi-enum-sync.sh` | `CBIFunctionType` values/order match in `CBIFunctionType.h` and `lab.ch` |
| `scripts/web-index-test.sh` | `./scripts/web-index-test.sh` | parse/round-trip, probe, namespace flattening, duplicates, internal namespace |
| `scripts/web-body-check-test.sh` | `./scripts/web-body-check-test.sh` | 001/002/006/008 negative, 005 positive, built-ins, `$__uni_*`, dedup, message quality |
| `scripts/web-two-side-test.sh` | `./scripts/web-two-side-test.sh` | server skips, client runs, index isolation, native-only build |
| `scripts/web-two-pass-test.sh` | `./scripts/web-two-pass-test.sh` | a **shared** `#universal` component builds in both `--web` passes; the server renders its markup, the client body-checks it and ships its function; no `symbol is not a component` |
| `scripts/web-types-only-test.sh` | `./scripts/web-types-only-test.sh` | Phase 4: declarations-only `dom` on a server job; caller-site refusal; no `chx_dom_*`; cache bypass |
| `scripts/web-js-library-test.sh` | `./scripts/web-js-library-test.sh` | `Library + --emit-js`: no `main`, one `.mjs` per module, Node import/call, `ship_files`, no `.so` |
| `scripts/web-typing-perf.sh` | `./scripts/web-typing-perf.sh` | cold/warm build, plugin build, bundle gz, index count/RSS |
| `scripts/component-typing-audit.sh` | `./scripts/component-typing-audit.sh` | no `dom`/`universal` names in `compiler/`; enum sync; no AST reads in backends |

### Fixtures

Place under `lang/tests/web/fixtures/`:

- `index_parse` — `index_with` forms; generated build.lab path.
- `index_probe` — a probe CBI that prints `job_begin`/`index_module`/counts.
- `body_unknown_prop` — 001.
- `body_unknown_member` — `document.titel` → 002.
- `body_no_provider` — `document` without `dom` → 008.
- `body_hook_placement` — 006.
- `body_ok_surface` — allowed roots and built-ins; must build.
- `body_ok_chains` — `document.body.classList`, `Math`, `console`, `$__uni_*`, `sessionStorage`; must build.
- `body_props_shadow` / `body_props_destructure` — counted; no false 001.
- `body_two_providers` / `body_internal_namespace` / `body_user_provider` / `body_user_document_unmarked`.
- `body_jsx_contexts` — attribute + `${}` child.
- `shared_component` — a `#universal` component in a module compiled by both `--web` passes, rendered by the server `src` through `#html`, with its provider imported `if client` (the real `import dom if client` shape). Gate: `web-two-pass-test.sh`.

Every fixture has a `chemical.mod` with `import dom if client` where needed, writes its build to a temp/`lang/compiled` dir, and is removed by the script.

### WebView fixture

`lang/tests/universal_webview/src/tests_document.ch` (new) plus a `DocumentFixture` in `fixtures.ch`:

```chemical
#universal DocumentFixture(props) {
    state title = ""
    state active = ""
    useEffect(() => {
        title = document.title
        active = document.activeElement ? document.activeElement.tagName : ""
    }, [])
    return <div data-testid="document-fixture" data-active={active}>{title}</div>
}
```

```chemical
#universal_test("allowed document surface works") {
    <DocumentFixture />
    <script>
        const f = byTestId('document-fixture')
        expect(f.text().length > 0).toBe(true)
        expect(f.getAttribute('data-active')).toBe('BODY')
    </script>
}
```

This is the test that ties the checker's allow-list to the real SSR/hydration path. It must be part of the phase-3 gate.

---

## 6. WebView testing

**What it proves:** the allowed `document`/`window` members are usable in a real WebKit page after SSR + hydration; a false-positive rule would fail the `--universal` build before the test runs.

**What it does not prove:** any compile-time rejection. That is the fixture scripts' job. A rejected `document.titel` never reaches a WebView.

**CI:** add `.github/workflows/universal.yml`:

```yaml
on:
  pull_request:
    paths:
      - 'lang/libs/components/**'
      - 'lang/libs/dom/**'
      - 'lang/libs/universal_cbi/**'
      - 'lang/libs/page/**'
      - 'lang/libs/lab/**'
      - 'compiler/cbi/**'
      - 'compiler/lab/**'
      - 'lang/tests/universal_webview/**'
      - 'lang/docs/component-body-typing-*.md'
      - 'scripts/test.sh'
  schedule:
    - cron: '0 4 * * *'
```

Steps: checkout, `./scripts/setup.sh`, `sudo apt-get install -y xvfb libgtk-3-dev libwebkit2gtk-4.1-dev`, `./scripts/configure.sh --no-llvm --generator Ninja`, `./scripts/build.sh --tcc -j "$(nproc)"`, `xvfb-run -a ./scripts/test.sh --tcc --universal --ut-headed-off`, upload logs on failure.

**Flake policy:** no retries that hide a failure; a known flaky test gets a quarantine file with an owner and a deadline; `t.sleep` only for real timers; every test has a timeout.

---

## 7. Performance workflow

### 7.1 Baselines

Capture on the same machine, `TCCCompiler` at the phase's parent commit:

| metric | command |
|---|---|
| cold components build | `rm -rf <build>`; `time cmake-build-debug/TCCCompiler lang/libs/components/chemical.mod ... --no-cache` (or the closest web fixture) |
| warm build | same, cache enabled |
| plugin build | `... -frecompile-plugins` with timing |
| client bundle | `gzip -9 -c <assets>/*.mjs | wc -c` |
| index work | `-v` line: modules/entries/conflicts; count `IndexModule` calls |
| peak RSS | `/usr/bin/time -v` |

### 7.2 Budgets

| budget | limit | failure action |
|---|---|---|
| client build | ≤ +2% | revert or record a new budget in the plan with a reason |
| plugin build | ≤ +10% | same |
| bundle | 0 B | same |
| compiler binary | < 5 MB | release gate |
| index memory | O(public declarations) | same |

### 7.3 Implementation rules

- One `IndexModule` call per marked module per job; one `JobBegin` clear. Log a counter; never rebuild per component.
- Hash maps, not ordered maps; keys are `string_view` into the job arena; reserve; no per-lookup string copies.
- The index is immutable before parallel body checks; no locks in the hot path.
- `types_only` (Phase 4) must bypass the object cache; the stale `dom.o` is a correctness bug, not a perf one.
- No MIR/LLVM changes in v1; no bytes ship.

---

## 8. Code quality checklist

**Compiler**
- [ ] No `dom`/`universal` string in `compiler/` (except generic docs/tests).
- [ ] `IndexModule`/`JobBegin` appended to both enums; enum-sync script green.
- [ ] Hook typedefs in `Model.h`; `<vector>` included; ABI spelling documented.
- [ ] `addIndexer` copies; `ModuleFileData::clear()` updated.
- [ ] `do_library_job` rejects `Library + --emit-js`; Interpretation excluded from `target_data.js`.
- [ ] No backend reads the AST.

**Plugin**
- [ ] `index.ch` (data), `ambient.ch` (host list/built-ins), `body_check.ch` (walk/diagnostics) separate.
- [ ] No panics; diagnostics through `ASTDiagnoser`; a `Result` where a call can fail.
- [ ] Zero-arg detection excludes `&self`; generated `init`/`drop` filtered.
- [ ] `$__uni_*`/`__reportError` exempt in both name and member resolution.
- [ ] One diagnostic per component + name; message includes component, file/line, name, suggestion.

**Tests**
- [ ] Every rule has a negative fixture that fails without it and a positive fixture that builds.
- [ ] Two-job test; index isolation; determinism; WebView allowed-surface test.
- [ ] No generated artifacts; scripts clean up.
- [ ] CI gates added and path-filtered.

**Docs**
- [ ] `component-body-typing-plan.md` updated (decisions, budgets).
- [ ] `web-stack-plan.md` links this workflow.
- [ ] `web-getting-started.md` documents `import dom if client`.
- [ ] `dom` docs use browser spelling; migration note.

---

## 9. Review protocol

1. **Red team each rule.** Try to produce a false positive (legit code rejected) and a false negative (`document.titel` accepted). Add the best attempt as a permanent fixture.
2. **Review the gate, not just the code.** Run it with the behavior reverted; it must fail. If it passes, the gate is wrong.
3. **Review the diagnostic.** Would an agent know what to change from the message alone? If not, fix the message before merging.
4. **Review the boundary.** Does the change put DOM/universal knowledge in the compiler, or an AST read in a backend? Send it back.
5. **Triage a red gate.** Reproduce, minimize to a fixture, add it as a test, then fix. Never delete or weaken the test.
6. **Phase PR review.** One reviewer for each phase, using §8; the plan's decisions table is the tie-breaker.

---

## 10. CI wiring

Add to `.github/workflows/web-stack.yml` (fast, no display):

- `./scripts/check-cbi-enum-sync.sh`
- `./scripts/web-index-test.sh`
- `./scripts/web-body-check-test.sh`
- `./scripts/web-two-side-test.sh`
- `./scripts/web-js-library-test.sh`
- `./scripts/component-typing-audit.sh`
- `./scripts/web-types-only-test.sh` (Phase 4)
- the existing `web-dom-test.sh`, `web-golden-test.sh`, `web-split-test.sh`, `js-backend-suites.sh`

Add a separate `universal.yml` (WebView, display) per §6. Because the local machine cannot build WebKit (no dev headers/xvfb/sudo), CI is the authoritative WebView run; the PR must still pass the fast gates locally.

Run `./scripts/test.sh --tcc --plugins` in CI when `lang/libs/*_cbi/**` or `compiler/cbi/**` changes (it is already the plugin regression suite; the plan's changes are plugin-side). Run `components-e2e` nightly or in the release gate when `lang/libs/components/**` or `lang/libs/dom/**` changes.

Artifacts on failure: the fixture logs, `/tmp/web_*`, `lang/compiled/web-*`, and the `-v` index dump. Never upload `node_modules` or build caches.

---

## 11. Definition of done (feature)

- [ ] Phase 0–5 complete on `feature/component-body-typing`.
- [ ] Rules 001/002/006/008 and 005 implemented; **strict library** (`005` error) green over `lang/libs/components`.
- [ ] Compiler-side generic hooks + metadata + JS library mode; no DOM special-case.
- [ ] `dom` browser spelling and full surface (`Element`/`DOMRect`/`classList`/`style`/`dataset`, chains, `Option<T>`).
- [ ] `components/chemical.mod` uses `import dom if client` for the checkpoint and `dom` is declarations-only on the server in Phase 4.
- [ ] Full negative/positive fixture set; two-job, index isolation, determinism, WebView allowed-surface test.
- [ ] The two-side gate uses a **shared** `#universal` component compiled by both `--web` passes (H31 fixed), not the Phase 2 client-only checkpoint.
- [ ] JS library mode: `Library + --emit-js` produces importable modules, no `main`, no `.so`.
- [ ] CI: web-stack steps + universal job; `--plugins` green; components-e2e scheduled.
- [ ] Perf rows: build (both sides), plugin build, bundle, index count/RSS.
- [ ] Docs: plan, workflow, web-stack-plan link, getting-started, dom migration, LSP limitation.
- [ ] No ignored build artifacts left by the feature; `git status` clean.

---

## 12. Open workflow questions

1. Is a dedicated `universal.yml` (full `--universal` on every PR path) affordable, or should it be nightly + pre-release only?
2. Should `--plugins` move into `web-stack.yml`, or stay in its own job?
3. Is `components-e2e` a release gate or nightly only?
4. Do we want a `--check-index` mode that dumps the ambient table for debugging/tests, or is `-v` enough?
5. How much of the performance harness should live in `scripts/bench-*` (dashboard) vs. a local `web-typing-perf.sh`?
