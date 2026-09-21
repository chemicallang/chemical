# Universal Component Testing (WebView) — Final Design & Implementation Plan

**Status:** Final (approved syntax `#universal_test`)
**Date:** September 2026
**Supersedes:** the August 2026 proposal of this file (`@component_fixture` +
`env.fixture("name")` + `env.click(...)`). That design was rejected: JSX is not a
language feature (it only exists inside `#`-macros), tests could not run real JS,
fixtures were stringly-typed, and there was no isolation control. This document
replaces it.

---

## 1. Goals and non-goals

### Goals

1. Run a user's universal components **and their tests** in a real WebView engine.
2. **Full JavaScript power** — a test can execute arbitrary JS against the live DOM
   (not a fixed helper vocabulary).
3. **SSR and production setup** — the fixture is rendered by the exact same
   `#html` / `universal_cbi` pipeline a real page uses (SSR markup, hydration
   boundary, client component function, dispatch). No test-only renderer.
4. **Easy to write** — fixture inline in the test, steps look like Testing
   Library/Playwright.
5. **One WebView by default for many tests** (fast), with an opt-in for one
   WebView per test and for N parallel WebViews.
6. Reuse the existing test reporting/filtering and the WebView library.

### Non-goals

- Replacing the Playwright suite. WebView needs GTK/WebKit + a display; it is a
  developer/native-WebView suite, not the headless-CI backend.
- Streaming SSR, request/response servers, or browser-context isolation
  (`--component-isolate` with separate storage) in the first iteration.
- A new JavaScript parser. Steps are **raw JS text** executed by the engine.

---

## 2. Verified findings (probes run in `lang/compiled`, this machine: GTK3 +
WebKit2GTK-4.1 + `DISPLAY=:0`)

These are not assumptions — they were compiled with `TCCCompiler` and executed.

### 2.1 Probe A — `SSR + hydration + JS interaction + bridge report`

`lang/compiled/universal_test_probe/` renders `#universal Counter` through
`HtmlPage` + `#html`, loads `page.toString()` into one WebView, then runs JS that
reads SSR text, clicks the button, and reports back over the bridge.

Actual output:

```
[native] bridge method=report
PROBE report: ["ssr=Count: 0","after=Count: 1","errors=present"]
PROBE PASS: SSR + hydration + interaction worked
```

Confirms: production SSR, hydration, event handling, and JS→native reporting all
work in this environment.

### 2.2 Probe B — `multiple tests, ONE WebView, native loop`

`lang/compiled/ut_multi_probe/` renders **two** fixture containers
(`data-ut="a"`, `data-ut="b"`, props `start={0}` / `start={5}`) into one page,
registers three JS tests, and has the native bridge handler advance from one test
to the next inside the message loop.

Actual output:

```
collected 3 results:
  ["{\"name\":\"ssr_state\",\"ok\":true,\"msg\":\"a=Count: 0\"}"]
  ["{\"name\":\"increment_updates\",\"ok\":true,\"msg\":\"b=Count: 6\"}"]
  ["{\"name\":\"reset_to_start\",\"ok\":true,\"msg\":\"a=Count: 0\"}"]
```

Confirms: one WebView, many tests, sequential, props survive SSR, per-fixture
containers are independent, and the native side can drive the next test from
inside the `webview_bind` handler.

### 2.3 `#html` cannot embed `#js`

A probe (`lang/compiled/ut_nested_probe/`) with `#html { <Counter /> #js { ... } }`
fails to parse (`#` starts a Chemical value inside `#html`). So `#universal_test`
cannot be sugar over `#html` + a nested `#js`; it needs its own macro/grammar.

### 2.4 Macro registration mechanism

- A CBI is declared in a `build.lab` and its hooks are indexed with
  `ctx.index_cbi_fn(cbi, <macro-name>, <fn>, <CBIFunctionType>)`
  (`lang/libs/universal_cbi/build.lab:98-104`).
- `Parser::parseMacroNodeTopLevel` looks up the hook by the macro name after `#`
  (`parser/statements/AnnotationMacro.cpp:82-100`).
- The body is lexed by whichever user lexer the CBI installed
  (`InitializeLexer`).
- Collector annotations are available to CBIs
  (`AnnotationControllercreateCollectorAnnotation`, `AnnotationController.h:210`).

### 2.5 Existing infrastructure to reuse

- `HtmlPage` (`lang/libs/page/src/page.ch`): `defaultPrepare()`,
  `defaultUniversalSetup()`, `toString()`, `append_js`, `require_component`.
