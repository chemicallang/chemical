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

### `shop` (22 checks green)

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

### `spa` (`--web`, 7 checks green)

| measurement | value |
|---|---|
| shell page | 810 B |
| JSON endpoint | 130 B |
| client job output | `web-client.mjs` 217,182 B (27,866 gzip) + `runtime.mjs` 27,586 B (8,060 gzip) + 778 B entry, unminified |
| shell references the client bundle? | **no** — the page loads `assets/spa.<hash>.js` (the universal hydration runtime), not `web-client.mjs` |

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

**2. The typed client job is not connected to the served page.**
`--web` compiles `client/*.ch` to `web-client.mjs`, and `web::App` emits `assets/spa.<hash>.js`
(the universal hydration runtime). Nothing joins them: the served shell never references the client
bundle. So typed client code — the stack's answer to "SPAs and client interaction" — cannot ship.
*Fix:* a `web::App.client_bundle(path)` / module-level marker that puts the emitted client module
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
same struct, so the request-shaped accessor is the whole web-side addition. `multipart/form-data`
(file uploads) remains open; CSRF is still `SessionStore.verify_csrf` called by the handler rather
than something the form does, because that is a policy decision per route.

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

**5. Interpolation never escapes, and there is no way to say "this is HTML".**
`page.ch` documents it: "The #html macro does NOT auto-escape interpolated values". `{userInput}`
is written verbatim into element text and into attribute values, so attribute injection is one
`{value}` away. The escape hatch direction is backwards: raw is the default, and `escape_html_view`
is opt-in.
*Fix:* make `{value}` escape by default and add an explicit raw marker (`SsrText`/`RawHtml`) that
components use for `props.children`/nested markup; `#html`'s own literal text stays raw. This is a
v1 break to take **now**, before apps exist (see §3.17 of the plan).

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
| 4 | Connect the client job to the page bundle (P0-2) | without it typed client code cannot ship | `spa`'s shell references the client module; a Node harness runs it against the served shell |
| 5 | Escape-by-default `{}` + explicit raw marker (P0-5) | the only finding that is a security bug; must be decided before apps exist | `shop`'s search page needs no `escape_html_view`; universal `children` still render raw; plugin suite + components e2e green |
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
| checks green | 31 | 11 (the search index replaced one 404 assertion with three positive ones) |
| build | — | **~50 ms** |
| artifacts | 3 (1 page, 1 shell, 1 chunk) | 46 (43 HTML + sitemap + robots + search index) |
| total bytes | shell 337 B | 143,128 B (136,565 B HTML, biggest page 3,688 B) |
| JS sent to a component-free page | **0 B** (no bundle, no `<script>`) | **0 B**, 0 script tags on every page |
| per-request render | **~15 µs** (400 renders in 6.2 ms) | n/a (all pre-rendered) |
| gzip | — | on; HTML has gzip variants |

Two numbers matter for the goals. `tasks` shows the *precompute* claim holding for an app: the chrome
is a frozen shell (337 B, spliced), and the only per-request work is the list itself at ~15 µs. `docs`
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

### What the second survey adds to the gap list

* **Shell fills never see the session** (new, P1). `app.shell(path, handler, values)` types `values`
  as `(req : &Request) => FillSet`, but the session was resolved by the app *before* the fill runs and
  lives on `RequestContext`. So the one route shape built for "chrome frozen, one region per user"
  cannot render the signed-in user's name or cart count from the session — it can only parse cookies
  itself. `tasks` works around it by rendering the greeting from a cookie. *Fix:* `SlotValues` should
  take `&mut RequestContext` (the fill runs inside `render_shell`, which already has it).
* **An endpoint cannot render a page** (confirmed, P1). `endpoint()` returns `http::Response`; a form
  post that should show a page must redirect (PRG) to a page route. That is the right default, but
  there is no `web::render_page(handler, ctx) : http::Response` escape hatch, so an endpoint that
  wants to re-render a form with errors has to build HTML by hand (the `tasks` probe avoids it by
  redirecting). *Fix:* expose the live-render path as a helper.
* **App state has no home** (confirmed, P1). `tasks` needs one `SessionStore` and one list of todos
  shared by every route. There is no `App`-owned store, so the probe keeps them behind one pointer in
  a request-local and touches it through `unsafe` — exactly the pattern `RequestContext` was added to
  remove, one level up. *Fix:* `App.state<T>(init) : &mut T` (or `app.set_state(&mut T)`), read back
  from a handler through `ctx.state()`.
* **`respond()` is identity-only and that is the right split** (checked while writing `docs`). With
  `set_gzip(true)` an artifact has two representations and two ETags; `respond(path)` always returns
  the identity bytes, and `respond_negotiated(path, etag, accept_encoding)` returns the gzip body +
  `Content-Encoding: gzip` + its own ETag only when the header allows it. A server must call the
  negotiated form to get `Vary: Accept-Encoding` right, so the probes do; no API change.
* **P1-11 is stale on the clock.** `std::time.Instant::now()`/`.elapsed()` exists and both new probes
  time themselves with it; only `http::Request.make` is still missing from that item.

### Next round, in order

| # | item | why | done when |
|---|---|---|---|
| 1 | `SlotValues` takes `&mut RequestContext` | a shell is the shape for per-user chrome; today it cannot read the session | `tasks` renders the signed-in user from `ctx.session_text` in a fill, no cookie parsing |
| 2 | `App.state<T>` + `ctx.state()` | every app has process state; today it is a global behind `unsafe` | `tasks` drops its request-local pointer |
| 3 | `http::Request.make` with fillable query/headers | request-level testing is hand-rolled in every probe | probes/tests build a request in one call |
| 4 | Point `spa`'s shell at the client bundle (P0-2, still open) | typed client code exists but nothing loads it | a Node harness runs the served shell's script |
| 5 | Escape-by-default `{}` (P0-5, still open) | the only security bug in the list | `shop` search needs no `escape_html_view` |
| 6 | `web::render_page` escape hatch for endpoints | form re-render with errors | an endpoint returns a rendered page |
