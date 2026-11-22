# Web stack: what five probe apps found (2026-11-21)

This is the survey the plan (`web-stack-plan.md`) asks for before the v1 API freeze: build real
shapes, measure them, and record what the **public** API makes an app author write by hand. Every
finding below came from writing an app against the documented API only — no private functions, no
compiler changes to make a probe pass.

The probes live in `lang/tests/web/probes/` and are built/run by `scripts/web-probes-test.sh`:

| probe | shape | what it exercises |
|---|---|---|
| `shop` | storefront | frozen pages + per-request renders + shell, sessions, CSRF, POST mutations, PRG redirects, a JSON endpoint, a search page (user input in HTML) |
| `blog` | content site | 47 pre-rendered pages from markdown, tag pages, one shared interactive island, per-route chunks |
| `spa` | single-page app | `--web` two-job split, a shell page, a JSON endpoint, client-side rendering and navigation in typed `client/*.ch` |
| `tasks` | authenticated multi-user app | `web::App` with `page`/`live`/`shell`/`endpoint` on shared paths, per-user sessions, a form POST + 303, per-user lists, `ctx.form`, CSRF |
| `docs` | documentation site | 43 generated pages with a sidebar and prev/next, sections, sitemap/robots, a JSON search index, gzip |

## Measured

Numbers are from this machine, TCC backend, `--mode debug_quick`, minified + gzip where the probe
enables it. They are the baseline the plan's budgets should be written against.

### `shop` (25 checks green)

| measurement | value |
|---|---|
| artifacts written | 8 (2 pages, bundle, css, 2 per-page chunks, sitemap, robots) |
| shared bundle | 48,283 B (11,051 B gzip) — **on pages with zero components** |
| per-page chunk | 28 B × 2 (`index.<hash>.js`) |
| `/` (frozen) | 432 B (309 B gzip), `no-cache` + ETag |
| `/products` (frozen) | 715 B |
| `/product/{slug}` (per request) | 580 B, `no-store` |
| dynamic render cost | 5,500 renders (5,000 product + 500 cart) in **2.03 s**, one thread, RSS 7.5 MB → **~0.36 ms/render** |
| build + write | included in the same 2.03 s run |

### `blog` (47 pages)

| measurement | value |
|---|---|
| build + write + sitemap | **0.16 s**, RSS 3.8 MB |
| output | 205,327 B total, 100 artifacts |
| HTML | avg 1,835 B, max 5,229 B, 86,284 B total |
| shared bundle | 48,283 B (11,051 gzip) + vendor 369 B |
| per-route chunks | 40 post pages get **40 separate 28-byte files with the same content hash** (`post-NN_index.14afcbcb6b1a.js`) |
| script tags on a post page | **0** (a page that uses no component correctly ships no JS) |

### `spa` (`--web`, 10 checks green)

| measurement | value |
|---|---|
| shell page | 810 B |
| JSON endpoint | 130 B |
| client job output | `web-client.mjs` 217,182 B (27,866 gzip) + `runtime.mjs` 27,586 B (8,060 gzip) + 778 B entry, unminified |
| shell references the client bundle? | **yes** (third pass) — `assets/client/boot.mjs` is a module script in the `<head>` and the bundle is served under `assets/client/`; before that the page loaded nothing that runs the client job |

## Bugs found and fixed here

1. **`@{ ... }` could not span lines.** The documented templating escape worked only when the whole
   block was on one line: a newline right after `@{` left a `NewLine` token where the statement
   parser expected a statement, so `@{\n while(...) {...}\n}` failed to parse while
   `@{while(...) {...}}` worked. Every documented example (AGENTS.md, skills, `web::Shell` docs)
   is multi-line. Fixed in `lang/libs/html_parser/src/parser/htmlchild.ch` (`skip_newlines` at the
   statement and closing-brace positions); `./scripts/test.sh --tcc --plugins` is green (1,315).

## API gaps, in priority order

