# Building a web app

This is the short, verified path from a clean checkout to a running site. It
describes what works **today**; the fuller target architecture is
[`web-stack-plan.md`](web-stack-plan.md), and the parts of it that are not
available yet are listed at the end so nothing here surprises you.

## 1. Build the compiler

```bash
./scripts/setup.sh                 # libtcc + submodules
./scripts/configure.sh --no-llvm   # TCCCompiler only (the web stack needs no LLVM)
./scripts/build.sh --tcc -j "$(nproc)"
```

## 2. Build and run the starter

The starter is a working app at `templates/web-starter/`, not a snippet — copy it
somewhere and edit it:

```bash
cmake-build-debug/TCCCompiler templates/web-starter/chemical.mod \
    -o lang/compiled/web-starter --mode debug_quick
./lang/compiled/web-starter
# → wrote lang/compiled/web-starter-output/index.html and about.html
```

That writes:

| file | what it is |
|---|---|
| `index.html`, `about.html` | server-rendered markup, with hydration markers (`id="u1"`) |
| `index.js` | the component definitions the page needs |
| `index_head.js` | the small head runtime |
| `index.css` | the page's compiled CSS |

Open `index.html` directly, or serve the directory with anything, e.g.
`cd lang/compiled/web-starter-output && python3 -m http.server 8000` (a server is
nicer once you add anything that fetches).

## 3. The four things to learn

```chemical
// 1. Static markup. A page made only of `#html` ships 0 bytes of JavaScript.
#html {
    <main class="page">
        <h1>Hello</h1>
    </main>
}

// 2. A component: rendered on the server AND hydrated in the browser from this
//    one source. `state` is reactive in the browser.
public struct CounterProps {
    var label : std::string_view = "Clicked"   // every field has a default, so
    var step : int = 1                          // call sites may omit any of them
}

#universal Counter(props : CounterProps) {
    state n = 0
    return <div>
        <button onClick={() => { n = n + props.step }}>{props.label}</button>
        <span>{n}</span>
    </div>
}
```

- **Typed props** (`props : CounterProps`) make a typo a compile error instead of
  `undefined` in the browser: `<Counter labl="x" />` fails with
  `unknown prop 'labl' for component 'Counter'; declared props: label, step`.
  A declared field with an initializer may be omitted, and the default reaches
  both the server markup and the client payload.
- **Implicit props** are always accepted by *any* component, typed or not:
  `children`, `class`/`className`, `style`, `key`, `ref`, `on*`, `data-*`,
  every `aria-*`, and the universal DOM attributes `id`, `title`, `role`,
  `tabindex`, `hidden`, `dir`, `lang`, `slot`, `part`, `contenteditable`,
  `draggable`, `spellcheck`, `inert`, `translate`, `autofocus`. That is what lets
  a component keep `{...props}` and forward `role`/`aria-label` while still
  type-checking its own knobs. A *declared* prop of the same name is still
  value-checked, so `id={3}` on a component declaring `id : string_view` fails.

Head metadata is typed rather than ad-hoc strings:

```chemical
page.set_head(Head { title: "Home", description: "…", canonical: "…", noindex: false })
```

Every field is optional; a second call **replaces** the block (a layout sets a
default, a page overrides it).

## 4. Multiple pages

`page.writeToDirectory(dir, name)` writes `<name>.html` and its assets. It
creates only the directory it is given, so pass a flat path — nested routes are
`StaticSite`'s job, not `writeToDirectory`'s.

## 5. The component library

`lang/libs/components/` is a shadcn-style library (Card, Button, Badge, Alert,
Avatar, Input, Sheet, Typography, …). Add `import components` to
`chemical.mod` and use it like any other component:

```chemical
#html { <Card><CardHeader><CardTitle>Hello</CardTitle></CardHeader></Card> }
```

Props are typed on 166 of the 184 components. That includes everything you need
to build a page today: Card, Button (+ every variant/size wrapper), Badge, Alert,
Avatar, Separator, ErrorOverlay, Input/TextArea/NativeSelect (+ the `Field` family,
`InputGroup`/`InputIcon`/`InputAddon` and their wrappers), all of Typography,
Surface (Paper, AppBar, Drawer, Menu, Popover/Dialog/Tooltip/Dropdown slots,
Snackbar, Icon, EmptyState, StatCard), Data (Progress, Accordion, TabList/Tab/
TabPanel, Table, List, PageItem), Utilities (Container, Stack, Grid, Skeleton,
Spinner, Divider, Kbd, Breadcrumbs) and the item components of Sheet, Toast,
Collapsible, ToggleGroup, RadioGroup and Select.

The remaining 18 accept any attribute name, exactly as before, so nothing breaks.
They fall into two groups: components whose props are a controlled/uncontrolled
pair (`Sheet`, `Collapsible`, `Toast`, `Select`, `Slider`, `ToggleGroup`,
`RadioGroup`, `Checkbox`, `Radio`, `Switch` — `open`/`value`/`checked` must be
able to stay absent, which a declared default or a required field cannot express),
and components that take a JSX element or array (`Popover`/`Dialog`/`Dropdown`/
`Tooltip` triggers, `AccordionItem`'s trigger, `Suspense`'s fallback, `Tabs`,
`Pagination`). Until then those accept any prop name and are still checked at
runtime by the browser, not by the compiler.

Two things to know when you call a typed component:

- **Omitted props take the declared default on both server and client.** You do
  not need to repeat a default at the call site.
- **A default may leave the attribute off the DOM entirely.** Optional text props
  that the browser treats specially (`placeholder`, `value`, `href`, `id`,
  `name`, `aria-label`) are absent unless you pass them, so an input stays
  uncontrolled when you omit `value`. When you *do* pass `value=""` deliberately,
  it is treated as "unset" — pass a real value to make the input controlled.

### If a build fails with `unresolved reference to 'compiler_VecRef__…'`

Your build directory holds CBI plugin objects (`<build>/universal.dir/object_tcc.o`)
compiled from an older version of the component library; a plain build does not
revalidate them against the plugin sources. Delete the build directory (or build with
`--no-cache`) after changing library or plugin code.

## What is not available yet

These are planned, not shipped — do not design around them:

- **`chemical serve` / a dev server.** You run a build script and serve the
  output yourself.
- **Declaring the app once and deriving client/server bundles** (`web::App`,
  `web_build`, chunk graph). The client/server split is done by hand today.
- **Writing client logic in Chemical.** Client behaviour lives in `#universal`
  component bodies (and `#js` escape hatches); the JS backend emits ES modules
  for whole programs but is not wired into the bundle graph.
- **Production packaging**: minification, `--static` multi-page export,
  sessions/CSRF, gzip precompression, size budgets.

## Known sharp edge

A **method call on a prop inside a universal JSX expression** renders nothing,
silently:

```chemical
{props.text.size() > 0 ? <b>x</b> : null}   // renders EMPTY
{props.text ? <b>x</b> : null}              // correct ("" is falsy)
{props.text != "" ? <b>x</b> : null}        // correct
```

The compiler reports this as a warning with the element's location, so
`--mode debug_quick` builds will show it. Compute the value into a local first
(`var n = props.text.size()` then `{n}`) and it works.