- `html_cbi` (`lang/libs/html_cbi/src/`): `parseHtmlRoot`, `ASTConverter`
  (`converter/language/main.ch`), `convertHtmlComponent` /
  `emit_universal_queue` (`converter/language/component.ch:237`,
  `converter/language/main.ch:416`), JS emission
  (`converter/language/js_embedded_value.ch`).
- `universal_cbi` (`lang/libs/universal_cbi/src/react/macro.ch`): how a macro
  parses a JS+JSX body, generates the component server/client functions, and marks
  a collector annotation.
- The `webview` library: `webview_create`, `webview_load_html`, `webview_show`,
  `webview_run`, `webview_stop`, `webview_bind`, `webview_evaluate_js`,
  `webview_evaluate_js_result`, `webview_destroy`.
- `test` library: `TestFunction`, `TestFunctionState`, `print_test_results`,
  `TestRunnerConfig`, and the `--test-names` / `--test-ids` parsing.

---

## 3. Final syntax

A component test is a top-level `#universal_test` macro whose body is **HTML/JSX
(the fixture) plus one `<script>` element containing the raw JS steps**:

```chemical
// components/Counter.ch — ordinary universal component
#universal Counter(props) {
    state count = props.start || 0
    return <div>
        <button data-testid="inc" onClick={() => count += 1}>Increment</button>
        <span data-testid="count">Count: {count}</span>
    </div>
}

// counter.test.ch
#universal_test("counter increments") {
    <Counter start={0} />
    <script>
        // full JS. globals `t`, `$`, `byTestId`, `byRole`, `expect`, `sleep` are injected.
        expect($('[data-testid=count]').text()).toBe('Count: 0')   // SSR before interaction
        $('[data-testid=inc]').click()
        expect($('[data-testid=count]').text()).toBe('Count: 1')   // after hydration + event
    </script>
}

#universal_test("counter with props") {
    <Counter start={5} />
    <script>
        expect($('[data-testid=count]').text()).toBe('Count: 5')
    </script>
}

// one WebView per test (portals/globals/timers)
#universal_test("portal escapes clipping", isolate) {
    <div style="overflow:hidden"><SelectFixture /></div>
    <script>
        expect(byRole('listbox')).toBeHidden()
        byRole('button', { name: 'Pick a fruit' }).click()
        expect(byRole('option', { name: 'Banana' })).toBeVisible()
    </script>
}

// arbitrary JS is always available
#universal_test("no runtime errors") {
    <Counter />
    <script>
        const errs = t.consoleErrors()
        if(errs.length) { throw new Error('runtime error: ' + errs[0]) }
    </script>
}
```

### Syntax rules

- `#universal_test("name")` — the name is the test name used in reports and
  `--test-names`.
- Optional options after the name:
  `#universal_test("name", isolate)`,
  `#universal_test("name", isolate, timeout = 5000)`,
  `#universal_test("name", group = "counter")`.
- The body has two parts, order-sensitive:
  1. **Fixture**: zero or more HTML/JSX root elements (production SSR + hydration).
  2. **Steps**: exactly one `<script>` element (raw JS, not parsed as HTML and not
     emitted inline). Its content becomes the test function body.
- If the fixture is omitted the test runs against an empty container (useful for
  pure-JS/page-level assertions).
- The fixture is wrapped by the macro in
  `<div data-ut="<name>"> … </div>` so steps can be scoped and multiple tests can
  share one page without colliding.

### Injected JS API (minimal; grows over time)

- Context: `t` — `t.name`, `t.group`, `t.log(msg)`, `t.skip(reason)`,
  `t.consoleErrors()`, `t.sleep(ms)`, `t.waitFor(fnOrSelector, {timeout})`,
  `t.eval(js)`.
- Locators (lazy, auto-wait on assertions): `$(css)`, `byTestId(id)`,
  `byRole(role, {name})`, `byText(text)`, `byLabel(text)`.
- Element handle: `.click()`, `.dblclick()`, `.type(txt)`, `.fill(txt)`,
  `.press(key)`, `.hover()`, `.focus()`, `.check()`, `.uncheck()`,
  `.selectOption(v)`, `.scrollIntoView()`, `.text()`, `.attr(k)`, `.value()`,
  `.isVisible()`, `.exists()`, `.count()`.