### P0 — the API cannot express the app, or expresses it unsafely

**1. `web::App` has no method-aware or `Response`-returning route.**
`App.handle` dispatches on *path* only; a `PageHandler` can only write HTML, so it cannot answer
`303`, `403`, `405`, `409`, or JSON. A shop cannot accept a POST at all. Both probes had to put a
hand-written table in front of `app.handle` and re-implement redirect/JSON/error responses; the
helpers that would do it (`redirect_response`, `error_response`) are private to `web`.
*Landed (2026-11-21).* `App.endpoint(method, pattern, handler)` takes a
`(req: &http::Request) => http::Response` handler; dispatch runs **before** the live/shell/page
routes (so an API route may share a path with a page), and a path that exists for other methods
answers `405` with `Allow` instead of `404`. Public helpers are `web::json`, `web::redirect` and
`web::text`; `AppEndpoint.cache_control` (default `no-store`) is applied only when the handler left
`Cache-Control` unset, and `App.endpoint_count()` reports the size of the table. The storefront probe
registers its four mutating/JSON routes and its hand-written dispatch is gone.

**2. ~~The typed client job is not connected to the served page~~ — fixed (third pass).**
`--web` compiles `client/*.ch` to `web-client/assets/*.mjs`, and the served shell referenced none of
it, so typed client code could not ship. `web::App.add_client_module()` now publishes those assets
under `<asset_prefix>/client/` and adds a `boot.mjs` module script to every page's `<head>`.
*Original proposal:* a `web::App.client_bundle(path)` / module-level marker that puts the emitted client module
into the page bundle (and into `SharedAssets`), plus one `web-client` gate that asserts the served
HTML references it.

**3. No form parsing, and no `web::Form`.**
`application/x-www-form-urlencoded` has no parser in `http` or `web`; every app writes percent
decoding by hand (the storefront probe carries ~60 lines of it). The plan lists "forms with
progressive enhancement" as v1 and `web::Form` in the frozen API; neither exists.
*Landed (2026-11-21), urlencoded only.* `http::Form` (`get`/`text`/`has`/`size`/`name_at`/
`value_at`) over `parse_query`, so `+`/`%XX` decoding is shared with query strings, plus
`http::parse_form(body)` and `web::form(req)` (which returns an empty form for another media type, so
a JSON handler is not fed a mis-parse). One type, not two: a separate `web::Form` would have been the
same struct, so the request-shaped accessor is the whole web-side addition.
*`multipart/form-data` landed (2026-11-22).* `http::parse_multipart` / `req.multipart()` walk the raw
bytes — a file part's body is kept byte-exact (NUL, CR, LF, high bytes), which is what a urlencoded
parser cannot do — and `MultipartForm.fields()` hands the text fields over as the same `Form`, so a
handler written for a urlencoded body keeps working. A body that stops before its closing boundary is
an error, not a form with a part missing. CSRF is still `SessionStore.verify_csrf` called by the
handler rather than something the form does, because that is a policy decision per route.

**4. A page with no components still ships the 48 KB hydration runtime.**
`web::App` calls `defaultUniversalSetup()` on every page unconditionally, so the storefront — no
`#universal` anywhere — serves a 48 KB bundle on `/`. `web::Static` does not (a post page has zero
`<script>` tags), so the behaviour is also inconsistent between the two entry points.
*Landed (2026-11-21), measured.* `web::App` renders the handler first and calls
`defaultUniversalSetup()` only when `page.used_universal()` (set by the first `require_component`,
i.e. by the generated guard of a component actually rendered). A component-free page therefore ends
up with an empty `pageJs`/`pageJsEnd`, and `StaticSite` gives it no chunk, no bundle `<script>` and
no `js`/`css` artifact at all (`build()` also stops naming a zero-byte bundle). Per-route chunks are
now named `chunk.<hash>.js` by content only, so identical chunks collapse into one artifact.

