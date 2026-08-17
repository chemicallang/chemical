---
name: Components E2E Testing
description: The browser-based end-to-end test suite for the components + universal libraries. Lives in lang/compiled/components-e2e (a standalone Node/TypeScript + Playwright repo, also hosted on GitHub as chemicallang/components-e2e). Covers real-browser verification of SSR → hydration → interaction for every interactive component, plus a runtime error assertion. Use when running, extending, or debugging browser tests, or when a component's generated HTML/CSS/JS passes string tests but misbehaves in a real browser.
---

# Components E2E Testing

The string-level plugin tests (`./scripts/test.sh --tcc --plugins`) assert generated
HTML/CSS/JS output. They cannot prove that clicking a button updates the DOM, that
hydration didn't duplicate nodes, or that state actually propagates. This skill covers
the **browser E2E suite** that verifies real behavior.

## Repo location & GitHub

The suite lives in **`lang/compiled/components-e2e/`** inside the Chemical repo, but it
is a **standalone repo** that is also hosted on GitHub as
**`github.com/chemicallang/components-e2e`**.

- If the directory is present: work in place (it is gitignored as scratch space in the
  main repo, but it is a real git repo of its own — the user pushes it to GitHub).
- If the directory is NOT present (fresh clone of the Chemical repo): clone it from
  GitHub instead of recreating it:

  ```bash
  git clone https://github.com/chemicallang/components-e2e.git lang/compiled/components-e2e
  cd lang/compiled/components-e2e && npm install && npx playwright install chromium
  ```

  The `scripts/build-app.mjs` expects to find the Chemical `TCCCompiler` three levels
  up (`../../../cmake-build-debug/TCCCompiler`), which is exactly where it lands when
  cloned into `lang/compiled/components-e2e`. Set `CHEMICAL_COMPILER=/path/to/TCCCompiler`
  to override.

## What it tests

A Chemical demo app (`app/`) renders every interactive component with stable
`data-testid` hooks. Playwright loads the static output, hydrates, clicks, and asserts
real DOM state:

- **Hydration correctness** — SSR output must become interactive without duplication
  (this caught a real bug: state-wrapped universal children rendered twice).
- **Interactivity** — tabs, accordion, dialog, select, slider, checkbox/switch/radio,
  toggle-group, radio-group, toast auto-dismiss, collapsible, sheet, counter.
- **No runtime errors** — a dedicated test asserts `$__uni_error` never fires.

Every fixture lives in `app/src/main.ch` inside a wrapper with `data-testid`, and state
lives in the fixture (so tests exercise SSR → hydration → click).

## How to run

```bash
cd lang/compiled/components-e2e

node scripts/build-app.mjs      # compile app/ with TCCCompiler → app/output/
npx playwright test             # run all tests (auto-starts the static server)
npx playwright test -g "toast"  # run one test by name
npx playwright test --reporter=list
```

- `playwright.config.ts` starts a static file server over `app/output/` (webServer).
- Rebuild the app after changing **any** Chemical library source
  (`lang/libs/components/`, `lang/libs/page/`, `lang/libs/universal_cbi/`, css/css_parser):
  `node scripts/build-app.mjs`. The compiler itself (`.cpp`/`.h`) requires
  `./scripts/build.sh --tcc` in the parent repo first.
- For a full rebuild pass `BUILD_NO_CACHE=1 node scripts/build-app.mjs`.

## How tests are written

`tests/*.spec.ts` — Playwright Test. Pattern per component:

```ts
test("select opens, picks an option, and reports the value", async ({ page }) => {
  await page.goto("/");                       // static page, already SSR'd
  const fixture = page.getByTestId("select-fixture");
  await expect(fixture.getByRole("listbox")).toBeHidden();   // SSR state
  await fixture.getByRole("button", { name: "Pick a fruit" }).click();
  await fixture.getByRole("option", { name: "Banana" }).click();
  await expect(fixture.getByTestId("select-value")).toHaveText("Chosen: Banana");
});
```

Rules of thumb:

- **Prefer roles over CSS/testid where possible** (`getByRole("button", { name })`,
  `getByRole("slider", { name: "Volume" })`, `getByRole("tabpanel", { name })`). This
  also catches accessibility gaps — e.g. tabpanels without `aria-labelledby` fail to
  resolve by name, which is exactly why Tabs now wires `aria-controls`/`aria-labelledby`.
- **`data-testid` requires `{...props}` spread** on the component root. If a test can't
  find its hook, the component is likely not forwarding props (a real library bug —
  shadcn components all spread props).
- **Controlled state lives in the fixture** (`state value = "x"` in the `#universal`
  fixture), so tests verify both SSR output and the parent's `onValueChange`/`onChange`.