- Assertions: `expect(x).toBe / toEqual / toContain / toMatch / toBeTruthy`;
  `expect(el).toHaveText / toContainText / toHaveAttr / toHaveCount /
  toHaveClass / toBeVisible / toBeHidden / toBeFocused / toBeEnabled /
  toBeDisabled`.

Anything not covered is plain JS (`document.querySelector`, `fetch`, etc.).

### Why this shape

- `#`-macro ⇒ JSX is legitimate; no JSX in plain function bodies.
- The fixture goes through `html_cbi`'s existing conversion ⇒ **production SSR**,
  not a special renderer.
- `<script>` is raw JS ⇒ **full JS power** with no JS parser to write.
- One line to declare a test ⇒ **easy authoring**.

---

## 4. Execution model

### 4.1 Default: one process, one WebView, one page, many tests

- At startup the runner builds one `HtmlPage`, calls every test's generated
  fixture function into it (each inside its own `data-ut` container), appends the
  JS harness and every test's step function, then loads the page into **one**
  WebView.
- Hydration happens once for the whole page.
- Tests run **sequentially**: native asks the page to run test *i*; the harness
  runs the steps, catches errors, and calls back
  `{name, ok, msg}`; native records and asks for the next; when done, native
  stops and destroys the WebView.
- Because each test owns its own fixture container, tests do not reset each
  other's state. (Verified in Probe B.)

### 4.2 Isolation and parallelism

- `isolate`: the test is rendered and run in its own page/WebView, created and
  destroyed around it. For portals, globals, timers, navigation, and flaky
  cross-test interference.
- `--ut-workers N` (later phase): shard tests across N WebViews (N windows) that
  each own a page with their subset of fixtures. Default `1`.
- `--headed` opens visible windows (default is a normal window today; a hidden
  mode is a later platform-dependent nicety).

### 4.3 Failure semantics

A test fails if: an assertion throws; a step throws; a command times out; the
page reports an uncaught error / `console.error` (`t.consoleErrors()`, and the
runner captures `window.onerror` / `unhandledrejection` per test); or the WebView
exits unexpectedly. One failure never closes the suite; a WebView crash marks the
current and remaining tests unavailable and exits non-zero.

### 4.4 Reporting

Reuse `TestFunctionState` / `TestLog` / `print_test_results` and the existing
flags (`--test-names`, `--test-ids`, `--failure-only`, `--no-logs`). Add
test-specific detail to failures: test name, operation, selector, expected vs
observed, timeout, captured console errors.

### 4.5 CLI

```
./scripts/test.sh --tcc --universal-tests          # build + run in a WebView
./scripts/test.sh --tcc --universal-tests --headed
./scripts/test.sh --tcc --universal-tests --ut-workers 4
```

Runs from a dedicated module (e.g. `lang/tests/universal_webview/`) so the
normal headless suites are unaffected. Because a display is required, it is an
opt-in suite, never part of `--all` by default.

---

## 5. Architecture

### 5.1 Compile time

`#universal_test` is implemented in the **`html_cbi` plugin** because that is
where production fixture conversion already lives.

1. **Lexing.** `#universal_test` uses the `html_cbi` user lexer. Add a contained
   raw-text mode for `<script>`: when the lexer enters a `<script>` element, the
   content up to `</script>` is a single raw text token (JS is never lexed as
   HTML). This is correct for `#html` too and must be covered by regression tests.
2. **Parsing.** A new hook `universal_test_parseMacroNode` registered under the
   macro name `universal_test` (`build.lab`):
   - reads `("name" [, options])`,
   - calls `parseHtmlRoot` for the body,
   - locates the single `<script>` child and records its raw text as the steps,
   - wraps/records the remaining children as the fixture.
3. **Sym-res.** Reuse `html_symResNode` (same `HtmlRoot`), so component
   references in the fixture resolve exactly as in `#html`.
4. **Replacement.** Build a Scope containing:
   - `func __ut_fixture_<id>(page : &mut HtmlPage) : void { <converted fixture> }`
     where `<converted fixture>` is `ASTConverter.convertHtmlRoot` output wrapped
     in `page.append_html("<div data-ut=\"<name>\">")` / `</div>`. This reuses
     `convertHtmlComponent` / `emit_universal_queue` verbatim, so SSR, the
     `<span data-chx-i>` boundary, the client function, and the dispatch are
     production-identical.
   - `func __ut_steps_<id>() : *char { return "<escaped script text>" }`.
   - Metadata for discovery (see 5.2).