Measured on the probes (same host, `--mode debug_quick`, `-bm-modules`):

| probe | before | after |
|---|---|---|
| `shop` `/` (no components) | 432 B, `assets/shop.24adc1dc6e45.js` (48,283 B; 11,051 B gz) | 270 B, **no script** |
| `shop` artifact set | 8 | 4 |
| `blog` 47 pages | 100 artifacts (47 route-named chunks) | 57 artifacts (4 `chunk.<hash>.js`) |

The universal plugin suite (`--tcc --plugins`, 1,315) and the component-bearing `blog` page still
hydrate: a page that does render a component gets the runtime, its definition and its chunk.

**5. ~~Interpolation never escapes~~ — fixed (third pass): `{expr}` escapes, `{!expr}` is raw.**
`page.ch` documents it: "The #html macro does NOT auto-escape interpolated values". `{userInput}`
is written verbatim into element text and into attribute values, so attribute injection is one
`{value}` away. The escape hatch direction is backwards: raw is the default, and `escape_html_view`
is opt-in.
*Landed (2026-11-22).* `{value}` is a text node and is escaped once; `{!value}` splices markup
verbatim; attribute values are escaped once; a `<script>`/`<style>` body stays raw text (see the
third-pass section at the end for why an escaped JSON island would be a bug). Universal
`props.children` still render raw, which is the documented opt-in a type expresses with
`writeToPageHtml` instead of `writeTextToPageHtml`.

### P1 — the API works but makes every app duplicate machinery

*Landed (2026-11-21).* `PageHandler = (ctx: &mut RequestContext, page: &mut HtmlPage)` and
`EndpointHandler = (ctx: &mut RequestContext) => http::Response`; `web::RequestContext` carries the
request (`ctx.param`, `ctx.query`, `ctx.header`, `ctx.path`, `ctx.body_text`, `ctx.form`) and the
session (`ctx.session_text`, `ctx.set_session`, `ctx.csrf_token`, `ctx.verify_csrf`,
`ctx.destroy_session`, `ctx.ensure_session`). `web::App` owns the `SessionStore`, loads the session
once per request, creates one only when a route asks, and attaches `Set-Cookie` when it did
(`app.session_store()` for config). The storefront probe's globals, pointer-to-request-local and
`handle_request` wrapper are gone; its CSRF check is `ctx.verify_csrf(t)` and its cart is
`ctx.session_text("cart")`.

One shape note for whoever extends this: `App.handle` takes `*mut http::Request`, not
`&mut`. A `RequestContext` holds the request for the route's lifetime, and the language refuses to
turn `&mut Request` into `*mut Request` for a destructible struct, while a reference — carrying a
lifetime — drags the whole generic machinery (and `@retained` restrictions) into every context
method. Callers write `app.handle(&raw mut req)`.

**6. (original finding, for reference)** A page handler got only `(page, req)`: the session, the
authenticated user, the locale and the CSRF token all had to be recovered inside every handler.
Because a mutable top-level global is `SAFE-GLOBAL-001`, the storefront probe published the session
through a pointer to a request-local and every access was `unsafe`.

*Resolved (2026-11-21).* `Session` was already an id-only handle (`{ id: string }`); the friction was
that nothing resolved it for a handler. `RequestContext.session` is that handle, loaded once per
request by `App.handle`, and every value is read through the store (`ctx.session_value` /
`ctx.session_text`), so there is no snapshot to keep fresh. The remaining cost is one 48-char id
allocation per request that has a session — not worth a borrowed form.

**8. `Static.add(page, path)` stores the `string_view` and never copies it.** The caller must keep
every path alive until `build()`; a view into a loop-local string produced
`two different artifacts at the same path` (a dangling view read back as a different path). There is
no diagnostic and no documentation of the lifetime. *Fix:* copy the path inside `add` (one
allocation per page, negligible next to the HTML) — or take `&str` so the compiler enforces
ownership. **Landed as `Static.page(path)`**, which does both and also owns the page: the raw pointer
to a loop-local page was the same bug, and it produced wrong bytes with no error (see the second
survey below).

