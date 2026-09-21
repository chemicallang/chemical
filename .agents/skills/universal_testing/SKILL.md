---
name: Universal Component Testing (WebView)
description: How to write and run #universal_test component tests — the native WebView test suite for universal components. Covers the macro syntax (inline fixture + raw JS <script> steps), the injected JS API, isolation/filters, the discovery intrinsic and runner library, how the fixture is SSR'd with the production pipeline, and the implementation gotchas. Load when writing component tests, extending the universal_test library/harness, or debugging the #universal_test macro.
---

# Universal Component Testing (WebView)

`#universal_test` runs a universal component in a **real WebView**, renders its
fixture with the **exact production SSR + hydration pipeline** (`#html` /
`universal_cbi`), and runs **arbitrary JavaScript** against the hydrated DOM.

It is the native alternative to the Playwright `components-e2e` suite. Use it
when you want to test components in Chemical itself, on the same engine your
WebView app ships with. Use `components_e2e` for headless CI / cross-browser
coverage.

- **Syntax:** a top-level macro, so JSX is legal (it lives inside a `#`-macro).
- **Fixture:** inline HTML/JSX — rendered once with production SSR, wrapped in
  `<div data-ut="<name>">`.
- **Steps:** one `<script>` element containing raw JS (full JS power).
- **Default:** one process, one page, one WebView, many tests, sequential.
- **Isolation:** `isolate` gives a test its own page + WebView.

> **Requires GTK3 + WebKit2GTK and a display.** It is an opt-in suite
> (`--universal-tests`) and is **not** part of `./scripts/test.sh --all`. For
> headless CI wrap the run in `xvfb-run`.

---

## Quick start

```chemical
#universal Counter(props) {
    state count = props.start || 0
    return <div>
        <button data-testid="inc" onClick={() => count += 1}>Increment</button>
        <span data-testid="count">Count: {count}</span>
    </div>
}

#universal_test("counter increments") {
    <Counter start={0} />
    <script>
        expect($('[data-testid=count]').text()).toBe('Count: 0')  // SSR before hydration
        $('[data-testid=inc]').click()
        expect($('[data-testid=count]').text()).toBe('Count: 1')
    </script>
}

public func main(argc : int, argv : **char) : int {
    return universal_test_runner(argc, argv)
}
```

Module (`chemical.mod`) must import the macro plugin, the runner and the
WebView stack:

```
application my_component_tests
source "src"
import cstd
import std
import page
import html_cbi
import css_cbi
import js_cbi
import universal_cbi
import universal_test
import webview
import window
```

Run it:

```bash
./scripts/test.sh --tcc --universal-tests                 # all tests
./scripts/test.sh --tcc --universal-tests --test-names "counter increments"
# show the WebView window (default is hidden/off-screen):
./scripts/test.sh --tcc --universal-tests --ut-headed
# or build the module directly:
cmake-build-debug/TCCCompiler path/to/chemical.mod -o /tmp/tests \
    --mode debug_quick --no-cache -frecompile-plugins
/tmp/tests [--test-names a,b] [--ut-headed]
```

Output is `PASS <name>` / `FAIL <name>: <message>` plus a summary; exit code is
non-zero if any test failed. **The WebView is hidden by default** (no window
flashes); pass `--ut-headed` to watch a run.

> `--test-names` splits on commas, so a test whose *name* contains a comma
> cannot be selected individually. Keep test names comma-free.

---

## Syntax

```
#universal_test("name" [, isolate] [, group = "g"]) {
    <fixture JSX/HTML/>          // zero or more root elements
    <script> ...raw JS steps... </script>   // exactly one
}
```

- `"name"` is the test name used in reports and `--test-names`.
- `isolate` runs the test in its own page + WebView (portals, globals, timers,
  navigation, flaky interference).
- `group = "g"` is stored on the test metadata (reserved for grouping).
- The fixture may be omitted (empty container) for pure JS assertions.
- The steps **must** be the single `<script>` element; its content is captured
  verbatim as JavaScript. It is **not** HTML-escaped and **not** Chemical — write
  plain JS.

### Steps are async

Each `<script>` body is wrapped in `async function(){ ... }`, so you can
`await t.sleep(ms)` (and return Promises). The context object is the **global
`t`** (not a parameter) — a test may freely declare its own `const t = ...`
without colliding, but then it shadows `t` for the rest of that test.

### How the fixture is rendered

The macro desugars to `ut_render_<name>(page : &mut HtmlPage)`, which:

1. appends `<div data-ut="<name>">`,
2. runs the fixture through `ASTConverter.convertHtmlRoot` — the **same**
   converter `#html` uses — so SSR markup, the `<span data-chx-i>` hydration
   boundary, the client component functions and the `window.$__uni_dispatch(...)`
   calls are byte-for-byte what a production page emits,
