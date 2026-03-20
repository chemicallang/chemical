---
name: universal
description: Diagnose, fix, and implement features in the Chemical universal component pipeline. Use when working on `#universal` components, `universal_cbi`, generated SSR/hydration output, `HtmlPage` runtime behavior, or bugs where compiled HTML/CSS/JS disagree with source components.
---

# Universal

Universal library is present in `lang/libs/universal_cbi`, its a macro processing library, it handles
specifically `#univeral` macros, for examples you can look for components present in `lang/libs/components`

Universal library emits components, Universal library is meant for server side rendering + hydration, The reason
its called universal is because we have libraries like `react_cbi`, `preact_cbi`, `solid_cbi`, they do not do hydration + ssr,
instead they just translate jsx to runtime calls, which is way easier. Universal components are supposed to
work inside react, preact or solid components, we have another `#html` macro, library `html_cbi`, universal components
work inside the html macro too. So yeah, Universal components work everywhere, they are fast because they do ssr + hydration.

### How universal performs ssr + hydration.

Universal uses the compiler api from the compiler library (`lang/libs/compiler`), it generates a function (a server function) that exists in the binary
that function takes three parameters, first the page reference, second the attribute list (struct for which is present in `lang/libs/page`)
third the text for the children, yes we pass children as `SsrText` (a struct in `lang/libs/page`, its like a `string_view`)

The server function does two things, it appends a js function that would perform hydration into the js bundle, It also appends the server side rendered html
to the html bundle.
A trick used by `react_cbi`, `solid_cbi` and `preact_cbi`, they actually capture this html and put it into the js bundle.

One very important thing to note:

The html (server rendered), does NOT contain attributes that use js expressions or for example js lambdas, because we cannot ssr them.
These skipped attributes are passed to the hydration function we generated (only the skipped attributes). The js function we generated
takes the element, skipped attributes as arguments.

#### Std Library Usage

We heavily use the `lang/libs/std` which provides us heavily used things like `std::string`, `std::string_view`, `std::vector`

## Issues

Trace universal issues across four layers, in this order:

1. Read the source component or page using `#universal`.
2. Read the generated output in the compiled package, especially `output/*.html`, `output/*.css`, and `output/*.js`.
3. Read the universal compiler pieces in `lang/libs/universal_cbi`.
4. Read `page.defaultUniversalSetup()` in `lang/libs/page/src/page.ch` before changing runtime behavior.

Do not assume the bug is in the component source. Many failures come from compiler output or hydration runtime behavior.

## Primary files

- `lang/libs/universal_cbi/src/converter/*`
- `lang/libs/universal_cbi/src/react/*`
- `lang/libs/page/src/page.ch`
- `lang/libs/components/src/*`
- `lang/compiled/*/output/*.html`
- `lang/compiled/*/output/*.css`
- `lang/compiled/*/output/*.js`

## Debug workflow

For interactive regressions, compare all three generated artifacts:

- In HTML, check the initial SSR state. Look for suspicious attrs such as `style=""`, `checked="null"`, duplicated attrs from prop spreading, or missing initial text/content.
- In JS, check whether state-derived props are emitted as reactive wrappers like `$_ucs(() => ...)` instead of one-time values.
- In CSS, check whether nested selectors compiled correctly. Universal CSS generation can accidentally introduce descendant spaces that change selector meaning.

If the user mentions dialog, tabs, toggles, or other interactivity, inspect generated output before editing source.

## Common failure modes

### Reactive props compiled as one-time values

Symptoms:

- Dialog closes or opens only after an extra click
- Tab content appends or leaves stale panels visible
- Button visibility changes but paired content does not
- Radio state text changes but visual state does not

What to check:

- `components.js` should emit state-dependent attrs and text as `$_ucs(() => ...)` when they depend on `state`.
- `convert_jsx_runtime_expr` and related JSX conversion paths must wrap state-derived expressions, not flatten them once.

Relevant files:

- `lang/libs/universal_cbi/src/converter/converter_core.ch`
- `lang/libs/universal_cbi/src/converter/converter_jsx.ch`
- `lang/libs/universal_cbi/src/react/jsx_props.ch`
- `lang/libs/universal_cbi/src/converter/converter_utils.ch`

### Subscriber mutation during notification

Symptoms:

- First click updates only part of the UI
- Second click updates the paired control
- Dialog button hides but dialog does not appear until another click
- Tabs partially update on each click

Cause:

- A state or computed-state subscriber unsubscribes/resubscribes while the runtime is iterating the same subscriber array.

Fix pattern:

- In `page.defaultUniversalSetup()`, snapshot subscriber arrays before notifying them.
- Apply this both to plain state (`$_us`) and computed state (`$_ucs`).

Relevant file:

- `lang/libs/page/src/page.ch`

### SSR attrs emitted incorrectly

Symptoms:

- `style=""` on elements that should be hidden or shown
- `checked="null"` in HTML
- Wrong initial tab or dialog state before hydration

Rules:

- Non-SSRable expressions should be skipped instead of degraded into bogus attrs.
- Simple state-derived expressions that can be resolved from known state initializers should be SSR-evaluated so initial DOM matches hydrated DOM.

Important:

- `current_func` on `JsConverter` is the server function, not the JS component AST.
- Do not read `current_func.body` expecting JS statements.
- If SSR evaluation needs state initial values, cache them explicitly during JS conversion.

Relevant files:

- `lang/libs/universal_cbi/src/converter/converter_utils.ch`
- `lang/libs/universal_cbi/src/converter/converter_base.ch`
- `lang/libs/universal_cbi/src/converter/converter_core.ch`

### Toggle visuals do not match state

Symptoms:

- Caption text changes but checkbox/switch/radio visuals stay static

What to check:

- Event handlers must reach the real `<input>`, not just the outer `<label>`.
- Do not blindly spread all props to the `<input>` if that duplicates `checked`, `name`, or other attrs in SSR output.
- Verify the compiled CSS selector meaning in `output/*.css`.

Known selector pitfall:

- Writing top-level `.chx-toggle-input[checked] + ...` can compile into `.chx-toggle-input [checked] + ...` with an unwanted descendant space.
- Prefer nesting under the base selector, e.g. inside `.chx-toggle-input { &[checked] + .chx-checkbox-box { ... } }`, then verify the generated CSS.

Relevant file:

- `lang/libs/components/src/Toggle.ch`

### Nested universal wrapper elements break icon styling

Symptoms:

- `IconButton` glyph looks off-center
- `Fab` shows a dark icon pill above the blue fab background
- Styling direct children is not enough

Cause:

- Universal hydration boundaries often wrap children in an extra node such as `<div id=...><span ...></span></div>`.

Fix pattern:

- Style both the immediate child and one nested descendant level when the component commonly receives another universal child such as `<Icon>`.
- Verify against generated HTML, not just source JSX.

Relevant files:

- `lang/libs/components/src/Button.ch`
- `lang/libs/components/src/Surface.ch`

## Practical checks by feature

### Dialog

Verify:

- Initial HTML has the correct visible/hidden state.
- `components.js` uses reactive `style` for both the dialog and the open button.
- Clicking once changes both controls.
- No bogus `style=""` remains unless it is intentionally empty.

### Tabs

Verify:

- Only one panel is initially visible in HTML.
- Clicking a tab updates tab button styling and panel visibility on the same click.
- Panels are not appended repeatedly due to partial hydration updates.

### Toggles

Verify:

- Generated HTML does not contain `checked="null"`.
- Event handlers are attached to the input that owns the `checked` attr.
- Generated CSS selectors still mean "same element has `[checked]`".

### IconButton and Fab

Verify:

- Generated HTML shape for the icon child.
- CSS neutralizes the nested icon background and border where needed.
- Glyph alignment is checked against compiled output, not inferred from source.

## Editing guidance

- Prefer fixing the smallest layer that explains the generated output.
- If output is wrong, do not only tweak the component source. Confirm whether the compiler or runtime is producing the wrong HTML/CSS/JS.
- When changing universal runtime behavior, treat it as cross-cutting. Re-read the generated output pattern first.
- If the user explicitly says not to compile or run tests, do not do so.

## Fast triage questions

Ask yourself:

- Is the initial HTML already wrong before hydration?
- Is the JS prop/text emission reactive or one-time?
- Is the runtime updating all subscribers in one click?
- Did CSS compilation change selector meaning?
- Did a nested hydration wrapper invalidate the styling assumption?