**9. Per-route chunks are not content-addressed.** 40 identical 28-byte chunks became 40 URLs with
the same content hash in the name. *Fix:* name a chunk by its content hash only (`chunk.<hash>.js`)
so identical chunks collapse to one artifact (and one request), independent of route count.

**10. `defaultUniversalSetup()` is a ritual with a late failure.** Forgetting it fails at build
("a page hydrates components but the shared bundle has no hydration runtime"), not at the call that
should have set it up. *Fix:* have the sink emit the runtime on the first `require_component` (it
already sees every instance), leaving `defaultUniversalSetup` as an explicit override.

**11. Testing a request is hard.** `RequestBody`'s fields are internal and its constructor is empty,
so a test cannot give a handler a body (`Request.set_body_text`/`set_body_bytes` landed with P0-3,
which is enough for the probes but still not a one-call `Request.make`); `Request.query` likewise cannot be filled, and
`App.handle` takes `&mut Request` (fine) while `StaticSite.add` wants the page *and* keeps pointers
(now solved by `Static.page(path)`). A probe can time its own renders after all —
`std::time.Instant::now()` / `.elapsed()` is a monotonic clock and both new probes use it; this item
is down to `http::Request.make(method, path, headers, body)` with fillable query and headers.

**12. The client-side primitive set is too small for an SPA.** `dom` has `get_element_by_id`,
`query_selector`, `create_element`, `set_text`, `set_html`, attributes, `append_child`, `remove`,
`add_event_listener`, `fetch_text`, `localStorage`, and URL parsing — but no
`query_selector_all`, `child_count`/`child_at`, `remove_child`, `location`/hash, history,
`fetch` with headers/body, or a JSON decoder. The SPA probe had to drive navigation from a click
handler on a container, clear lists with `set_html("")`, and split JSON by hand. *Fix (additive):*
add those members to `dom` (they are additive to the frozen v1 set), plus `web::Client` for typed
`fetch` with a Chemical callback.

### P2 — missing layers, not API breaks

* **Content API.** The blog reads generated strings; real content is files or a CMS. A
  `content/` module (`load(dir) -> []Page` with front matter) plus `get_static_paths`-style route
  generation is what a content site actually needs.
* **Layouts/slots for plain pages.** `web::Shell` exists for the frozen-chrome case, but a normal
  multi-page site has no layout mechanism: the blog probe writes its header/footer three times
  (there is a `layout()` helper in the probe that an app would have to invent).
* **Images/assets pipeline** (resize, `srcset`, content-hashed imports).
* **i18n/RTL**, **HMR/dev overlay**, **streaming SSR**.

## What to do first (plan)

Ordered so each step is shippable and gated by a probe we already have. Each is additive except
where noted, and none is a rewrite.