3. appends `</div>` and `window.__ut_register("<name>", <isolate>, function(t){ <steps> });`.

The runner builds one `HtmlPage`, appends the JS harness, calls every fixture
function, loads the page into one WebView, and the in-page harness runs the
registered tests sequentially.

---

## Injected JS API

Everything is scoped to the current test's `[data-ut="<name>"]` container.

### Locators (lazy handles)

| Helper | Description |
|---|---|
| `$(css)` / `byCss(css)` | first match by CSS selector |
| `byTestId(id)` | `[data-testid="id"]` |
| `byRole(role, { name })` | `[role="role"]` (optionally matching accessible name) |
| `byText(text)` | first leaf element whose text equals `text` |
| `byLabel(label)` | `[aria-label="label"]` |

### Handle methods

`.text()`, `.value()`, `.attr(name)`, `.exists()`, `.count()`, `.isVisible()`,
`.click()`, `.dblclick()`, `.type(v)` / `.fill(v)`, `.press(key)`, `.hover()`,
`.focus()`, `.blur()`, `.check()`, `.uncheck()`, `.selectOption(v)`,
`.scrollIntoView()`.

### Assertions

```js
expect(value).toBe(x); expect(value).toEqual(x); expect(value).toContain(s);
expect(value).toBeTruthy();
expect(handle).toHaveText(s); expect(handle).toContainText(s);
expect(handle).toHaveAttribute(k, v); expect(handle).toHaveCount(n);
expect(handle).toHaveClass(c);
expect(handle).toBeVisible(); expect(handle).toBeHidden();
```

### Utilities

- `t.sleep(ms)` — returns a Promise (the harness awaits returned Promises).
- `t.log(msg)` — console log.
- `t.skip(reason)` — reported as a pass with the `SKIP:` message.
- Anything else is plain JS: `document`, `fetch`, `setTimeout`, etc.

Uncaught errors and unhandled rejections are captured per test and fail it with
the message. A test that never settles is failed by a **15 s timeout** and the
suite continues.

---

## Execution model

- **Default (fast):** every non-isolated test's fixture is SSR'd into **one
  page**; the whole page is hydrated once in **one WebView**; tests run
  **sequentially** against their own containers (no reset needed).
- **`isolate`:** the test gets its own page + WebView, created and destroyed
  around it. The suite keeps running after it.
- **Filters:** `--test-names a,b` and `--test-ids 1,2` select tests before any
  page is built. Names must match exactly (spaces included).
- Results are keyed by name; the report follows declaration order.

---

## Architecture (for extending/debugging)

| Layer | Location |
|---|---|
| `#universal_test` macro (lexer, parse, symres, replacement) | `lang/libs/html_cbi/src/universal_test/ut.ch` |
| Macro registration + `universal_test` collector annotation | `lang/libs/html_cbi/build.lab` |
| Raw `<script>` lexing | `lang/libs/html_parser/src/lexer/{HtmlLexer.ch,nextToken.ch}` |
| Verbatim `<script>` emission in `#html` | `lang/libs/html_cbi/src/converter/language/main.ch` |
| Discovery intrinsic `intrinsics::get_universal_tests<UTFunction>()` | `ast/utils/GlobalFunctions.cpp` (`InterpretGetUniversalTests`) |
| Runner (`UTFunction`, `universal_test_runner`, `run_universal_tests`) | `lang/libs/universal_test/src/runner.ch` |
| In-page JS harness | `lang/libs/universal_test/src/harness.ch` |
| Suite | `lang/tests/universal_webview/{chemical.mod,src/main.ch,src/fixtures.ch,src/tests_*.ch}` |
| CLI | `./scripts/test.sh --universal-tests` (`--ut-headed` to show the window) |

**Discovery flow.** `ut_parseMacroNode` creates the fixture function at parse
time and `controller.collect(...)`s the embedded node with args
`[name, isolate, group, steps, fixture_fn]`. The C++ intrinsic turns that
collection into a `UTFunction[]`. `universal_test_runner` is a **comptime**
function, so `ut_all()` is evaluated at the user's call site — after the test
module has been parsed/collected (the same pattern as `test_runner`). That is
why the test declarations must be reachable from the module that calls the
runner.

---

## Gotchas

- **JSX is only legal inside `#`-macros.** You cannot write `<Counter/>` in a
  plain function; `#universal_test`'s body is the macro that enables it.
- **`#html` cannot embed `#js`.** `#universal_test` exists as its own macro
  because the steps need a raw-JS section, which `#html` does not have.
- **`<script>` is a raw-text element.** Its content is lexed verbatim up to
  `</script>` (case-insensitive) — do not nest `</script>` inside the steps.
- **Locators are scoped** to the test's `data-ut` container, so `data-testid`
  collisions across tests are fine.
