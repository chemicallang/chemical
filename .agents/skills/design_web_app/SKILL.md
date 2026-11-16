---
name: Designing Web Apps in Chemical
description:
    How complex and scalable web apps are designed in chemical using macro plugins
---

Writing scalable and complex web apps require that you have great organization, components that handle their responsibilities
and the glue that holds everything together is small, predictable and reliable.

### Beginning the app

First you must ask yourself, whether the app is a static app (static pages website like a blog), a SPA (single page application) or 
a server served app (where server serves pages, modifying the actual content based on each user)

Everything happens with the `page` library, which you should import in the `chemical.mod`

#### Writing a single static page with styles

Here's simple functions that execute on the server, to create a single page with its assets into the current directory.

```chemical
func CreateOneSimplePage() {
    // comes from the page library
    var page = HtmlPage()
    
    // lets first put static content in the page
    PutStaticContentInSimplePage(page)
    
    // write the static page to the directory
    // you should definitely analyze the page library (lang/libs/page) for this function's definition
    // so you can understand how it emits the assets for the page
    // index.html + any assets it requires (js / css) will be written to current directory
    page.writeToDirectory("./", "index.html")
}
```

The `#css` value form returns a compiler-generated class you attach with `class={…}`:

```chemical
// returns a deterministic, compiler-generated class name (e.g. `.h23unfi3`)
// for the styles we wrote
func style_button(page : &mut HtmlPage) : *char {
    return #css {
        color : blue;
        padding : 8px;
        border-radius : 4px;
        background : white;
        // targeting a class name present in the entire page
        .my-global-button {
            color : red;        
        }
    }
}
```

#### `#css` in value position vs statement position

- **Value position** (`return #css { ... }`, `var s = #css { ... }`) — the macro's value is a
  compiler-generated class (`.hXXXXXX` for a plain block, `.rXXXXXX_` when the block has media
  queries, keyframes or Chemical values), and the block's rules are scoped under it. Attach the
  returned class to the element you want styled (`class={style_button(page)}`). Top-level
  declarations describe that element; nested rules without `&` are implicit descendants of it.
- **Statement position** (`#css { ... }` on its own line) — nothing can receive the generated
  class, so the block is emitted as a **global stylesheet**: the block's own declarations go to
  `:root`, `&` anchors to the document root, and nested rules, media queries and keyframes keep
  the selectors you wrote. This is the form used for page-wide CSS.
- Media queries are emitted **after** the block's other rules, so a responsive override wins
  over the base rule it overrides (the cascade breaks ties by source order).

#### `#css`, `#globalcss`, and shared assets

- `#css { ... }` and `#js { ... }` are **always part of the page's own bundle**. They pair
  with the `#html` you wrote in that page helper, so they are never pushed into a shared
  bundle — even when the page has one attached.
- `#globalcss { ... }` is **app-wide global CSS** (global selectors). When the page has a
  shared assets sink attached it is emitted there (served/cached once for all pages);
  without a sink it falls back to the page. Use it for themes, base styles, and shared
  header/footer chrome — not for page-specific layout.
- `#globaljs { ... }` is the JS analogue: page-independent JS emitted into the shared
  bundle (page fallback), **once per source location**. Use it for theme bootstrapping,
  analytics init, and shared helpers — not for JS that touches the current page's DOM.
- Component styling belongs in a `style { }` block inside the `#universal` component; that
  follows the component (shared sink when attached).

| Syntax | Scope | Bundle |
|---|---|---|
| `style { }` (in `#universal`) | component-scoped | follows the component |
| `#globalcss { }` | global selectors | shared sink first, page fallback |
| `#globaljs { }` | page-independent JS | shared sink first, page fallback (once) |
| `#css { }` / `#js { }` | global selectors / page JS | always the page |

#### `style { }` — component-scoped CSS

Inside a `#universal` component, a `style { … }` block is parsed by the CSS parser and
evaluates to the component's generated (deterministic) class name. Attach it with
`class={…}`; the rules are emitted once per component. This is the preferred place for
component CSS — it keeps the styles next to the markup they belong to.

```chemical
#universal Badge(props) {
    var badge = style {
        color: red;
        display: inline-flex;
        &:hover { color: blue; }
        @media (min-width: 600px) { padding: 8px; }
    }
    return <span class={badge}>{props.children}</span>
}
```