| # | item | why first | done when |
|---|---|---|---|
| 1 | ~~`App.endpoint` + public `web::json`/`web::redirect`~~ **done** | unblocks every mutating app (P0-1) | `shop`'s POST routes are registered through the app; its hand-written dispatch disappears; gate green |
| 2 | ~~Form parsing + `web::Form`~~ **done** (urlencoded) | forms are claimed v1 (P0-3) | `shop`'s `form_value`/`url_decode` are deleted and replaced by `web::form` + `http::Form` |
| 3 | ~~Emit the hydration runtime only when a component was used (P0-4)~~ **done**, content-address chunks (P1-9) **done** | immediate, measurable win for the common case | `shop` `/` and `/products` carry no bundle (432 B → 270 B); blog artifacts 100 → 57 with 4 `chunk.<hash>.js` |
| 4 | ~~Connect the client job to the page bundle (P0-2)~~ **done as `app.add_client_module()`** | typed client code had nowhere to ship | `spa`'s shell loads `assets/client/boot.mjs`; a Node harness boots it and renders |
| 5 | ~~Escape-by-default `{}` + explicit raw marker~~ **done, raw form is `{!expr}`** | it was the only security bug in the list | `shop`'s search page needs no `escape_html_view`; script/style bodies stay raw |
| 6 | ~~`RequestContext` (P1-6) + `Session` handle (P1-7)~~ **done** | removes the globals/`unsafe` pattern from every route | `shop` keeps its session in the app, no `unsafe` for session access |
| 7 | ~~Copy the path and own the page in `Static.add` (P1-8)~~ **done as `Static.page(path)`**; `Request.make` + `now_ms` (P1-11) | probe/gate ergonomics; one-line-ish each | `blog` drops `page_paths`; probes time their own renders |
| 8 | `dom` additions + `web::Client` (P1-12) | makes an SPA expressible without the `#js` escape hatch | `spa`'s client uses `location.hash`, `query_selector_all`, and typed `fetch` |

Items 1–4 are the difference between "the stack can render pages" and "the stack can run an app".
Item 5 is a design decision that should be taken in the same release as 1, because both change what
an app author writes in every route.

---

## Second survey: `tasks` and `docs` (2026-11-21)

Two more shapes were built the same way — documented API only — because the first three did not
cover the two axes that decide whether the public API is final: **per-user mutable state over a
session** (a real app) and **hundreds of similar routes** (a real content site).

| probe | shape | what it exercises |
|---|---|---|
| `tasks` | authenticated multi-user app | `app.page` + `app.live` + `app.shell` + `app.endpoint` on **shared paths**, per-user sessions, form POST → 303, per-user lists, `ctx.form`, CSRF token in a form, 405 vs 200 on one path |
| `docs` | documentation site | 43 pages generated from one table with sections, a sidebar marking the active page, prev/next, sitemap/robots, a JSON search index artifact, gzip |

### Measured

| measurement | `tasks` | `docs` |
|---|---|---|
| checks green | 38 | 11 (the search index replaced one 404 assertion with three positive ones) |
| build | — | **~50 ms** |
| artifacts | 3 (1 page, 1 shell, 1 chunk) | 46 (43 HTML + sitemap + robots + search index) |
| total bytes | shell 381 B | 143,128 B (136,565 B HTML, biggest page 3,688 B) |
| JS sent to a component-free page | **0 B** (no bundle, no `<script>`) | **0 B**, 0 script tags on every page |
| per-request render | **~15 µs** (400 renders in 6.2 ms) | n/a (all pre-rendered) |
| gzip | — | on; HTML has gzip variants |

Two numbers matter for the goals. `tasks` shows the *precompute* claim holding for an app: the chrome
is a frozen shell (381 B, spliced), and the only per-request work is the list itself at ~15 µs. `docs`
shows the *zero-JS* claim holding at scale: 43 pages with a sidebar and pagination ship no JavaScript
at all, because every page is HTML and no page mounts a component.

### Bugs found and fixed here

1. **`Static.add` in a loop was a silent use-after-free.** Pages created in a loop and registered
   with `site.add(&mut page, path)` stored a raw pointer into a scope that no longer existed, plus a
   `string_view` into a loop-local path. The result was **wrong HTML bytes, no error** — the `blog`
   probe's post pages had been corrupt and its earlier measurements invalid. This is the worst class
   of bug for this project (nothing crashes, the output is just wrong), so the API changed rather
   than the docs: **`web::Static.page(path) : &mut HtmlPage`** — the site owns pages in vectors
   reserved to `PAGE_CAPACITY = 512`, reserves them up front, and copies the path. `blog` and `docs`
   both use it; over capacity fails loudly in `error_message()`.
