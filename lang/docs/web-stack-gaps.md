# Web stack: what three probe apps found (2026-11-21)

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
*Fix:* add `App.endpoint(method, pattern, handler: (req) => Response)` (or a `Handler` union) and
public `web::json`/`web::redirect` helpers. This is the single change that unblocks forms, auth
flows, JSON APIs and webhooks.

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
*Fix:* `http::Form.parse(body, content_type)` (urlencoded + a `multipart` follow-up), `web::Form`
with typed field lookup, required-field errors, and CSRF integration on `SessionStore`.

**4. A page with no components still ships the 48 KB hydration runtime.**
`web::App` calls `defaultUniversalSetup()` on every page unconditionally, so the storefront — no
`#universal` anywhere — serves a 48 KB bundle on `/`. `web::Static` does not (a post page has zero
`<script>` tags), so the behaviour is also inconsistent between the two entry points.
*Fix:* emit the runtime only when the sink actually recorded a component use (the graph already
knows), and drop the bundle `<script>` from pages that have none. Expected: `/` and `/products`
lose 48 KB each; a component-free app serves 0 B of JS.

**5. Interpolation never escapes, and there is no way to say "this is HTML".**
`page.ch` documents it: "The #html macro does NOT auto-escape interpolated values". `{userInput}`
is written verbatim into element text and into attribute values, so attribute injection is one
`{value}` away. The escape hatch direction is backwards: raw is the default, and `escape_html_view`
is opt-in.
*Fix:* make `{value}` escape by default and add an explicit raw marker (`SsrText`/`RawHtml`) that
components use for `props.children`/nested markup; `#html`'s own literal text stays raw. This is a
v1 break to take **now**, before apps exist (see §3.17 of the plan).

### P1 — the API works but makes every app duplicate machinery

**6. A page handler gets only `(page, req)`.** The session, the authenticated user, the locale and
the CSRF token all have to be recovered inside every handler. Because a mutable top-level global is
`SAFE-GLOBAL-001`, the storefront probe publishes the session through a pointer to a request-local
and every access is `unsafe`. *Fix:* `PageHandler = (ctx: &mut RequestContext, page: &mut HtmlPage)`
where the app provides a `resolve(req)` hook once (session, user, params), or a `Ctx` passed by
`App.handle`.

**7. `Session` is an owned value with no handle.** `SessionStore.load(&req) -> Option<Session>`
returns a struct owning its id `string`; there is no cheap `&Session`/id form, so an app cannot
keep "the current session" without either re-loading per handler (used here) or fighting the
borrow checker. *Fix:* `Session` as an id-only handle (`{ id: string_view }` into store-owned
storage) plus `store.with(id, |s| ...)`.

**8. `Static.add(page, path)` stores the `string_view` and never copies it.** The caller must keep
every path alive until `build()`; a view into a loop-local string produced
`two different artifacts at the same path` (a dangling view read back as a different path). There is
no diagnostic and no documentation of the lifetime. *Fix:* copy the path inside `add` (one
allocation per page, negligible next to the HTML) — or take `&str` so the compiler enforces
ownership.

**9. Per-route chunks are not content-addressed.** 40 identical 28-byte chunks became 40 URLs with
the same content hash in the name. *Fix:* name a chunk by its content hash only (`chunk.<hash>.js`)
so identical chunks collapse to one artifact (and one request), independent of route count.

**10. `defaultUniversalSetup()` is a ritual with a late failure.** Forgetting it fails at build
("a page hydrates components but the shared bundle has no hydration runtime"), not at the call that
should have set it up. *Fix:* have the sink emit the runtime on the first `require_component` (it
already sees every instance), leaving `defaultUniversalSetup` as an explicit override.

**11. Testing a request is hard.** `RequestBody`'s fields are internal and its constructor is empty,
so a test cannot give a handler a body; `Request.query` likewise cannot be filled, and
`App.handle` takes `&mut Request` (fine) while `StaticSite.add` wants the page *and* keeps pointers.
There is also no clock in `std` (`time` exists as a module — check it — but no monotonic `now_ms`),
so a probe cannot time its own renders. Both probes solved this by passing the body as a parameter
to their own `handle_request`. *Fix:* `http::Request.make(method, path, headers, body)` and a
monotonic `std::time.now_ms()`.

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
| 1 | `App.endpoint` + public `web::json`/`web::redirect` | unblocks every mutating app (P0-1) | `shop`'s POST routes are registered through the app; its hand-written dispatch disappears; gate green |
| 2 | Form parsing + `web::Form` | forms are claimed v1 (P0-3) | `shop`'s `form_value`/`url_decode` are deleted and replaced by `web::Form` |
| 3 | Emit the hydration runtime only when a component was used (P0-4) and content-address chunks (P1-9) | immediate, measurable win for the common case | `shop` `/` and `/products` carry no bundle; blog emits one `chunk.<hash>.js` for the 40 identical chunks |
| 4 | Connect the client job to the page bundle (P0-2) | without it typed client code cannot ship | `spa`'s shell references the client module; a Node harness runs it against the served shell |
| 5 | Escape-by-default `{}` + explicit raw marker (P0-5) | the only finding that is a security bug; must be decided before apps exist | `shop`'s search page needs no `escape_html_view`; universal `children` still render raw; plugin suite + components e2e green |
| 6 | `RequestContext` (P1-6) + `Session` handle (P1-7) | removes the globals/`unsafe` pattern from every route | `shop` keeps its session in the app, no `unsafe` for session access |
| 7 | Copy the path in `Static.add` (P1-8), `Request.make` + `now_ms` (P1-11) | probe/gate ergonomics; one-line-ish each | `blog` drops `page_paths`; probes time their own renders |
| 8 | `dom` additions + `web::Client` (P1-12) | makes an SPA expressible without the `#js` escape hatch | `spa`'s client uses `location.hash`, `query_selector_all`, and typed `fetch` |

Items 1–4 are the difference between "the stack can render pages" and "the stack can run an app".
Item 5 is a design decision that should be taken in the same release as 1, because both change what
an app author writes in every route.