- **`UTFunction` fields are `string_view`** (like `TestFunction`). `std::string`
  fields in the comptime array break the 2c/TCC translation, and the member
  order must match the struct declaration (`id, name, group, isolate,
  fixture_fn, steps`).
- **Display required.** If `webview_create` fails, the runner prints a hint and
  exits non-zero. Use `xvfb-run -a ./tests` on headless machines.
- **Debugging the generated page JS:** set `UT_DUMP_JS=1` to print
  `page.toStringJsOnly()` between `===UT_JS_START===`/`===UT_JS_END===` markers
  and exit before opening a WebView. A page-script syntax error otherwise hangs
  the run (the harness never starts); dump + `node --check` finds it.
- **`--universal-tests` is not in `--all`** (like `tls`, it is opt-in).

---

## When to use this vs. `components_e2e`

| | `#universal_test` (WebView) | `components_e2e` (Playwright) |
|---|---|---|
| Engine | the app's real WebView (GTK/WebKit, WebView2) | Chromium (Playwright) |
| Language | Chemical + raw JS | TypeScript |
| CI | needs a display (`xvfb-run`) | headless |
| Best for | testing components in the shipping engine, local dev, WebView integration | cross-browser, headless CI, broad component matrix |

Both exercise the same SSR → hydration → interaction contract; keep the
Playwright suite as the CI backend and use `#universal_test` for engine-native
verification.

## The ported suite (components-e2e → `#universal_test`)

`lang/tests/universal_webview/` is a port of the Playwright `components-e2e`
suite (142 tests). Layout:

- `src/main.ch` — the runner `main` + a few small components.
- `src/fixtures.ch` — all `#universal *Fixture` components copied verbatim from
  `lang/compiled/components-e2e/app/src/main.ch`.
- `src/tests_core.ch` — counter/ssr/root-shapes/static-children.
- `src/tests_components.ch` — the component suite.
- `src/tests_runtime.ch` — batching/unmount/effect-deps/derived lists/keyed
  reconciliation/error boundaries/memo/SVG/ref/Suspense.
- `src/tests_edges.ch` — edge + utility + controlled components.
- `src/tests_reactivity.ch` — rapid interactions, hydration, ARIA.

### Porting a Playwright test

1. Reuse the existing fixture component (`<XFixture />`) if one exists; otherwise
   copy it from the e2e app's `main.ch` into `fixtures.ch`.
2. Write a `#universal_test("...") { <XFixture /> <script>...</script> }`.
3. Translate the Playwright API to the harness:
   - `page.getByTestId(x)` → `byTestId(x)`; `f.getByTestId(x)` →
     `byTestId('f').getByTestId(x)`; `locator(css)` → `find(css)` (first) or
     `findAll(css)` (collection); `.nth(i)`/`.first()`/`.last()`/`.count()`.
   - `getByRole(role, {name})` → `byRole(role, { name })` (also matches native
     tags and `aria-label`/`aria-labelledby`/wrapping-label names).
   - `expect(...)` assertions map 1:1, including `.not.*`, `toBeDisabled`,
     `toBeEnabled`, `toBeChecked`, `toBeFocused`, `toHaveValue`, `toHaveAttribute`,
     `toHaveCSS`, `toHaveJSProperty`, `toBeGreaterThanOrEqual`, …
   - `page.keyboard.press(k)` → `t.press(k)` (Tab focus movement is emulated);
     `locator.press(k)` → handle `.press(k)`.
   - `locator.fill(v)`/`el.value=…; dispatch input` → `.setValue(v)` / `.fill(v)`.
   - `await page.waitForTimeout(ms)` → `await t.sleep(ms)`.
   - `page.evaluate(() => window.__x)` → read `window.__x` directly.
4. **Portals** (Dialog/Sheet/Select/Dropdown/Toast) append to `document.body`,
   outside the test's container — mark those tests `isolate` so their portal
   content cannot collide with another test's fixture on the shared page.
5. Components that do **not** forward `data-testid` (Card, Badge, Avatar,
   Typography, Tooltip, Alert, Separator, Stack, Grid, Skeleton, Spinner, Text,
   Collapsible, Slider, Container, Fab) need structural selectors, text/role
   queries, or an attribute the component *does* render (`[data-variant]`,
   `[data-size]`, `[data-muted]`, `role`, tag name).
6. Run and iterate:
   `cmake-build-debug/TCCCompiler lang/tests/universal_webview/chemical.mod -o /tmp/t \
       --mode debug_quick --no-cache -frecompile-plugins && /tmp/t --test-names "your test"`

## Related

- `universal` — the SSR/hydration pipeline the fixture uses.
- `testing` — the general test infrastructure.
- `components_e2e` — the Playwright browser suite.
- `lang/docs/universal-component-testing-design.md` — full design + implementation plan.
