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

### Mixed sites: pre-rendered pages and per-request routes

Real sites are rarely all-static: `/` and `/pricing` can be pre-rendered while `/account` and
`/invoices/:id` must be rendered per request. `StaticSite` expresses both in **one build** over one
bundle — a site is a route table, not a mode.

```chemical
// 1. Warm pass, BEFORE build(): render every per-request route once, discard the page.
var warm = HtmlPage()
site.attach_dynamic(&mut warm)
warm.defaultUniversalSetup()
render_account(&mut warm, std::string_view("warm"))

if(!site.build()) { /* site.error_message() */ }

// 2. Per request: a fresh page, the same sink, the site's frozen head tags.
var page = HtmlPage()
site.attach_dynamic(&mut page)
page.defaultUniversalSetup()
render_account(&mut page, session_user_name)
var html = site.render_dynamic(&mut page)      // "" means refused: dynamic_error_message()
var res = site.dynamic_response(&html)         // 200 text/html, no-store, no ETag
```

- **`attach_dynamic(&mut page)`** attaches the site's sink, so the runtime, component definitions and
  global CSS the bundle already carries contribute **zero bytes** to the page. What is left is this
  request's SSR markup plus its dispatch statements — inline, because they are request-scoped.
- **`render_dynamic`** returns the complete document with the site's frozen `<script src>`/`<link>`,
  and **refuses** (empty string, reason in `dynamic_error_message()`) when the route needs a component
  the frozen bundle does not contain. That refusal is the point of the warm pass: the warm render is
  what puts a dynamic-only component into the bundle. The alternative — appending it at request time —
  would leave a published, content-hashed URL naming bytes it no longer holds, i.e. HTML whose
  hydration silently misses a definition.
- **`dynamic_response`** is `no-store` with **no ETag**: the same URL now returns different bytes, so
  there is nothing to validate. Pass `"private, no-cache"` only when a render is genuinely stable per
  URL. An empty document is a **503**, not an empty 200.
- **Pre-rendered routes are unaffected**: `no-cache` + ETag + the `respond_if_none_match` 304 path.
  Dynamic routes are not artifacts — `artifact_count()` does not change and nothing is written for
  them — so `finish()` / a static export covers exactly the routes you pre-rendered.

Serving both kinds over HTTP is the app's own dispatch, because a dynamic route needs a render
callback that `webserve` cannot invent:

```chemical
srv.router.add("GET", "/:path*", ||(req) => {
    if(req.path == "/account") {          // a per-request route
        var page = HtmlPage()
        site.attach_dynamic(&mut page)
        page.defaultUniversalSetup()
        render_account(&mut page, whoami(req))
        var html = site.render_dynamic(&mut page)
        return html_response(site.dynamic_response(&html))
    }
    return webserve.respond_to_request(req)   // pre-rendered routes + assets
})
```

What `StaticSite` deliberately does not do: match route patterns, pick a cache policy per route, or
model redirects/404. That is `web::App` (below).

### `web::App` — the route table (use this for a real app)

`lang/libs/web` (`web::App`) owns the route table and does the dispatch `StaticSite` leaves to you.
Each route declares **when** its HTML is made; the app builds one bundle for all of them.

```chemical
public func Home(page : &mut HtmlPage, req : &http::Request) { … }
public func Account(page : &mut HtmlPage, req : &http::Request) { … }

var app = web::App.make("dist", "app", "assets")
app.page("/", Home)                  // pre-rendered at build: no-cache + ETag + 304
app.page("/pricing", Pricing)
app.live("/account/{id}", Account)   // per request: no-store, request data
app.live("/files/{path*}", Files)
app.redirect("/prices", "/pricing")  // 308 by default
app.not_found(Missing)               // rendered per request, warmed with the bundle
app.set_base_path("/app")            // optional: serve the whole app under a mount
app.export_sitemap("https://example.com", true)   // sitemap.xml + robots.txt, before build()

if(!app.finish()) { /* app.error_message() */ }  // build() + write the exportable routes

unsafe { web::install(&raw mut app) }
srv.router.add("GET", "/:path*", ||(req) => web::serve_request(req))
```

- **Route handlers are `(page : &mut HtmlPage, req : &http::Request) => void`** — the same shape for a
  pre-rendered and a per-request route. `page` must be named `page` for `#html`; `req` gives you
  `req.param("id")`, `req.query`, headers.
- **`page()` patterns must be concrete.** `{param}` in a pre-rendered route is a build error (there is
  no value at build time); use `live()`.
- **`live()` warms automatically.** `build()` renders each live route once into a discarded page, so
  its components join the frozen bundle. A live route that renders a component the bundle does not
  carry is answered **503** with the reason, never HTML that cannot hydrate.
- **`handle(&mut req)` fills `req.route_params`** for the matched pattern (after which `req.param`
  works), then dispatches: redirects → live routes → pre-rendered pages/assets → the 404 route.
- **Cache policy is per route**: pre-rendered is `no-cache` + ETag (304 on `If-None-Match`); a live
  route is `no-store` with no validator unless you pass `cache_control` (and `vary`, e.g.
  `"accept-language"`).
- **`finish()` writes only the pre-rendered routes** (plus sitemap/robots). A per-request route is not
  an artifact — a static export of an app with live routes is exactly the exportable subset.