- Timing: use Playwright's built-in auto-waiting; only add explicit `timeout` for
  genuinely slow effects (toast auto-dismiss) — and keep durations short.

## Writing a new fixture + test (step by step)

1. Add a `#universal XFixture(props)` in `app/src/main.ch` with a `data-testid`
   wrapper, render the component with controlled state + an output hook
   (`<p data-testid="x-value">Value: {v}</p>`).
2. Register `<XFixture />` inside the `<main>` block in `main()`.
3. `node scripts/build-app.mjs`, then open `app/output/index.html` in a browser or
   inspect `app/output/index.js` to confirm the SSR HTML and the component's JS.
4. Write `tests/x.spec.ts`, run `npx playwright test -g "x"`, iterate.

## Debugging failures

- **Element not found / strict-mode violation (2 elements):** hydration duplicated a
  node. Look at `$__uni_hydrate_node` in `lang/libs/page/src/page.ch` — the state
  branch must adopt the SSR'd element (the `stateVal.t` path) rather than append a
  fresh copy via `$_urn`.
- **Click has no effect / state frozen:** the component unwraps a reactive prop into a
  plain local (`var pressed = props.pressed || false`). Fix: read `props.x` directly in
  the JSX attribute so the converter emits `$_ucs(() => ...)` / the raw signal and the
  runtime subscribes. See `ToggleGroupItem`/`RadioGroupItem` for the correct pattern.
- **Attribute never lands on the DOM:** component lacks `{...props}` on its root.
- **`data-testid` present in `index.html` but not found by the test:** the SSR html is
  fine but hydration replaced it — re-check the state-hydration path above.
- **Test is flaky:** re-run twice; if a timing edge (e.g. toast duration) is the cause,
  either lengthen the effect in the fixture or use `expect(...).toBeHidden({ timeout })`.
- Screenshots/traces: `test-results/` — `npx playwright show-trace <trace.zip>`.

## How E2E findings map to library fixes

These are the real bugs the suite has caught so far — keep the fixes intact:

1. **Missing `{...props}` spread** on Checkbox/Radio/Switch/Toast/ToggleGroup roots —
   custom attributes (`data-testid`, `aria-*`, `id`) must reach the DOM node.
2. **Frozen reactive props in child components** — `ToggleGroupItem.pressed` and
   `RadioGroupItem.checked` were unwrapped to plain values, so clicking never updated
   `aria-pressed`/`checked`. Fix: use `props.pressed`/`props.checked` directly in the
   attribute expression (the converter wraps props reads in `$_ucs`), never
   `var x = props.x || default` when the value must stay reactive.
3. **State-wrapped universal children hydrated twice** (toast in a `{show ? <Toast/> :
   null}` conditional) — `$__uni_hydrate_node` created a duplicate because it didn't
   recognize the SSR'd element as the state's content. Fixed in `page.ch`.
4. **Tabs tabpanels had no accessible name** — added `id` + `aria-controls` on tabs and
   `id` + `aria-labelledby` on panels (shadcn semantics; panel name = its tab).
5. **Hidden native inputs were unclickable** — `.chx-toggle-input`/`.chx-radio-input`
   were 1px with `pointer-events: none`; now `inset: 0; width/height: 100%; z-index: 1`
   so the whole control is a valid click target (keyboard + Playwright `check()` work).

## Performance notes

- 15 tests across 10 workers finish in ~6–7s plus app build time (~30–60s for the
  Chemical compile step). The browser tests themselves are fast; the Chemical build is
  the slow part.
- `--cache` (default in `build-app.mjs`) skips unchanged modules. Only pass
  `BUILD_NO_CACHE=1` when the compiler/libraries changed in ways the cache misses.
- Keep the demo app single-page: one page load per test group, fixtures stacked in a
  `<main>`, not one page per component.
- Don't add heavy waits; rely on Playwright auto-waiting and WebSocket-free static
  serving.

## Repo structure

```
lang/compiled/components-e2e/
├── app/                      # Chemical demo app (the fixture source of truth)
│   ├── chemical.mod          # imports std/page/html_cbi/css_cbi/js_cbi/universal_cbi/components
│   └── src/main.ch           # all fixtures + main() that emits app/output/
├── scripts/
│   ├── build-app.mjs         # compile + run the app → app/output/
│   └── serve.mjs             # static server (used by playwright webServer)
├── tests/
│   ├── counter.spec.ts       # hydration + runtime-error smoke tests
│   └── components.spec.ts    # one test per interactive component
├── playwright.config.ts
├── package.json              # @playwright/test
└── README.md
```

`app/output/` and `test-results/` are gitignored (generated). Commit the Chemical
`app/` sources, the Node scripts/config, and the `tests/` — that's the whole repo.