2. **A page route sharing a path with an endpoint answered 405 to GET.** `app.live("/todos")` plus
   `app.endpoint("POST", "/todos")` made `GET /todos` return 405, because dispatch asked "which
   methods are registered for this path" and did not consider that a page route answers GET/HEAD.
   Fixed with `page_route_matches` + `method_listed` (a hand-built request with an empty method counts
   as GET; HEAD is served by GET). The `tasks` probe locks this down.
3. **A non-HTML artifact had nowhere to go.** A search index, RSS feed or web manifest was not
   expressible: the artifact set was HTML routes plus sitemap/robots/profile, so `docs` could only
   assert a 404. Added `StaticSite.add_artifact(path, bytes, content_type)` (copied, pre-build only)
   and delegated `web::Static.add_artifact`; `docs` now serves `search-index.json` with
   `application/json; charset=utf-8` and its own ETag. Also exposed `artifact_bytes(path)` and
   `has_gzip_variants()` on `web::Static`.

### What the second survey adds to the gap list — and what the third pass closed

Four of the six were fixed in the pass that followed the survey. The last two (`P0-2`, `P0-5`) were
feature-sized and are closed in the third pass below.

* ~~**Shell fills never see the session**~~ **fixed.** `SlotValues`/`ShellPredicate` now take
  `&mut RequestContext`, so a fill reads `ctx.session_text("uid")` and a variant predicate can choose
  "signed in vs out" chrome. `tasks` renders `Signed in as user N` from the session (and `Not signed
  in` anonymously) in a **frozen 381 B shell** — the same shell for every viewer, one spliced slot
  apart. A fill still cannot *create* a session: the document is spliced by `render_shell`, not built
  by `App.handle`, so no `Set-Cookie` would ride along. Mutating routes are `live`/`endpoint`.
* ~~**An endpoint cannot render a page**~~ **fixed as `ctx.render_page(handler, status, cache)`.** It
  renders the page against the app's frozen bundle exactly like a `live` route (a component in it
  hydrates) and answers 503 when the bundle lacks one, which is the failure a `live` route reports.
  `tasks` re-renders its sign-in form with **422** on a wrong password, and the reason travels in the
  session flash. PRG is still the default for a successful POST.
* ~~**App state has no home**~~ **fixed as `app.set_state<T>(&mut state)` + `ctx.state<T>()`.** The app
  keeps one pointer (the caller keeps ownership); every route — including a build-time `page` route —
  reads the typed value from the context. `tasks` dropped its `@never_destructed` global and its
  `db()` accessor; `Db` is now a stack local in `main`, and the only `unsafe` left in `db.ch` is the
  single dereference of the pointer the app was given. Note: the type argument is **required**
  (`app.set_state<Db>(&mut db)`) — a generic parameter that appears only behind a reference is not
  inferred (`SymRes:link: couldn't infer type for generic parameter`).
* ~~**A request cannot be built for a test**~~ **fixed as `http::request(method, path)`** plus the
  chainable `with_cookie` / `with_header` / `with_form` / `with_json`. It splits and decodes `?query`
  off the path exactly as the server's parser does, so a handler cannot tell a built request from a
  received one. `is_form_post()` was missing from `http::Request` despite being documented — added
  there and on `RequestContext`. Pinned by six `@test`s in `lang/tests/server` (suite 9 → 16).
* **`respond()` is identity-only and that is the right split** (checked while writing `docs`). With
  `set_gzip(true)` an artifact has two representations and two ETags; `respond(path)` always returns
  the identity bytes, and `respond_negotiated(path, etag, accept_encoding)` returns the gzip body +
  `Content-Encoding: gzip` + its own ETag only when the header allows it. A server must call the
  negotiated form to get `Vary: Accept-Encoding` right, so the probes do; no API change.

### Next round, in order

