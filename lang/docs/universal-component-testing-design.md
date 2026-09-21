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

> **How-to guide:** the `.agents/skills/universal_testing/SKILL.md` skill is the
> task-oriented reference for writing/running these tests (syntax, injected JS
> API, gotchas). This document is the design + implementation record.

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
- `--ut-workers N` (not implemented): shard tests across N WebViews (N windows)
  that each own a page with their subset of fixtures. Default `1`.
- **Hidden by default:** the runner does not call `webview_show`, so no window
  flashes during a run. `--ut-headed` shows the window for debugging.

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
./scripts/test.sh --tcc --universal          # build + run in a WebView (hidden)
./scripts/test.sh --tcc --universal --ut-headed   # show the window
./scripts/test.sh --tcc --all                # --all now includes the universal suite
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

### Phase 0 — DONE (de-risked the macro, 2026-09)

Implemented and verified:

1. **Raw `<script>` lexing** — `HtmlLexer` gained `pending_script` / `in_script`
   (`lang/libs/html_parser/src/lexer/HtmlLexer.ch`); `nextToken.ch` now emits the
   content of a `<script>` element as a single raw text token up to
   `</script>` (case-insensitive), and `html_cbi`'s converter emits `<script>`
   content verbatim (no HTML entity escaping). Constructors updated in
   `html_cbi/src/main.ch` and `html/src/tokenizer.ch`.
2. **`#universal_test` macro** in `html_cbi`
   (`lang/libs/html_cbi/src/universal_test/ut.ch`, registered in
   `html_cbi/build.lab` under macro name `universal_test`):
   - custom `UTLexer` (`ut_getNextToken`): delegates to the base Chemical lexer
     for the `("name", options)` arguments, then switches to the html lexer with
     `lb_count = 1` for the body;
   - `ut_parse_body` parses the body, splits the single `<script>` (raw steps)
     from the fixture, and stores both in a `UniversalTestDecl`;
   - `ut_symResNode` synthesizes `func ut_render_<name>(page : &mut HtmlPage)`,
     declares `page`, and calls the existing `sym_res_root` — so component
     references and Chemical interpolations resolve exactly as in `#html`;
   - `ut_replacementNode` wraps the fixture in `<div data-ut="<name>">`, runs it
     through the production `ASTConverter.convertHtmlRoot`, and appends
     `window.__ut_register("<name>", <isolate>, function(t){ <raw steps> });` to
     the page JS bundle.
3. **Two forms**: top-level (`ParseMacroTopLevelNode`) synthesizes the
   `ut_render_<name>` function; statement (`ParseMacroNode`) emits the fixture
   into the enclosing function's `page`. Phase 0 validates the conversion via the
   statement form (`lang/compiled/ut_macro_probe/`); the top-level form compiles
   through type-check but **directly calling `ut_render_<name>` does not resolve
   yet** — the generated symbol is a `FunctionDeclaration` produced in the
   replacement pass, so Phase 1 must expose it through a collector annotation +
   intrinsic instead of relying on symbol resolution.

Verified output from `lang/compiled/ut_macro_probe/` (exit 0):

```html
<div data-ut="counter increments"><span id="u3098…" data-chx-i><div>
        <button data-testid="inc">Increment</button>
        <span data-testid="count">Count: 0</span>
    </div></span></div>
```

```javascript
function ut_macro_probe_Counter(props) { … }            // client component fn
window.$__uni_dispatch('ut_macro_probe_Counter', document.getElementById('u3098…'), {"start":0});
window.__ut_register("counter increments", false, function(t){

            expect($('[data-testid=count]').text()).toBe('Count: 0')
            $('[data-testid=inc]').click()

});
```

Tests: `lang/tests/compiler_plugins/html/src/script_raw.ch` (3 tests) and the
full suites — plugins 1125/1125, libs 650/650, main 2200/2200.

### Phase 1 — Minimal end-to-end (3–5 days)

**DONE (2026-09).**