5. **Discovery/reflection.** Create a `universal_test` **collector annotation**
   from the plugin and mark the generated declaration. Add an intrinsic
   `intrinsics::get_universal_tests<UTFunction>()` modeled on
   `InterpretGetTests` (`ast/utils/GlobalFunctions.cpp:1641`) returning
   `UTFunction { id, name, group, isolate, timeout, fixture_fn, steps_fn }`.
   This mirrors `get_tests` and avoids module-init calls (which the language does
   not support at top level — see Probe A's `string()` failure).
   - Chemical binding struct `UTFunction` lives in the new `universal_test`
     library (5.3).

### 5.2 Runtime (new library `lang/libs/universal_test/`)

- `TestRegistry` / `UTFunction` structs and the `universal_test_runner(argc, argv)`
  entry that:
  1. reads `config` exactly like `test`'s runner does
     (`TestRunnerConfig`, `--test-names`, `--test-ids`),
  2. builds the page and the harness string,
  3. creates one WebView, binds the bridge, loads the page, and drives the
     sequential loop (verified protocol in Probe B),
  4. builds `TestFunctionState` entries and calls `print_test_results`,
  5. handles `isolate` by creating/destroying an extra WebView per test.
- `TestHarness` (JS, one string constant) providing `t`, `$`, `byTestId`,
  `byRole`, `byText`, `byLabel`, `expect`, `sleep`, `waitFor`, console capture,
  and the `window.__ut.run(name)` / `window.__webview__.call('done', json)`
  protocol. This is a plain JS string (like the universal runtime) — no compiler
  work.
- The runner appends the harness **before** the step registrations and appends
  `window.__ut_autostart()` after, called on `load`.

### 5.3 Native runner — `universal_test_runner(argc, argv)`

```chemical
// library sketch (verified protocol)
var g_wv : *mut webview::WebView = null
var g_tests : std::vector<UTFunction>
var g_idx : size_t = 0

func ut_done(method : string_view, args : string_view) : string {
    record_result(args)
    g_idx += 1
    if(g_idx >= g_tests.size()) { webview::webview_stop(g_wv) }
    else { webview::webview_evaluate_js(g_wv, ut_run_call(g_idx)) }
    return "{\"ok\":true}"
}

public func universal_test_runner(argc : int, argv : **char) : int {
    var config = parse_test_runner_args(argc, argv)      // reuse test lib
    var tests = intrinsics::get_universal_tests<UTFunction>()
    // build page: for each selected test call fixture_fn(&mut page) + append steps
    // create wv, bind(ut_done), load, show, run, destroy
    // print_test_results(...)
}
```

---

## 6. Implementation plan (final)

### Phase 0 — De-risk the macro (spike, 1–2 days)

Goal: prove `#universal_test` parsing + fixture SSR + steps capture **before**
building the runner.

1. Add raw `<script>` lexing to `HtmlLexer` (`lang/libs/html_parser/src/lexer/`).
2. Add `universal_test_parseMacroNode` + `universal_test_symResNode` +
   `universal_test_replacementNode` to `html_cbi/src/`, registered under macro
   name `universal_test` in `html_cbi/build.lab`.
3. For the spike, emit a single fixture function and print the generated
   `page.toString()` from a small app in `lang/compiled/ut_macro_probe/`.
4. Success = the output HTML contains the production SSR for the fixture and the
   page JS contains the test name + the raw steps.
5. Add a regression test proving `#html { <script>1 < 2</script> }` is unaffected
   (raw text, no parse errors).

If step 1 is unexpectedly invasive, fall back to a dedicated `initializeLexer`
for `universal_test` that installs a lexer with raw-script mode enabled only for
this macro (same code, gated by a flag).

### Phase 1 — Minimal end-to-end (3–5 days)

1. `lang/libs/universal_test/` with `UTFunction`, `TestHarness` JS, and
   `universal_test_runner` (single WebView, sequential, no isolation).
2. `intrinsics::get_universal_tests<UTFunction>()` in
   `ast/utils/GlobalFunctions.cpp` + CBI binder/`GlobalFunctions` registration +
   the Chemical binding.
3. `html_cbi` replacement emits `__ut_fixture_<id>` and `__ut_steps_<id>` and
   marks the collector.
4. Fixture wrapper `data-ut="<name>"`; steps scoped to it by the harness.
5. Reporting via `TestFunctionState`; `--test-names` / `--test-ids`.
6. Sample test module `lang/tests/universal_webview/` with 5 tests (SSR, click,
   props, input, no-errors) and `--universal-tests` in `scripts/test.sh`.

### Phase 2 — Ergonomics and robustness (3–5 days)

1. Full injected API (§3) and friendly assertion failures (selector + expected/
   observed + fixture HTML snapshot).
2. Console/runtime error capture per test; `t.consoleErrors()`.
3. In-place fixture reset is unnecessary (own container) but add `t.reload()` for
   tests that need a fresh document in the same WebView.
4. Timeouts per test (`timeout = N`), `t.skip`.
5. `--headed`, `--failure-only`, `--no-logs`.

### Phase 3 — Isolation, parallelism, CI (3–5 days)

1. `isolate` (fresh WebView per test).
2. `--ut-workers N` with N WebViews, each owning a shard of fixtures.
3. Xvfb support so CI can run the suite headlessly (document `xvfb-run`).
4. Optional: share the harness with the Playwright suite so one fixture
   definition can run under either backend.

---

## 7. Files to add / change

**Add**

- `lang/libs/universal_test/build.lab` and `src/` (`types.ch`, `runner.ch`,
  `harness.ch`, `args.ch`).
- `lang/tests/universal_webview/chemical.mod`, `src/main.ch`, and test files.
- `lang/compiled/ut_macro_probe/` (spike, scratch).

**Change**

- `lang/libs/html_parser/src/lexer/HtmlLexer.ch` + `nextToken.ch` — raw
  `<script>` text mode.
- `lang/libs/html_cbi/src/` — `universal_test` parse/symres/replacement/
  registration; `build.lab` hook indexes.
- `ast/utils/GlobalFunctions.cpp` — `get_universal_tests` intrinsic (+ binder
  registration in `CBI.cpp`/`CompilerBinder` and the Chemical binding).
- `lang/libs/compiler` / `lang/libs/cstd` bindings if the intrinsic needs a
  Chemical declaration.
- `scripts/test.sh` — `--universal-tests` flag.
- `lang/tests/build.lab` — wire the new module (or keep it standalone).

---

## 8. Testing the framework itself

- **Lexer**: `<script>` raw text in `#html` and `#universal_test`; `<`/`>`
  inside JS; nested `</script>` not allowed (document).
- **Macro**: fixture SSR byte-identical to the equivalent `#html` page (compare
  `toString()` of both); steps captured verbatim; options parsed.
- **Runner**: multiple tests one WebView (Probe B scenario as a regression);
  isolation creates/destroys a WebView; filters select the right tests; a failing
  test does not stop the suite; a thrown step is reported with its message.
- **Harness**: each locator/action/assertion has a JS unit test run inside the
  probe page.

---

## 9. Risks and mitigations

| Risk | Likelihood | Mitigation |
|---|---|---|
| Raw `<script>` lexing regresses `#html` | Medium | Gate behind a flag first; regression tests for both macros; Phase 0 proves it before anything else. |
| Cross-plugin reuse impossible | Low | Design keeps everything in `html_cbi`, which already owns fixture conversion and JS emission. |
| WebView not available (no display/GTK) | Medium | Opt-in suite; skip with a clear message when `webview_create` fails; document `xvfb-run` for CI. |
| Fixture SSR drift from production | Low | Reuse `convertHtmlComponent`/`emit_universal_queue` verbatim; assert byte-equality in tests. |
| `intrinsics::get_universal_tests` complexity | Medium | Mirror `InterpretGetTests` exactly; Phase 1 only needs a flat array. |
| Top-level runtime registration unsupported | Confirmed (Probe A `string()` failure) | Use the collector annotation + intrinsic, never module-init calls. |
| Steps need Chemical values | Low | Steps are JS; expose a `t.chemical(...)` or `@{}` in the fixture only. Defer. |

---

## 10. Compromises (explicit)

Accepted to keep the implementation small and the semantics strong:

- Tests live in `#`-macros (the language has no other place for JSX).
- Steps are raw JS in a `<script>` element, not Chemical. This is the price of
  “full JS power” with “no new parser”.
- One WebView/page per runner by default; parallelism is sharded WebViews, not
  isolated browser contexts.
- No test-only renderer: the fixture *is* an `#html` render, so what you test is
  what ships.

---

## Appendix — Probe sources

- `lang/compiled/universal_test_probe/` — SSR + hydration + bridge (Probe A).
- `lang/compiled/ut_multi_probe/` — multi-test one-WebView native loop (Probe B).
- `lang/compiled/ut_nested_probe/` — proof `#html` cannot embed `#js`.