| # | item | why | done when |
|---|---|---|---|
| 1 | ~~`SlotValues` takes `&mut RequestContext`~~ **done** | a shell is the shape for per-user chrome | `tasks` renders the signed-in user from the session in a fill, frozen shell unchanged |
| 2 | ~~`App.state<T>` + `ctx.state()`~~ **done** | every app has process state; it was a global behind `unsafe` | `tasks` keeps `Db` on the stack, no global |
| 3 | ~~`http::Request.make`~~ **done as `http::request`** | request-level testing was hand-rolled in every probe | probes/tests build a request in one call |
| 4 | ~~`web::render_page`~~ **done as `ctx.render_page`** | form re-render with errors | `tasks` answers **422** with the form and its message |
| 5 | ~~Point `spa`'s shell at the client bundle~~ **done as `app.add_client_module()`** | typed client code existed but nothing loaded it | the served shell references the module; a Node harness boots it |
| 6 | ~~Escape-by-default `{}`~~ **done, raw form is `{!expr}`** | the only security bug in the list | `shop` search needs no `escape_html_view` |

### Third pass: P0-2 and P0-5 closed, and what the served runtime measures (2026-11-22)

**Escape-by-default (P0-5) is done.** `{expr}` in `#html` is a **text node**: it is
HTML-escaped exactly once, so no app ever calls `escape_html_view` before interpolating
user data. `{!expr}` is the explicit raw form, for a value that already *is* markup. Two
rules complete the contract:

* **Attributes** are escaped once by the template (a value in `class={state}` needs no
  escaping either);
* a `<script>`/`<style>` body is **raw text** — the browser never entity-decodes it, so a
  JSON island (`<script type="application/json">{payload}</script>`) must not be escaped;
  the template leaves both literal text and interpolated values verbatim there.

The escape lives in `page`'s new `append_text_*` sinks (one definition, so it holds for
every backend), and a type opts out only by implementing `writeTextToPageHtml`-free
markup deliberately (`writeToPageHtml`), which is what universal `children` do. `shop`
needs no escaper call, `tasks` lost its `escape_html_view` helper, and the plugin suite
pins the contract (`html/src/escape.ch`, `script_interpolation.ch`,
`to_string.ch` — 1,318 tests).

**The `--web` client bundle is wired (P0-2).** One call:

```chemical
var app = web::App.make("dist", "app", "assets")
app.page("/", Shell)
var wired = app.add_client_module()      // false + error_message() without --web
```

`add_client_module()` publishes every `.mjs` asset the client job wrote to
`<build>/web-client/assets/` under `<asset_prefix>/client/`, and adds
`<script type="module" src="assets/client/boot.mjs">` to **every** page's `<head>`
(`static_site.add_head_tag`, which is also the general hook). `boot.mjs` is a two-line
module that imports `index.mjs` and calls `$run()`, so relative imports inside the bundle
(`./runtime.mjs`, `./web-client.mjs`) resolve without the app naming any file. `spa` now
checks the tag, serves the boot/index/program modules with 200, and
`lang/tests/web/probes/spa/client_boot_check.mjs` boots the exported bundle in Node
against a stub DOM (`fetch` included) and asserts the client rendered rows, fetched
`/api/tasks`, and bound the nav — 6 checks that the bytes are not just present but run.

A backend bug fell out of that wiring: two modules' non-capturing lambdas were both named
`__chemda_0`, so the C backend's single translation unit saw a redefinition (the counter
is per module). Lambda/closure helper names now include the enclosing function's name
(`MIRLowerer::lambda_scope_tag_`), which is unique across modules by construction.

**Served runtime, measured (`lang/tests/web/probes/bench`, `./scripts/web-probes-test.sh bench`)**

A benchmark probe now serves the app through `server.Server` and times the same route
three ways: `app.handle` alone (the framework), a fresh TCP connection per request, and
one kept-alive connection. 300 requests, one thread, loopback, TCC backend,
`--mode debug_quick`, single-threaded client, measured on an otherwise idle machine (a
loaded box reads 2–3× these latencies, so compare ratios across runs, not absolutes):

