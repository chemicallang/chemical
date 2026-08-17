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
- **Portals** — Select menus inside `overflow: hidden` and `transform` containers must
  open fully visible and clickable (proves the body-portal escapes clipping).
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
6. **Focus trap + focus return on Dialog/Sheet** — on open, save `document.activeElement`
   (the trigger), focus the first focusable inside, trap Tab/Shift+Tab to the dialog, and
   restore focus on close. Wired via `useEffect` with `[isOpen]` deps + `contentRef`.
7. **Select keyboard navigation** — WAI-ARIA combobox/listbox pattern: trigger has
   `aria-haspopup="listbox"`, options are `role="option"` with stable ids
   (`chx-select-opt-N`), `aria-activedescendant` on the trigger tracks the highlighted
   option. ArrowDown/Up move highlight, Home/End jump, Enter selects, printable chars
   typeahead (with 500ms reset), Escape closes. **Gotcha:** `aria-activedescendant` must
   be an inline attribute expression (`props.options[highlight]`), NOT a precomputed
   local — plain locals are frozen at mount and never re-evaluate.
8. **Hydration mismatch detection** — the runtime now warns (once, via
   `$__uni_warn_hydration`) when hydrated text/tag disagrees with SSR, then self-corrects
   instead of silently rendering stale UI. Guarded so busy pages don't spam.
9. **Effects subscribe to reactive deps** — `useEffect(fn, [dep])` where `dep` is a
   computed (e.g. `isOpen` derived from a controlled `props.open`) now re-runs when the
   dep's signal changes, not only on the component's own state assignment. This is what
   makes the Dialog/Sheet focus effect fire when a *parent* toggles `open`.
10. **Portals** — Select menu, Dialog and Sheet now render through
    `$_r.createPortal(...)` into `document.body`, escaping `overflow: hidden` and
    `transform` ancestors (shadcn pattern). Runtime: `__uni_portal` vnode marker
    hydrates SSR'd nodes in place then MOVES them to a body container; `$_urn` renders
    fresh into a body container. `$__uni_floating(trigger, menu, opts)` anchors portaled
    menus under their trigger with fixed coords, re-measured on scroll/resize.

## Portal gotchas (learned the hard way)

- **Menu visibility must NOT be an inline `style` toggle.** The floating helper sets
  `menu.style.*` (position/top/left); a reactive `style={open ? ...}` subscription does
  `el.style.cssText = v` which WIPES the inline positioning. Use a `data-open`
  attribute + CSS `&[data-open="true"] { display: grid; }` instead.
- **Component roots that return `createPortal(<jsx/>)` need converter support.**
  `find_returned_jsx` (react/utils.ch) and the SSR FunctionCall case must unwrap
  `createPortal` to the inner JSX element, otherwise the JS component function is
  never emitted (`missing component function by name`) and SSR is skipped. Also
  register `createPortal` in the hook-name switch (converter_core.ch) so it becomes
  `$_r.createPortal`.
- **SSR renders portal content inline** (no body on the server); hydration moves the
  SSR'd nodes to body. E2E tests must query portaled content from `page`/`document`
  scope, not from inside the fixture: `page.getByRole("listbox")` not
  `fixture.getByRole("listbox")`.
- **Components with portals still need `{...props}`** on their root for `data-testid` /
  custom attributes to reach the DOM (Select root was missing it).

## Production-hardening patterns (verified in browser)

These E2E tests double as living examples of the a11y/UX contracts every interactive
component should meet:

- **Dialog/Sheet:** `role="dialog"`, `aria-modal="true"`, accessible name
  (`aria-label={props.ariaLabel}` on Dialog, `aria-label={props.title}` on Sheet), focus
  moves to the first focusable on open, Tab cycles within, Escape closes, focus returns
  to the trigger.
- **Select:** trigger `aria-expanded`/`aria-haspopup`/`aria-activedescendant` all react
  live; menu is `role="listbox"`; options `role="option"` with `aria-selected`;
  `data-highlighted` on the active option; `scrollIntoView({block:"nearest"})` keeps the
  highlight visible while arrow-navigating.
- **Tabs:** roving tabindex (only the active tab is tabbable) + ArrowLeft/Right,
  Home/End move focus AND selection; panels named via `aria-labelledby`.
- **Accordion:** ArrowDown/Up, Home/End move focus between sibling item triggers via
  `[data-accordion-root]` DOM traversal (works with the children-based API).
- **Dropdown:** menu is portaled like Select; `role="menu"`/`role="menuitem"` on
  Menu/MenuItem.
- **Error boundary:** a component whose render throws is replaced by its
  `useErrorBoundary(fallback)` result or the default `.chx-error-boundary` UI; the
  page keeps working (verified by clicking a counter after a fallback mounts).
  Fallbacks apply per-component (each universal component mounts independently — a
  parent cannot catch a child's render error).

## Portaled menu collision flipping

`$__uni_floating` flips the menu ABOVE the trigger when there is not enough room
below (viewport bottom). The E2E suite caught a real bug here: a fixed-position menu
opening below the fold is unreachable — Playwright cannot scroll a `position: fixed`
element into view. Tests must keep the fixture high enough on the page, or the flip
must kick in (it does: `spaceBelow < menuHeight + gap`).

## Performance notes

- 26 tests across 10 workers finish in ~6–15s plus app build time (~30–60s for the
  Chemical compile step). The browser tests themselves are fast; the Chemical build is
  the slow part.
- After changing `lang/libs/page/src/page.ch` (the runtime JS lives in a C++ string),
  pass `BUILD_NO_CACHE=1` — the cache can otherwise produce stale/broken builds
  (`struct/union/enum already defined`).
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