Multiple blocks compose (`class={a + " " + b}`), and all CSSOM features work
(nesting with `&`, media queries, `@keyframes`, Chemical `${…}` values).

#### Pairing CSS with `#html`

The rest of the example pairs page-local JS (`#js`) with `#html`. Use `#css` for CSS
that only this page's `#html` needs, and `#globalcss` for app-wide/reset/theme CSS.

```chemical
// when we open a {} for the attribute value, it means we are going to write chemical code inside those braces
// NOT JS, just chemical code that would execute on the server 
func PutStaticContentInSimplePage(page : &mut HtmlPage) {
    #js {
        const myElem = document.getElementById("clickable-btn")
        myElem.onclick = () => {
            console.log("you clicked on the button");
        }
    }
    #html {
        <div>
            <button class={style_button(page)}>Hello World</button>
            <button id="clickable-btn" class="my-global-button">This is targeted using global selector</button>
        </div>
    }
}
```

##### Benefits of this approach

- #css is parsed during compile time, class name is computed during compile time
- #html only emits static html, #html doesn't allow any JS
- Guarantees that emitted page is static

##### Limitations of this approach

- Hard to write JS
  - select element, have event listener, modify text (all of a simple state)
  - lambdas can't be given to onClick attribute of element (like React supports)
- Everything calculated at compile time or in the server, client side requires that manually handling events and DOM manipulation
- Components take server side props (because they are native functions)

#### Cannot split #html

You must not do this, an element that begins, must end in the same macro

```chemical
func InvalidHtml(page : &mut HtmlPage) {
    #html {
        <div>
    }
    
    // chemical statements in between
    if(true) {}
    
    #html {
      </div>
    }
}
```
Again do NOT write ^ such code.

Here's how to write correct code.

```chemical
func InvalidHtml(page : &mut HtmlPage) {
    #html {
        <div>
        @{if(true) {
            // i can write chemical code here
        }}
        </div>
    }
}
```

#### `@{}` escape syntax for dynamic content

Use `@{...}` to escape from JSX/HTML mode back to Chemical code inside `#html` blocks. Inside the escape, you can write any Chemical statement (loops, conditionals, variable declarations). Use nested `#html { }` blocks inside the escape to emit HTML elements.

**Loops (for/while):**
```chemical
var items = // ... array of data
var idx : size_t = 0
var count = items.size()

#html {
    <div class="grid-3">
        @{while(idx < count) {
            var item = items.get_ptr(idx)
            var name = // extract from item
            var id = // extract from item
            var link = std::string("/detail/")
            link.append_string(&cars_core::int_to_string(id))
            idx = idx + 1
            #html {
                <Card><CardBody><CardTitle>{name}</CardTitle>
                    <Button variant="outline" size="sm"><Link href={link}>View</Link></Button>
                </CardBody></Card>
            }
        }}
    </div>
}
```

**Conditionals (if/@else):**
```chemical
#html {
    <div>
        @{if(has_data) {
            <div class="grid-3">
                @{while(idx < count) {
                    // ... render items
                }}
            </div>
        } @else {
            <div>No data available</div>
        }}
    </div>
}
```

**Key rules:**
- You CANNOT split a `#html` block across multiple blocks with Chemical code in between
- Always use `@{}` to write Chemical logic inside `#html` blocks
- The index variable (`idx`) must be incremented BEFORE the nested `#html { }` block
- The loop variable extraction happens inside the `@{}` block, before the `#html { }`
- Each `@{}` block must be self-contained (all variables declared or accessible within it)

#### Styled components (`#styled`)

`#styled` (from `css_cbi`) lets you declare a reusable component whose styling is scoped to a compiler-generated hash class and automatically injected into the page CSS — no manual `#css` + `class={...}` wiring needed.

```chemical
#styled Card("div") {
    background: #ffffff;
    border: 1px solid #cccccc;
    padding: 8px;
}

#styled Title(.div) {
    font-weight: bold;
    color: #333333;
}
```

Use them directly in `#html` like any component:

```chemical
#html {
    <Card class="xl">Hello <Title>World</Title></Card>
}
```

This renders `<div class="hAz5DrX xl">Hello <div class="hAStij2">World</div></div>` and injects both components' CSS (`.hAz5DrX{...}` and `.hAStij2{...}`) into the page. The generated classes are content hashes of the CSS, so they are deterministic.

Wrap mode composes components while merging styles:

```chemical
#styled Wrapped(Title) {
    margin: 4px;
}
#html {
    <Wrapped>wrapped content</Wrapped>
}
```

`Wrapped` forwards to `Title` and merges its own generated class with `Title`'s onto the single rendered element, so both `margin` and `font-weight` apply.

Notes:
- User-provided `class` attributes are preserved and merged with the generated hash class.
- `#styled` components are pure SSR + injected CSS — no hydration/JS, unlike `#universal`.
- They are usable cross-module: declare in one module, import it, and use `<Name>` in another module's `#html`.

#### Components

To better provide support for JS handling of elements, applying event listeners and DOM manipulation, universal components come into play.

What if instead of server side functions, we wrote components that can be used at client side or server side ?

```chemical
// this function returns a random class name for the styles we wrote
// the random class name would be like .h23unfi3
func style_button(page : &mut HtmlPage) : *char {
    return #css {
        color : blue;
        padding : 8px;
        border-radius : 4px;
        background : white;
        // targeting a class name present in the entire page
        .my-global-button {
            color : red;        
        }
    }
}
#universal MyButton(props) {
    // page is always available inside a universal component
    var server_value = ${style_button(page)}
    state counter = 0
    var arr = ["first", "second"]
    // the .map on arr would break ssr, meaning it would be rendered at client side
    // everything else would render using ssr + hydration
    return <div>
        <button onClick={() => {
            console.log("you pressed my button")
            counter++
        }}>{counter} : {props.text}</button>
        arr.map((i) => {
            <span>{i}</span>
        })
    </div>
}
func PutStaticContentInSimplePage(page : &mut HtmlPage) {
    // you can mount universal components in #html or other #universal components
    // remember in #html, you can't pass js props, if you want to do that, wrap in a universal component
    #html {
        <div>
            <MyButton text="This is my button" />
            <MyButton text="This is my button" />
        </div>
    }
}
```

in `universal` macro, some things are different

- braces `{}` do not mean a server side (chemical) value, braces contain JS expressions
- dollar braces `${}` contain server side chemical values

#### Benefits of using #universal components

- Automatic hydration support
- Lambdas in attributes support (like onClick)
- Supports state for reactive expressions
- Easily manipulate DOM using JS expressions

#### Drawbacks of using #universal components

- No guarantee of static content being generated, something can break SSR + hydration (like .map)
- Not explicitly clear about what gets generated for the runtime code

### Typed props

An untyped `#universal Name(props)` accepts any attribute, so a typo (`counts=` instead of
`count=`) reaches the browser as `undefined` and the bug shows up at runtime. Declare the prop
surface with a `props :` type instead:

```chemical
public struct CardProps {
    var title : std::string_view
    var count : int
}

#universal Card(props : CardProps) {
    return <div class="card">{props.title}</div>
}

// ok
#html { <Card title="first" count={1} /> }

// compile error: unknown prop 'counts' for component 'Card'; declared props: title, count
#html { <Card title="first" counts={1} /> }
```

Rules:

- The type must be a struct whose fields are the props, and every declared field is **required**
  (there are no optional props or defaults yet — give the field a sentinel value or split the
  component).
- Implicit props are always allowed even when not declared: `children`, `class`/`className`, and
  `on*` event handlers.
- `props.title` type-checks as `std::string_view` (etc.), so the body fails to compile if you treat
  a declared `int` as a string — typed props make the component body typed too.
- Adding or renaming a prop is a compile error at every call site, which is why the goal is *all*
  components typed (a grep-based count is a release gate).

## Sharing JS/CSS across pages

If several pages use the same components, render them through **one shared sink** so the hydration
runtime, component definitions, and component classes are written **once** into a shared, cacheable
bundle instead of once per page. Each page keeps only its own dispatch statements and SSR HTML.

Measured on two pages that instantiate the same components (`scripts/static-site-test.sh`):

| | per-page HTML | inline `<script>` | shared assets |
|---|---|---|---|
| every page standalone (`page.toString()`) | 109,317 B **each** | 2 per page | none |
| one shared bundle | **424 B / 409 B** | **0** | 1 bundle (108,816 B) + 30 B CSS, fetched once |

### `StaticSite` — the recommended way

`StaticSite` does the whole job: one bundle with a content-hashed name, one HTML file per page, and a
content-hashed script chunk per page so the HTML ships **no inline `<script>`** (CSP-clean).