| route | delivery | `app.handle` p50 | new connection p50 | keep-alive p50 | keep-alive req/s |
|---|---|---|---|---|---|
| `/` | frozen page | **3 µs** | 63 µs | **38 µs** | ~25,000 |
| `/shell` | frozen shell + 2 slot fills | **5 µs** | 66 µs | **44 µs** | ~20,000 |
| `/live` | per-request render (24-item list) | **34 µs** | 103 µs | **77 µs** | ~12,600 |

Reading the numbers:

* the framework is **2–4 µs** per frozen request — routing, artifact lookup, ETag, splice;
  a full 2.5 KB per-request render of a 24-row list is **34 µs**. Rendering is not the
  bottleneck at this size;
* a fresh connection costs **~60 µs** on loopback (accept + close), which is 15× the
  framework work for the same page. That is why keep-alive is the shape to optimise and
  why a page with 10 assets under HTTP/1.1 without reuse pays 600 µs of nothing;
* **the first version of this table read keep-alive p50 = 40,412 µs.** Accepted sockets
  had Nagle on while a response is written as head + body, so the body waited for the
  client's delayed ACK: **~40 ms per request**, i.e. 48 req/s instead of 25,000 — a 500×
  regression that only appears on a reused connection (every browser). Fixed by setting
  `TCP_NODELAY` on accepted sockets (`net::raw::set_no_delay`, applied in
  `http::server`'s accept paths); the probe script now fails if keep-alive p50 exceeds
  5 ms, so the regression cannot come back silently.

What is worth improving next, in the order the profile suggests:

1. ~~**Write the response in one `send`.**~~ **Tried and measured: no gain, reverted.** Appending
   the body to the head buffer (with `head.reserve(head + body)` first, bodies up to 256 KiB)
   replaced one `write(2)` with a full copy of the body. A/B runs alternating both binaries,
   five iterations each, alternating order:

   | build | frozen keep-alive p50 | live keep-alive p50 |
   |---|---|---|
   | two sends (current) | 76–100 µs | 187–282 µs |
   | one send | 82–95 µs | 175–275 µs |

   The saved syscall is ~1 µs on loopback and the copy costs the same, so the two are
   indistinguishable — and for a large body the copy is strictly worse. Kept the streaming
   shape; a scatter-gather `writev` binding (absent from `net`) would be the only version worth
   trying, and it only pays if the response is written in one call with headers *and* body in
   separate buffers.
2. **A worker pool, not a thread per connection.** `serve_accept` submits every accepted
   socket to a pool and serves it blocking; with 300 sequential requests that is fine, and
   under concurrency it is the number to watch next (the benchmark is deliberately
   sequential so the framework cost is visible).
3. **Per-request render allocation.** `/live` is 34 µs for 24 rows; the string builder
   growth is the next thing to measure (reserve from the previous render, or render into
   a reused buffer).

The probe also prints `artifacts` and the frozen page size, so a size change is as visible
as a speed change.

### Still open

The production checklist — which of these remain, and what else a real deployment needs — is
[`web-stack-production.md`](web-stack-production.md). Items 1–3 of the HTTP response protocol
(`Date`, `Range`, `413`/`431` refusals) landed on 2026-11-22, as did the operator-facing gaps
(access logging, dynamic gzip, multipart uploads, directory-backed durable sessions); per-request
panic containment is the one production item still open.

| # | item | why | done when |
|---|---|---|---|
| 1 | ~~Response write as one syscall~~ **measured, no gain** (see 1 above); a `writev` binding is the only variant left | two sends cost ~1 µs more, the copy costs the same | a scatter-gather write measured faster than two sends |
| 2 | Concurrent-load benchmark | the numbers here are sequential by design | a second bench line at N in-flight requests |
| 3 | `dom` location/history API | the `spa` client routes by click handler; a real router needs `#js` | `spa` navigates a path without a click handler |
| 4 | Typed JSON decode in `dom` | `spa` splits strings to read `/api/tasks` | the client decodes into a struct |