1. **Discovery** — `html_cbi/build.lab` creates a `universal_test` collector
   annotation; `ut_parseMacroNode` creates `ut_render_<name>` at parse time and
   `controller.collect(...)`s the embedded node with args
   `[name, isolate, group, steps, fixture_fn]`. A new C++ intrinsic
   `intrinsics::get_universal_tests<UTFunction>()` (`InterpretGetUniversalTests`
   in `ast/utils/GlobalFunctions.cpp`) turns the collection into an array.
2. **`lang/libs/universal_test/`** — `runner.ch` (`UTFunction`, the comptime
   `universal_test_runner` + `run_universal_tests`, one-page execution, filters,
   isolation, reporting) and `harness.ch` (the in-page JS harness).
3. Fixture wrapper `data-ut="<name>"`; the harness scopes locators to the test's
   container.
4. Filters `--test-names a,b` and `--test-ids 1,2`.
5. `lang/tests/universal_webview/` (6 tests: SSR, props, click, conditional,
   typing, isolate) and `--universal` in `scripts/test.sh`.

Important: `universal_test_runner` is a **comptime** function, so `ut_all()` is
evaluated at the user call site — after the test module has been parsed and its
`#universal_test` declarations collected (the same pattern as `test_runner`).

### Phase 2 — Ergonomics and robustness (DONE, 2026-09)

- Injected JS API (`$`, `byTestId`, `byRole`, `byText`, `byLabel`, `expect(...)`
  with text/count/attr/class/visibility assertions, actions incl. `type`,
  `press`, `check`, `hover`, `selectOption`).
- Per-test **timeout** (15 s) that fails the test and advances; `t.skip(...)`
  reported as a pass with the SKIP message.
- Uncaught errors / unhandled rejections are captured per test and fail it with
  the message.

### Phase 3 — Isolation and CI (DONE / partial)

- `isolate` runs a test in its own page + WebView (implemented and tested).
- The default is still one page + one WebView for the shared group.
- **Not done:** `--ut-workers N` parallel WebViews (GTK main-loop threading is
  risky), and Xvfb CI wiring. Document `xvfb-run` for headless runs.

### Ported suite (components-e2e → `#universal_test`)

`lang/tests/universal_webview/` now ports the Playwright `components-e2e` suite:
**142 tests** across `tests_core.ch`, `tests_components.ch`, `tests_runtime.ch`,
`tests_edges.ch`, `tests_reactivity.ch`, using the fixtures copied into
`fixtures.ch`. Portal tests are `isolate`. The porting workflow and the
Playwright→harness API mapping are documented in the `universal_testing` skill.

---

## 7. Files to add / change

**Added (Phase 0, done)**

- `lang/libs/html_cbi/src/universal_test/ut.ch` — the `#universal_test` macro.
- `lang/tests/compiler_plugins/html/src/script_raw.ch` — raw `<script>` tests.
- `lang/compiled/ut_macro_probe/` — Phase 0 verification app (scratch).

**Changed (Phase 0, done)**

- `lang/libs/html_parser/src/lexer/HtmlLexer.ch` — `pending_script` / `in_script`.
- `lang/libs/html_parser/src/lexer/nextToken.ch` — raw `<script>` lexing.
- `lang/libs/html/src/tokenizer.ch`, `lang/libs/html_cbi/src/main.ch` — HtmlLexer
  constructor fields + `UTLexer`/`ut_initializeLexer`/`ut_getNextToken`.
- `lang/libs/html_cbi/src/converter/language/main.ch` — verbatim `<script>`
  emission.
- `lang/libs/html_cbi/build.lab` — `universal_test` hook indexes.

**Added (Phase 1–3, done)**

- `lang/libs/universal_test/chemical.mod`, `src/runner.ch`, `src/harness.ch`.
- `lang/tests/universal_webview/` (`chemical.mod`, `src/main.ch`, `src/tests.ch`).
- `ast/utils/GlobalFunctions.cpp` — `InterpretGetUniversalTests` +
  `IntrinsicsNamespace` registration.
- `scripts/test.sh` — `--universal` flag.
- `lang/tests/build.lab` — `test_universal_webview_exe` + `test-universal-tests`.

Run it with `./scripts/test.sh --tcc --universal` (requires GTK3/WebKit2
and a display; for headless CI wrap in `xvfb-run`).

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