```chemical
public func main() : int {
    // out dir, bundle base name, URL prefix the bundle is served under
    var site = StaticSite.make("out", "app", "assets")

    var index = HtmlPage()
    site.add(&mut index, "index.html")   // attaches the sink: render AFTER this
    index.defaultUniversalSetup()
    render_index(&mut index)

    var about = HtmlPage()
    site.add(&mut about, "about/index.html")   // nested routes get ../assets/…
    about.defaultUniversalSetup()
    render_about(&mut about)

    if(!site.finish()) {                  // build + write; false => site.error_message()
        unsafe { printf("%s\n", site.error_message().data()) }
        return 1
    }
    return 0
}
```

Output: `out/index.html`, `out/about/index.html`, `out/assets/app.<hash>.js`,
`out/assets/app.<hash>.css`, and one `out/assets/<page>.<hash>.js` per distinct page script. A page
that emits the same dispatch as another shares its chunk file.

Two rules:

- **Attach before rendering.** `site.add(&mut page, path)` attaches the sink; anything page-independent
  the page emits afterwards lands in the bundle. A page rendered *after* the build would change the
  bundle's bytes after its name was fixed — `finish()` reports that instead of publishing it.
- **Pages and paths must outlive the build.** The site holds a pointer to each page and a view of
  each path (it never copies a page); pass `&mut page` on a local you keep alive until `finish()`.

And one guard worth knowing about: a page that renders components must have called
`defaultUniversalSetup()` first, because that is what writes the hydration runtime into the bundle.
Without it the page ships `$__uni_c(...)` calls and nothing defines `$__uni_c` — a browser TypeError
nobody sees until production — so `build()` refuses it:
`a page hydrates components but the shared bundle has no hydration runtime; call
defaultUniversalSetup() on every page before rendering it`.

### Serving from memory

The same build can answer requests without touching the filesystem:

```chemical
if(!site.build()) { /* error_message() */ }      // freeze + render, no disk access
var res = site.respond("/about/")                // SiteResponse
// res.status (200 / 404 / 503 before build), res.content_type,
// res.cache_control ("no-cache" for HTML, immutable for hashed assets),
// res.etag (strong validator), res.body (std::string_view into the site — no copy per request)

// Conditional request: pass the request's If-None-Match. A match is a 304 with
// an empty body, so the browser keeps its copy and downloads nothing.
var again = site.respond_if_none_match("/about/", res.etag)
if(again.not_modified()) { /* 304: no body, same ETag + cache policy */ }
```

`/`, `/index.html`, `/about/` and `/about` all resolve to their page; anything else is a 404 page, and
`..` traversal is refused. The bundle's ETag is the same hash that is in its file name, so an immutable
asset revalidates without re-hashing. `artifact_count()` / `artifact_path(i)` / `artifact_bytes(i)`
expose the build's manifest (pages, bundle, chunks).

### Serving it over HTTP

`webserve` (`lang/libs/webserve`) is the bridge from a built site to `http::Server`. It imports both
`http` and `page`, so neither library depends on the other and only an app that serves pays for the
pair:

```chemical
var site = StaticSite.make("out", "app", "assets")
// ... add pages, render ...
if(!site.build()) { /* site.error_message() */ }

unsafe { webserve.install(&raw mut site) }        // the site must outlive the server
var srv = http::Server.bind(cfg)                  // Result<Server, net::NetError>
srv.router.add("GET", "/:path*", ||(req) => {
    return webserve.respond_to_request(req)
})
srv.serve()
```

Two `web::Router` details that bite every author once: the catch-all pattern is `/:name*`
(**not** `/*name`), and the handler must be a **lambda** — a bare function reference
(`router.add("GET", "/:path*", webserve.respond_to_request)`) is rejected with
`SAFE-SEND-002: cannot prove this callback is Send + 'static: its capture witness was erased`.

`respond_to_request` maps the `SiteResponse` onto an `http::Response`: status, `Content-Type`,
`Cache-Control`, `ETag`, `If-None-Match` → 304, and the body. A request before a site is installed is
a 503, not a crash. The body is copied into the response (one copy per request) because
`http::Response` owns its body; keep that in mind for very large bundles.

### `SharedAssets` — the low-level primitive

`StaticSite` is a thin owner around `SharedAssets`, which you can still drive yourself when you need
custom naming or serving:

```chemical
var shared = shared_assets()          // or SharedAssetsHandle.make("app")

var home = HtmlPage()
home.attach_shared(shared)
BuildHome(&mut home)

var about = HtmlPage()
about.attach_shared(shared)
BuildAbout(&mut about)

// render every page first, then take the output out:
shared.write_to("output", "app")      // output/app.js + output/app.css — serve + cache these
// per page: home.getHtml() (SSR) + home.local_js() (dispatch statements)
```

`attach_shared` is opt-in and low level: it does not manage routes, hashed names or serving. A page
that never calls it behaves exactly as before. App-wide CSS/JS written with `#globalcss { ... }` /
`#globaljs { ... }` goes into the shared bundle when a sink is attached (falling back to the page
otherwise), while `#css`/`#js` always stay on the page. See the `universal` skill section "Shared
JS/CSS bundles across pages (`SharedAssets`)" for the routing rules and gotchas.

## Testing your components

Write component tests next to the app with `#universal_test`. Each test declares
an inline fixture (rendered with the same production SSR + hydration pipeline as
a real page) and a raw JS `<script>` of steps run in a real WebView:

```chemical
#universal_test("counter increments") {
    <Counter start={0} />
    <script>
        expect($('[data-testid=count]').text()).toBe('Count: 0')
        $('[data-testid=inc]').click()
        expect($('[data-testid=count]').text()).toBe('Count: 1')
    </script>
}
```

Run with `./scripts/test.sh --tcc --universal`. See the `universal_testing`
skill for the full API and gotchas; for headless/cross-browser coverage use the
`components_e2e` Playwright suite instead.

## What exists today vs what the web-stack plan adds

Everything documented above works today. `lang/docs/web-stack-plan.md` adds a lot more, and **none
of the right-hand column has landed yet** — code written against it will not compile:

| capability | today | planned (not available yet) |
|---|---|---|
| app shape | one module: `chemical.mod` + `#html` / `#css` / `#js` / `#universal` | `shared` / `server` / `client` modules, `--web`, `chemical serve` + a dev overlay |
| assembling pages | `HtmlPage` + `StaticSite` (one shared, hashed bundle) | `web::App` (`app.page("/", Home)`), `web::Bundle`, `web::Shell` + slots |
| serving | `StaticSite.build()` + `respond(path)` / `respond_if_none_match`, served over `http::Server` through `webserve` | `web::App.handle(req)`, route table + per-route chunks, three delivery modes (static / shell+slots / full SSR) |
| client JS | `#js` (raw), `#universal` (hydration, `state`, event handlers) | typed `dom`, `web::Client` completion handlers |
| JS output | inlined page script, or one shared bundle + per-page chunk | browser-target `--emit-js`, an ESM chunk graph, minification, `--static` |
| props | untyped `(props)`, or typed `(props : Props)` | optional props / defaults (`@props`) |

Escape hatches while a typed surface is missing: `#js` blocks (raw JS, still inside the page's or
bundle's JS) and `@{}` escapes for server-side logic inside `#html`. Neither breaks the probes.

### Performance rules that hold today

The probe (`./scripts/web-probe.sh analyze <out-dir> --strict`) measures each of these:

- Component definitions are per component, **not** per instance — but a page that instantiates many
  components has a big bundle, and pages only share those bytes through one sink (`StaticSite`).
- Dispatch text is per **instance**: the two-page fixture's page chunk is 137 B, 105 B of which is
  the two `$__uni_c` calls. Keeping dispatch out of the bundle is what makes a page chunk
  route-cacheable.
- The hydration runtime dominates the shared bundle (~108 KiB for two trivial components), which is
  why it is hashed, served once, and cached immutably instead of inlined per page.
- Minification is worth 8–14% of raw generated JS (measured); it is the emitter's job, so do not
  hand-minify pages.

### Bootstrap prompt for an app agent

Hand an agent this sentence before it writes code:

> Read the `design_web_app` (web stack), `universal`, and `universal_testing` skills before writing
> any code: this app renders pages with `#html` / `#css` / `#universal`, types every component's
> props, serves them through one `page.StaticSite` shared bundle, and keeps
> `scripts/web-probe.sh --strict` green.

When the plan's Phases 3–4 land (module split, `web::App`/`Bundle`/`Shell`, minification), use the
longer sentence in `lang/docs/web-stack-plan.md` §3.21 instead.
