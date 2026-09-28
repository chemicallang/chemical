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
## Sharing JS/CSS across pages

If several pages use the same components, render them through one `SharedAssets`
sink so the hydration runtime, component definitions, and component classes are
written **once** into a shared, cacheable bundle instead of once per page. Each
page keeps only its own dispatch statements and SSR HTML.

```chemical
var shared = shared_assets()

var home = HtmlPage()
home.attach_shared(shared)
BuildHome(&mut home)

var about = HtmlPage()
about.attach_shared(shared)
BuildAbout(&mut about)

// Render every page first, then take the output out:
shared.write_to("output", "app")   // output/app.js + output/app.css — serve + cache these
// per page: home.getHtml() (SSR) + home.local_js() (dispatch statements)
```

This is opt-in and low level: it does not manage routes or serving. A page that
never calls `attach_shared()` behaves exactly as before. App-wide CSS/JS written
with `#globalcss { ... }` / `#globaljs { ... }` goes into the shared bundle when a
sink is attached (falling back to the page otherwise), while `#css`/`#js` always
stay on the page. See the `universal` skill section "Shared JS/CSS bundles across
pages (`SharedAssets`)" for the routing rules and gotchas.

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