### Sessions, cookies and CSRF

`http` has the cookie layer and `web` the session layer; neither keeps global state.

```chemical
var jar = http.CookieJar.of(&req)                 // parses every Cookie header
if(jar.get("theme") is Option.Some) { … }

res.cookie("theme", "dark", http.CookieOptions.make("/", "Lax", true, true, 86400))
res.delete_cookie("theme", http.CookieOptions.make())

var store = web::SessionStore.make()              // one per process
var session = store.create()                      // 48-hex id, record created on first write
var existing = store.load(&req)
if(existing is Option.Some) {
    var Some(s) = existing else unsafe { unreachable }
    session = s
}
store.set(&session, "user", "ada")
var token = store.csrf_token(&session)            // render into <input type="hidden" name="_csrf">
store.attach(&mut res, &session)                  // Set-Cookie: session=<id>; Path=/; SameSite=Lax; HttpOnly
// on a POST:
if(!store.verify_csrf(&session, submitted_token)) { /* 403 */ }
store.destroy(&mut res, &session)                 // drops the server record and expires the cookie
```

Defaults are the safe ones: `HttpOnly`, `SameSite=Lax`, `Path=/`, no `Secure` (pass `secure: true` /
`store.set_secure(true)` in production — `http://localhost` needs the default). Cookie names and
values that cannot be serialised produce **no** header rather than a broken one. CSRF comparison is
constant time. The store is in-memory (process lifetime) and the interface is what a persistent store
implements.

### Client modules and typed `dom`

Browser code lives in a `client` module and is compiled to JavaScript by
`chemical build --web` (one application, two jobs: `shared` + `client` → JS,
`shared` + `server` + `src` → native). Reach for the typed `dom` module instead of
`#js` — a typo is a compile error instead of a blank page:

```chemical
// client/chemical.mod
module client
source "."
import std
import dom
import "../shared"          // pure logic only: no dom, no page, no platform

// client/counter.ch
public func on_click(e : &dom::Event) {
    e.prevent_default()
    var doc = dom.document()
    var node = doc.get_element_by_id("count")
    if(node is Option.Some) {
        var Some(label) = node else unsafe { unreachable }
        var next = shared_next(shared_parse_int(label.text().to_view()))
        label.set_text(shared_label(next).to_view())
    }
}

public func main() : int {
    var doc = dom.document()
    var button = doc.get_element_by_id("count-button")
    if(button is Option.Some) {
        var Some(el) = button else unsafe { unreachable }
        el.add_event_listener("click", on_click)
    }
    dom::fetch_text("/api/count", on_loaded)   // (status : int, body : &string_view)
    return 0
}
```

- **Handles, not pointers.** `Document`/`Element`/`Event` hold an opaque id into a host
  table the JS runtime owns. Copies are free, there is nothing to free, and a stale
  handle never aliases a different element. On the native side `dom` has no
  implementation: import it from a `client` module only.
- **The split is enforced by the module set**, not by annotations: the client pass
  compiles `shared` + `client`, the server pass `shared` + `server` + `src`, each with
  its own build directory. Calling a native-only symbol from `shared` or `client` fails
  the client pass by name and source location (`web-split-test.sh`).
- **v1 surface:** `document()`, `Document.get_element_by_id`/`query_selector`/
  `create_element`/`body`, `Element` text/`set_html`/attributes/`value`/`append_child`/
  `remove`/`focus`/`add_event_listener`/`query_selector`, `Event.prevent_default`/
  `stop_propagation`/`target`/`key`, `fetch_text`, `storage_get`/`storage_set`/
  `storage_remove`, `url_origin`/`url_pathname`/`url_search_param`. Raw host bindings
  live under `dom::raw`; canvas, uploads, pointer/rAF and `WebSocket` are later work.
- **`set_html` is trusted-markup only** — it parses HTML and never escapes; use
  `set_text` for anything that came from a user.
- Debugging: the client job always emits a source map (`web-client.mjs.map`), so a
  browser stack frame maps back to the `.ch` line. `scripts/web-dom-test.sh` runs the
  same fixture headlessly under Node with a small DOM harness
  (`scripts/js_backend/dom_harness.mjs`) — the fastest way to see a `dom` change work
  without a browser.

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
| serving | **`web::App`** (route table: `page`/`live`/`redirect`/`not_found`, `{id}`/`{path*}`, per-route cache policy, mount path, `install` + `serve_request`), `StaticSite.build()` + `respond(path)` / `respond_if_none_match` for pre-rendered routes, per-request routes (`attach_dynamic` + `render_dynamic` + `dynamic_response`, sharing the same bundle), `webserve` bridge | per-route chunks, shell+slots (byte-range) delivery, `Vary: Cookie` render hook |
| sessions | **`http.CookieJar` / `CookieOptions` / `Response.cookie`, `web::SessionStore` + constant-time CSRF** (in-memory store) | persistent stores, rotating session ids on privilege change, signed cookies |
| client / `dom` | **`--web` two-job build (`shared`/`server`/`client`), typed `dom` slice** (document, element, event, `fetch`, `localStorage`, `URL`) run headlessly by `web-dom-test.sh`; source maps; native-symbol-in-client diagnostic | canvas, uploads, pointer/rAF, `WebSocket`, `chemical serve` + dev overlay, browser E2E of `dom` |
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
