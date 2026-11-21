// `web::App` — an application as a route table, not a site as a mode.
//
// A `StaticSite` knows about pages; an app knows about **routes**, and each route
// decides when its HTML is produced:
//
//   * `app.page(path, Home)`      — pre-rendered at build, served from memory with
//                                   `no-cache` + ETag (304 on revalidation);
//   * `app.shell(path, Home, Fill)` — chrome rendered once, each request splices
//                                   its slot values into the frozen bytes (§3.12);
//   * `app.live(path, Acc, ...)`  — rendered per request against the same frozen
//                                   bundle, answered `no-store` by default.
//
// All three share one bundle: the warm pass in `build()` renders every live and
// shell route once so their components join the bundle before it is frozen, and a
// route that needs a component the bundle does not carry is refused (503 naming the
// reason) instead of shipped as HTML whose hydration cannot run.
//
//   var app = web::App.make("dist", "app", "assets")
//   app.page("/", Home)
//   app.page("/pricing", Pricing)
//   app.live("/account", Account)          // per request
//   app.redirect("/prices", "/pricing")
//   app.not_found(NotFound)
//   if(!app.finish()) { /* app.error_message() */ return 1 }
//
//   unsafe { web::install(&raw mut app) }
//   srv.router.add("GET", "/:path*", ||(req) => web::serve_request(req))
//
// The app owns its pre-rendered pages (the build holds a pointer to each), so an
// `App` must outlive anything that serves it — the same lifetime rule as
// `StaticSite`, one level up.
public namespace web {

using std::Option;
using std::pair;
using std::string;
using std::string_view;
using std::vector;

/// A route render function: it writes the page's markup and reads the request.
/// One shape serves a pre-rendered route and a per-request one; the delivery mode
/// is a property of the route, not of the function.
public type PageHandler = (page : &mut HtmlPage, req : &http::Request) => void

/// A non-page route: it owns its response, so it can answer any status, set any
/// header, and return JSON, a redirect or an empty body. This is what a form POST,
/// a JSON API and a webhook need; a `PageHandler` cannot express them because it
/// can only write HTML into a 200 page.
public type EndpointHandler = (req : &http::Request) => http::Response

/// One method-aware route (`app.endpoint("POST", "/cart/add", AddToCart)`).
@direct_init
public struct AppEndpoint {
    /// `GET`, `POST`, ... or `*` for any method.
    public var method : string
    public var pattern : string
    public var handler : EndpointHandler
    /// Applied only when the handler did not set `Cache-Control` itself.
    public var cache_control : string
    public var vary : string
}

/// A route: a path pattern plus how its HTML is produced.
@direct_init
public struct AppRoute {
    /// `/`, `/pricing`, `/invoices/{id}`, `/files/{path*}`.
    public var pattern : string
    public var handler : PageHandler
    /// True: render per request. False: render once, at build.
    public var per_request : bool
    /// Cache policy for a per-request render (`no-store` unless the route says
    /// otherwise). Pre-rendered routes always use `no-cache` + ETag.
    public var cache_control : string
    /// Optional `Vary` value for a render that depends on a request header
    /// (`accept-language`, `x-requested-with`). Empty means no `Vary`.
    public var vary : string

    public func is_static(&self) : bool {
        return !per_request
    }
}

@direct_init
public struct AppRedirect {
    public var from : string
    public var to : string
    public var status : u16
}

/// A route served from a frozen shell (§3.12): the handler's bytes are produced
/// once and `values` is the only code that runs per request.
@direct_init
public struct ShellRoute {
    public var pattern : string
    public var handler : PageHandler
    public var values : SlotValues
    /// Cache policy for the filled document (`private, no-cache` by default: the
    /// same URL returns different bytes per user).
    public var cache_control : string
    public var vary : string
    /// The frozen document; empty until `build()` compiles it.
    var shell : Shell
    var compiled : bool
    /// Variant selection: when `has_when`, this route answers only if the
    /// predicate accepts the request; otherwise it is the route's fallback. All
    /// variants of a pattern share the pattern, so dispatch is
    /// "first matching predicate, else the fallback".
    var when : ShellPredicate
    var has_when : bool

    public func is_compiled(&self) : bool {
        return compiled
    }

    /// How many regions the frozen document has (0 before `build()`).
    public func slot_count(&self) : ubigint {
        return shell.slot_count()
    }
}

/// The application: route table + shared bundle + serving.
@direct_init
public struct App {
    private var site : StaticSite
    private var routes : vector<AppRoute>
    private var shells : vector<ShellRoute>
    private var endpoints : vector<AppEndpoint>
    private var redirects : vector<AppRedirect>

    /// Owned by the app because the build holds `*mut HtmlPage` to each one and
    /// those pointers must stay valid until the bundle is frozen.
    private var pages : vector<HtmlPage>
    /// Artifact paths of the pre-rendered pages. The sink keeps a *view* of each
    /// path, so every path is allocated before the first view is handed over.
    private var page_paths : vector<string>

    private var not_found_handler : PageHandler
    private var has_not_found : bool

    /// Build outputs, replayed in `build()` (a rebuild of the sink must not lose
    /// them).
    private var out_dir : string
    private var base_name : string
    private var asset_prefix : string
    private var base_path : string
    private var site_url : string
    private var want_sitemap : bool
    private var want_robots : bool
    /// Replayed in `build()`: it rebuilds the sink from the final configuration, so
    /// a flag set before that must be stored here, not on the old sink.
    private var want_minify : bool
    /// Precompress the artifact set and negotiate it per request (`set_gzip`).
    private var want_gzip : bool
    /// Add `profile.json` to the export (`set_profile`).
    private var want_profile : bool

    private var built : bool
    private var failed : bool
    private var failure : string

    @make
    public func make(out_dir : string_view = "out", base_name : string_view = "app",
                     asset_prefix : string_view = "assets") : App {
        var o = string()
        o.append_view(&out_dir)
        var n = string()
        n.append_view(&base_name)
        var p = string()
        p.append_view(&asset_prefix)
        return App {
            site : StaticSite.make(out_dir, base_name, asset_prefix),
            routes : vector<AppRoute>(),
            shells : vector<ShellRoute>(),
            endpoints : vector<AppEndpoint>(),
            redirects : vector<AppRedirect>(),
            pages : vector<HtmlPage>(),
            page_paths : vector<string>(),
            not_found_handler : empty_page,
            has_not_found : false,
            out_dir : o,
            base_name : n,
            asset_prefix : p,
            base_path : string(),
            site_url : string(),
            want_sitemap : false,
            want_robots : false,
            want_minify : false,
            want_gzip : false,
            want_profile : false,
            built : false,
            failed : false,
            failure : string()
        }
    }

    // ── Route table ──────────────────────────────────────────────────────
    /// Register a **pre-rendered** route: `handler` runs once in `build()` and
    /// requests are answered from the frozen artifact. The pattern must be
    /// concrete (no `{param}`) — a URL that only exists per request is a `live`
    /// route.
    public func page(&mut self, pattern : string_view, handler : PageHandler) {
        push_route(pattern, handler, false, std::string_view("no-cache"), std::string_view(""))
    }

    /// Register a **per-request** route: `handler` runs for every request against
    /// the site's frozen bundle. `cache_control` defaults to `no-store` because the
    /// same URL now returns different bytes; pass `"private, no-cache"` when the
    /// render is stable per URL.
    public func live(&mut self, pattern : string_view, handler : PageHandler,
                     cache_control : string_view = "no-store", vary : string_view = "") {
        push_route(pattern, handler, true, cache_control, vary)
    }

    /// Register a **method-aware endpoint**: `handler` runs per request and returns
    /// the whole response, so it can answer 201/303/409, set headers, and return
    /// JSON. This is the route kind for a form POST, an API and a webhook.
    ///
    /// ```chemical
    /// app.endpoint("POST", "/cart/add", AddToCart)
    /// app.endpoint("GET", "/api/products", ProductsJson)
    /// app.endpoint("*", "/hooks/{id}", Hook)
    /// ```
    ///
    /// `method` is matched case-insensitively; `*` accepts any method. A request
    /// whose path matches an endpoint registered only for another method is answered
    /// **405** with an `Allow` header, not 404. Endpoints are tried before live and
    /// pre-rendered routes, so an endpoint can claim `/api/...` next to a page.
    ///
    /// `cache_control` (default `no-store`) is applied only when the handler left the
    /// header unset: a handler that decides its own caching wins.
    public func endpoint(&mut self, method : string_view, pattern : string_view,
                         handler : EndpointHandler, cache_control : string_view = "no-store",
                         vary : string_view = "") {
        var m = string()
        m.append_view(&method)
        var p = string()
        p.append_view(&pattern)
        var c = string()
        c.append_view(&cache_control)
        var v = string()
        v.append_view(&vary)
        endpoints.push_back(AppEndpoint {
            method : m,
            pattern : p,
            handler : handler,
            cache_control : c,
            vary : v
        })
    }

    /// How many method-aware endpoints the app has.
    public func endpoint_count(&self) : ubigint {
        return endpoints.size()
    }

    /// Register a **shell** route (§3.12): `handler` renders the whole page once, in
    /// `build()`, and every request is answered by splicing `values(req)` into the
    /// frozen bytes. The chrome costs nothing per request — no component walk, no
    /// CSS hashing, no hydration-id allocation.
    ///
    /// The handler marks its varying regions with `page.slot("name")`. `values`
    /// fills them with plain HTML (`set`) or escaped text (`set_text`); it must not
    /// call a component, because the hydration ids around it are already frozen.
    /// The pattern may capture (`/dashboard/{id}`); the handler need not depend on
    /// the request (a shell is rendered against an empty one).
    public func shell(&mut self, pattern : string_view, handler : PageHandler, values : SlotValues,
                      cache_control : string_view = "private, no-cache", vary : string_view = "") {
        var p = string()
        p.append_view(&pattern)
        var cache = string()
        cache.append_view(&cache_control)
        var vary_s = string()
        vary_s.append_view(&vary)
        shells.push_back(ShellRoute {
            pattern : p,
            handler : handler,
            values : values,
            cache_control : cache,
            vary : vary_s,
            shell : Shell(),
            compiled : false,
            when : default_shell_predicate,
            has_when : false
        })
    }

    /// A **variant** of a shell route (Phase 4 §3.12): the same pattern, a different
    /// frozen document, chosen per request by `when`.
    ///
    /// This is how "signed in vs signed out" chrome is done without branching
    /// inside a frozen buffer (which would defeat the byte-range serving): register
    /// one variant per shape, each with its own handler and fill provider, and the
    /// app picks the first whose predicate accepts the request. A variant registered
    /// with `shell` (no predicate) is the fallback when no predicate matches.
    ///
    /// Every variant is warmed and compiled at build, so each one's components are
    /// in the bundle and a request is still one splice per slot.
    public func shell_variant(&mut self, pattern : string_view, handler : PageHandler, values : SlotValues,
                              when : ShellPredicate,
                              cache_control : string_view = "private, no-cache", vary : string_view = "") {
        var p = string()
        p.append_view(&pattern)
        var cache = string()
        cache.append_view(&cache_control)
        var vary_s = string()
        vary_s.append_view(&vary)
        shells.push_back(ShellRoute {
            pattern : p,
            handler : handler,
            values : values,
            cache_control : cache,
            vary : vary_s,
            shell : Shell(),
            compiled : false,
            when : when,
            has_when : true
        })
    }

    /// The 404 page. Rendered per request (it is the one route that must always be
    /// available), so it is warmed with the bundle like any live route.
    public func not_found(&mut self, handler : PageHandler) {
        not_found_handler = handler
        has_not_found = true
    }

    /// A permanent (308) or temporary redirect. Redirects are checked before any
    /// route and are never rendered, so they cost one string compare.
    public func redirect(&mut self, from : string_view, to : string_view, status : u16 = 308u) {
        var from_s = string()
        from_s.append_view(&from)
        var to_s = string()
        to_s.append_view(&to)
        redirects.push_back(AppRedirect {
            from : from_s,
            to : to_s,
            status : status
        })
    }

    /// Serve the app under a mount path (`/app`): requests are matched with the
    /// prefix stripped and the bundle is published at `<base>/assets/...`, so the
    /// same build works at the root and behind a proxy prefix.
    public func set_base_path(&mut self, base : string_view) {
        base_path.clear()
        base_path.append_view(&base)
    }

    /// Minify the frozen bundle's JS/CSS (`page.StaticSite.set_minify`). Call before
    /// `build()`; it is a build-time cost only, and never changes the served HTML.
    public func set_minify(&mut self, on : bool) {
        want_minify = on
        site.set_minify(on)
    }

    /// Precompress every text artifact and serve the compressed representation to
    /// clients that send `Accept-Encoding: gzip` (`StaticSite.set_gzip`). Call
    /// before `build()`; the flag is re-applied to the sink `build()` creates.
    public func set_gzip(&mut self, on : bool) {
        want_gzip = on
        site.set_gzip(on)
    }

    public func is_gzip_enabled(&self) : bool {
        return want_gzip
    }

    /// Write `profile.json` into the artifact set: per-route HTML/chunk bytes,
    /// chunk composition, and the first-load vs navigation cost of every route
    /// (`StaticSite.export_profile`). The build script decides — `build.lab` turns
    /// `chemical build --web --profile` into this call through
    /// `intrinsics::defined("profile")`.
    public func set_profile(&mut self, on : bool) {
        want_profile = on
    }

    public func is_profile_enabled(&self) : bool {
        return want_profile
    }

    /// The profile report as text (empty until `build()`), for a build script that
    /// wants to print it.
    public func profile_report(&self) : std::string {
        return site.profile_report()
    }

    public func is_minify_enabled(&self) : bool {
        return want_minify
    }

    public func route_count(&self) : ubigint {
        return routes.size()
    }

    public func shell_count(&self) : ubigint {
        return shells.size()
    }

    /// Bytes frozen for every shell route: paid once at build, shared by every
    /// request.
    public func shell_frozen_bytes(&self) : ubigint {
        var total : ubigint = 0
        var i : ubigint = 0
        while(i < shells.size()) {
            var r = unsafe(&*shells.get_ptr(i))
            total = total + r.shell.frozen_bytes()
            i = i + 1
        }
        return total
    }

    /// The shell route at `index` (a build helper reads `is_compiled()`/slot count
    /// from it without holding a borrow on the table).
    public func shell_route_at(&self, index : ubigint) : *mut ShellRoute {
        return shells.get_ptr(index)
    }

    public func error_message(&self) : string_view {
        return failure.to_view()
    }

    public func static_site(&self) : &StaticSite {
        return &site
    }

    // ── Unit graph (build diagnostics) ───────────────────────────────────
    /// Component definitions the bundle kept (the runtime plus everything two
    /// routes — or any per-request route — use).
    public func site_retained_units(&self) : ubigint {
        return site.retained_unit_count()
    }

    /// Component definitions moved into a single route's chunk.
    public func site_chunked_units(&self) : ubigint {
        return site.chunked_unit_count()
    }

    /// `name:owners` per recorded component definition.
    public func site_unit_report(&self) : std::string {
        return site.unit_report()
    }

    /// The route at `index` (the build helper and the dispatcher walk the table
    /// without holding a `&self` borrow on it).
    public func route_at(&self, index : ubigint) : *mut AppRoute {
        return routes.get_ptr(index)
    }

    // ── Build ────────────────────────────────────────────────────────────
    /// Render every pre-rendered route and **warm** every per-request route, then
    /// freeze the bundle. After this the app answers requests; nothing here
    /// touches the filesystem (that is `finish()`).
    public func build(&mut self) : bool {
        if(built) { return !failed }
        built = true

        // One sink built for the final configuration: a mount path changes the
        // bundle's URL, and every page must be rendered against the prefix it is
        // actually served under.
        var prefix = self.resolved_asset_prefix()
        var prefix_view = prefix.to_view()
        var out_view = out_dir.to_view()
        var name_view = base_name.to_view()
        site = StaticSite.make(out_view, name_view, prefix_view)
        if(want_minify) { site.set_minify(true) }
        if(want_gzip) { site.set_gzip(true) }

        if(!warm_artifacts()) { return false }

        if(want_sitemap || want_robots) {
            if(site_url.empty()) {
                fail_with("export_sitemap() needs a site URL")
                return false
            }
            var url_view = site_url.to_view()
            site.set_site_url(url_view)
            if(want_sitemap && !site.export_sitemap()) {
                fail_with(site.error_message())
                return false
            }
            if(want_robots && !site.export_robots(want_sitemap)) {
                fail_with(site.error_message())
                return false
            }
        }

        if(!site.build()) {
            fail_with(site.error_message())
            return false
        }
        if(want_profile) {
            if(!site.export_profile()) {
                fail_with(site.error_message())
                return false
            }
        }
        if(!compile_shells()) { return false }
        return !failed
    }

    /// `build()` plus writing the artifact set (a static export). Per-request
    /// routes are not artifacts, so an export covers exactly the pre-rendered
    /// routes — the correct answer for a route that needs a request.
    public func finish(&mut self) : bool {
        if(!build()) { return false }
        if(!site.finish()) {
            fail_with(site.error_message())
            return false
        }
        return true
    }

    /// Publish `sitemap.xml` (and `robots.txt`) for the pre-rendered routes. Call
    /// before `build()`/`finish()`; the artifacts are added when the build runs, so
    /// the artifact list and the bundle can never disagree.
    public func export_sitemap(&mut self, url : string_view, robots : bool = true) {
        if(built) {
            fail_with("export_sitemap must be called before build()")
            return
        }
        self.site_url.clear()
        self.site_url.append_view(&url)
        want_sitemap = true
        want_robots = robots
    }

    // ── Serving ──────────────────────────────────────────────────────────
    /// Answer one request: redirect → live route → pre-rendered page/asset → 404.
    ///
    /// `req.route_params` is filled for a matched pattern, so a handler reads
    /// `/invoices/{id}` through `req.param("id")`.
    public func handle(&mut self, req : &mut http::Request) : http::Response {
        if(!built) {
            var not_built = string_view("app not built")
            return error_response(503u, &not_built)
        }

        // The path as the app sees it: query and fragment dropped, mount prefix
        // stripped. Matching and artifact lookup both use it.
        var path_s = request_path_without_base(&base_path, req.path.to_view())
        if(path_s.empty()) {
            return self.not_found_response(req)
        }
        var path = path_s.to_view()

        // 1. Redirects: a table lookup, no render.
        var ri : ubigint = 0
        while(ri < redirects.size()) {
            var r = unsafe(&*redirects.get_ptr(ri))
            var from_view = r.from.to_view()
            if(path_equals(&from_view, &path)) {
                var status = r.status
                var to_view = r.to.to_view()
                return redirect_response(status, to_view)
            }
            ri = ri + 1
        }

        // 2. A method-aware endpoint: it owns its response, so it can answer any
        //    status or content type. Tried first because an API route may share a
        //    path with a page (a GET page, a POST handler) — and because a request
        //    for a path that exists for another method must be 405, not 404.
        var wildcard = string_view("*")
        var endpoint_hit : int = -1
        var endpoint_params = vector<pair<string, string>>()
        var ei : ubigint = 0
        while(ei < endpoints.size()) {
            var e = unsafe(&*endpoints.get_ptr(ei))
            var mv = e.method.to_view()
            var pv = e.pattern.to_view()
            var trial = vector<pair<string, string>>()
            if(method_matches(&mv, &wildcard, req) && match_route_pattern(&pv, &path, &mut trial)) {
                endpoint_hit = ei as int
                endpoint_params = trial
                break
            }
            ei = ei + 1
        }
        if(endpoint_hit >= 0) {
            fill_route_params(req, &endpoint_params)
            var route = unsafe(&*endpoints.get_ptr(endpoint_hit as ubigint))
            var handler = route.handler
            var res = handler(unsafe(&*req))
            apply_endpoint_policy(&mut res, route)
            return res
        }
        var allow = self.allowed_methods(&path)
        if(!allow.empty()) {
            return method_not_allowed(&allow)
        }

        // 3. A live route.
        var matched : int = -1
        var params = vector<pair<string, string>>()
        var i : ubigint = 0
        while(i < routes.size()) {
            var r = unsafe(&*routes.get_ptr(i))
            if(r.per_request) {
                var pv = r.pattern.to_view()
                if(match_route_pattern(&pv, &path, &mut params)) {
                    matched = i as int
                    break
                }
            }
            i = i + 1
        }
        if(matched >= 0) {
            fill_route_params(req, &params)
            var route = unsafe(&*routes.get_ptr(matched as ubigint))
            var handler = route.handler
            var cache = route.cache_control.to_view()
            var vary = route.vary.to_view()
            return self.render_live(handler, cache, vary, 200u, req)
        }

        // 3b. A shell route. Same pattern matching as a live route, but the body is
        //     the frozen shell plus one splice per slot: no component work at all.
        //     A pattern may have several variants (`shell_variant`): the first one
        //     whose predicate accepts the request wins, and a plain `shell` route with
        //     the same pattern is the fallback.
        params.clear()
        var shell_matched : int = -1
        var fallback : int = -1
        var fallback_params = vector<pair<string, string>>()
        var si : ubigint = 0
        while(si < shells.size()) {
            var r = unsafe(&*shells.get_ptr(si))
            var pv = r.pattern.to_view()
            var trial = vector<pair<string, string>>()
            if(match_route_pattern(&pv, &path, &mut trial)) {
                if(r.has_when) {
                    var pred = r.when
                    if(pred(req)) {
                        shell_matched = si as int
                        params = trial
                        break
                    }
                } else if(fallback < 0) {
                    fallback = si as int
                    fallback_params = trial
                }
            }
            si = si + 1
        }
        if(shell_matched < 0 && fallback >= 0) {
            shell_matched = fallback
            params = fallback_params
        }
        if(shell_matched >= 0) {
            fill_route_params(req, &params)
            var route_ptr = shells.get_ptr(shell_matched as ubigint)
            var route = unsafe(&mut *route_ptr)
            return self.render_shell(route, req)
        }

        // 4. A pre-rendered page or a content-hashed asset: the site's own table.
        //    A path claimed by a pre-rendered route is an exact artifact lookup.
        var asset = self.serve_artifact(&path_s, req)
        if(asset.status != 404u) {
            return asset
        }

        // 5. The 404 route.
        return self.not_found_response(req)
    }

    /// Serve `path` from the frozen artifacts (`no-cache` + ETag for HTML,
    /// immutable caching for hashed assets). Before `build()` it is the site that
    /// answers 503.
    func serve_artifact(&self, path : &string, req : &http::Request) : http::Response {
        var if_none_match = string_view("")
        var inm = req.headers.get("If-None-Match")
        if(inm is Option.Some) {
            var Some(value) = inm else unsafe { unreachable }
            if_none_match = value
        }
        var accept_encoding = string_view("")
        var enc = req.headers.get("Accept-Encoding")
        if(enc is Option.Some) {
            var Some(value) = enc else unsafe { unreachable }
            accept_encoding = value
        }
        var path_view = path.to_view()
        var out = site.respond_negotiated(path_view, if_none_match, accept_encoding)
        // The body is an artifact: bytes the site froze at `build()` and never moves,
        // so the response can borrow instead of copying (a 100 KB asset used to cost
        // a 100 KB `memcpy` per request). This is the only place in the app that may
        // borrow, because every other body is a `string` local to the render.
        var vary = out.vary
        return unsafe(response_of_borrowed(&out, vary))
    }

    /// Render a live route for this request; `status` lets the 404 route reuse the
    /// same path.
    func render_live(&mut self, handler : PageHandler, cache_control : string_view,
                     vary : string_view, status : u16, req : &http::Request) : http::Response {
        var page = HtmlPage()
        if(!site.attach_dynamic(&mut page)) {
            var reason = site.dynamic_error_message()
            return error_response(500u, &reason)
        }
        page.defaultUniversalSetup()
        handler(&mut page, req)
        var html = site.render_dynamic(&mut page)
        if(html.empty()) {
            // The bundle lacks a component this route renders. Serving the page
            // anyway would fail hydration in the browser, so report it instead.
            var reason = site.dynamic_error_message()
            return error_response(503u, &reason)
        }
        var out = site.dynamic_response(&html, cache_control)
        var res = response_of(&out, vary)
        res.status = status
        return res
    }

    /// Fill a shell route for this request: one `emit` over the frozen segments,
    /// with the values `values(req)` supplies. A missing fill emits nothing (that is
    /// a legitimate "no banner for this user").
    func render_shell(&mut self, route : &mut ShellRoute, req : &http::Request) : http::Response {
        var provider = route.values
        var fills = provider(req)
        var values = vector<string_view>()
        var n = route.shell.slot_count()
        var i : ubigint = 0
        while(i < n) {
            var name = route.shell.slot_name(i)
            var value = fills.get(name)
            values.push_back(value)
            i = i + 1
        }
        var html = route.shell.emit(&values)
        var cache = route.cache_control.to_view()
        var out = site.dynamic_response(&html, cache)
        var vary = route.vary.to_view()
        var res = response_of(&out, vary)
        res.status = 200u
        return res
    }

    func not_found_response(&mut self, req : &http::Request) : http::Response {
        if(has_not_found) {
            var handler = not_found_handler
            var cache = string_view("no-store")
            var vary = string_view("")
            return render_live(handler, cache, vary, 404u, req)
        }
        var res = http::Response()
        res.status = 404u
        var body = string_view("<!DOCTYPE html><html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1></body></html>")
        res.html(&body)
        var no_store = string_view("no-store")
        res.set_header_view("Cache-Control", &no_store)
        return res
    }

    // ── Internals ────────────────────────────────────────────────────────
    /// `assets`, or `<base>/assets` when the app is mounted under a prefix. An
    /// absolute prefix is used as-is by every page, so nested pages do not walk out
    /// of the mount.
    func resolved_asset_prefix(&self) : string {
        var out = string()
        if(!base_path.empty()) {
            var base = base_path.to_view()
            out.append_view(&base)
            out.append('/')
        }
        var prefix = asset_prefix.to_view()
        out.append_view(&prefix)
        return out
    }

    /// The methods an endpoint registered for `path` answers, as an `Allow` header
    /// value (`"GET, POST"`); empty when no endpoint claims the path.
    func allowed_methods(&self, path : &string_view) : string {
        var out = string()
        var i : ubigint = 0
        while(i < endpoints.size()) {
            var e = unsafe(&*endpoints.get_ptr(i))
            var pv = e.pattern.to_view()
            var trial = vector<pair<string, string>>()
            if(match_route_pattern(&pv, path, &mut trial)) {
                var mv = e.method.to_view()
                if(!mv.empty()) {
                    if(!out.empty()) { out.append_view(", ") }
                    out.append_view(&mv)
                }
            }
            i = i + 1
        }
        return out
    }

    func push_route(&mut self, pattern : string_view, handler : PageHandler, per_request : bool,
                    cache_control : string_view, vary : string_view) {
        var p = string()
        p.append_view(&pattern)
        var pv = p.to_view()
        if(!per_request && has_params(&pv)) {
            // A pre-rendered route must name one concrete URL: `{id}` has no value
            // until a request arrives.
            fail_with("a pre-rendered route cannot have a {param} pattern; register it with live()")
            return
        }
        const cookie_vary = string_view("Cookie")
        if(!per_request && vary.contains(&cookie_vary)) {
            // The render hook for sessions: a page whose bytes depend on the session
            // cookie cannot be pre-rendered, because one frozen artifact would be
            // served to every user (and a shared cache would mix them). Refusing here
            // is the alternative to a cache-poisoning bug that only shows up behind a
            // CDN.
            fail_with("a pre-rendered route cannot vary by Cookie; register it with live() and vary=\"Cookie\"")
            return
        }
        var cache_s = string()
        cache_s.append_view(&cache_control)
        var vary_s = string()
        vary_s.append_view(&vary)
        routes.push_back(AppRoute {
            pattern : p,
            handler : handler,
            per_request : per_request,
            cache_control : cache_s,
            vary : vary_s
        })
    }

    /// Warm the bundle (one throwaway render per live route, plus the 404 page),
    /// then render every pre-rendered route into the sink.
    func warm_artifacts(&mut self) : bool {
        // One throwaway request for the build-time renders. A page must not depend
        // on request data to collect its components; a live route that does can
        // miss a component in the bundle, which the render-time refusal reports.
        var req = http::Request()
        var route_req = unsafe(&mut req)

        // 1. Warm pass: a live route renders once into a discarded page so its
        //    components and runtime land in the bundle before it is frozen.
        var i : ubigint = 0
        while(i < routes.size()) {
            var route = unsafe(&*routes.get_ptr(i))
            var per = route.per_request
            if(per) {
                var handler = route.handler
                var warm = HtmlPage()
                if(!site.attach_dynamic(&mut warm)) {
                    fail_with(site.dynamic_error_message())
                    return false
                }
                warm.defaultUniversalSetup()
                handler(&mut warm, route_req)
            }
            i = i + 1
        }
        var s : ubigint = 0
        while(s < shells.size()) {
            var route = unsafe(&*shells.get_ptr(s))
            var handler = route.handler
            var warm = HtmlPage()
            if(!site.attach_dynamic(&mut warm)) {
                fail_with(site.dynamic_error_message())
                return false
            }
            warm.defaultUniversalSetup()
            handler(&mut warm, route_req)
            s = s + 1
        }
        if(has_not_found) {
            var nf = HtmlPage()
            if(!site.attach_dynamic(&mut nf)) {
                fail_with(site.dynamic_error_message())
                return false
            }
            nf.defaultUniversalSetup()
            var handler = not_found_handler
            handler(&mut nf, route_req)
        }

        // 2. Pre-rendered pages: reserve first, because the site holds a pointer to
        //    each page and reallocating the vector would move them.
        var static_count : ubigint = 0
        var j : ubigint = 0
        while(j < routes.size()) {
            var r = unsafe(&*routes.get_ptr(j))
            if(!r.per_request) { static_count = static_count + 1 }
            j = j + 1
        }
        // Allocate every page and every artifact path first: the sink holds a
        // pointer to each page and a view of each path, so neither may move after
        // the first one is handed over.
        var a : ubigint = 0
        while(a < static_count) {
            pages.push_back(HtmlPage())
            a = a + 1
        }
        var b : ubigint = 0
        while(b < routes.size()) {
            var r = unsafe(&*routes.get_ptr(b))
            if(!r.per_request) {
                var pattern = string()
                var src = string_view("")
                unsafe { src = r.pattern.to_view() }
                pattern.append_view(&src)
                var pv = pattern.to_view()
                page_paths.push_back(artifact_path_for(&pv))
            }
            b = b + 1
        }

        var slot : ubigint = 0
        var k : ubigint = 0
        while(k < routes.size()) {
            var route = unsafe(&*routes.get_ptr(k))
            var per = route.per_request
            if(!per) {
                var handler = route.handler
                var page_ptr = pages.get_ptr(slot)
                var page_mut = unsafe(&mut *page_ptr)
                var path_ptr = page_paths.get_ptr(slot)
                var artifact_view = unsafe(path_ptr.to_view())
                site.add(page_mut, artifact_view)
                page_mut.defaultUniversalSetup()
                handler(page_mut, route_req)
                slot = slot + 1
            }
            k = k + 1
        }
        return true
    }

    /// Freeze every shell route, **after** the bundle is frozen: render the route
    /// once against it (the same call a live route makes per request), then record
    /// its slot ranges. A shell a request could not render must fail the build, not
    /// the first visitor.
    func compile_shells(&mut self) : bool {
        if(shells.size() == 0) { return true }
        var req = http::Request()
        var build_req = unsafe(&mut req)
        var i : ubigint = 0
        while(i < shells.size()) {
            var route = unsafe(&mut *shells.get_ptr(i))
            var handler = route.handler
            var page = HtmlPage()
            if(!site.attach_dynamic(&mut page)) {
                fail_with(site.dynamic_error_message())
                return false
            }
            page.defaultUniversalSetup()
            handler(&mut page, build_req)
            var html = site.render_dynamic(&mut page)
            var pattern_view = route.pattern.to_view()
            if(html.empty()) {
                var reason = site.dynamic_error_message()
                fail_with("shell route ")
                fail_with(pattern_view)
                fail_with(" was not compiled: ")
                fail_with(reason)
                return false
            }
            var html_view = html.to_view()
            var compiled = Shell.compile(html_view)
            if(!compiled.is_valid()) {
                var reason = compiled.error_message()
                fail_with("shell route ")
                fail_with(pattern_view)
                fail_with(" has an invalid slot: ")
                fail_with(reason)
                return false
            }
            route.shell = compiled
            route.compiled = true
            i = i + 1
        }
        return true
    }

    func fail_with(&mut self, message : string_view) {
        failed = true
        if(!failure.empty()) {
            failure.append_view("; ")
        }
        failure.append_view(&message)
    }
}

// Copy the captures a pattern match produced into the request, so a handler reads
// `/invoices/{id}` through `req.param("id")`. Used by the live and shell paths
// alike.
func fill_route_params(req : &mut http::Request, params : &std::vector<std::pair<string, string>>) {
    req.route_params.clear()
    var k : ubigint = 0
    while(k < params.size()) {
        var p = params.get_ptr(k)
        var name_v = string_view("")
        var value_v = string_view("")
        unsafe {
            name_v = p.first.to_view()
            value_v = p.second.to_view()
        }
        var name_s = string()
        name_s.append_view(&name_v)
        var value_s = string()
        value_s.append_view(&value_v)
        req.route_params.push_back(pair<string, string> { first : name_s, second : value_s })
        k = k + 1
    }
}

// ---------------------------------------------------------------------------
// A no-op handler: the default for "no 404 route registered".
// ---------------------------------------------------------------------------
func empty_page(page : &mut HtmlPage, req : &http::Request) {
}

// A pre-rendered route's artifact path: `/` -> `index.html`, `/about` ->
// `about/index.html`, so the URL is a directory and the file is its index.
func artifact_path_for(pattern : &string_view) : string {
    var out = string()
    var i : ubigint = 0
    while(i < pattern.size() && pattern.at(i) == '/') { i = i + 1 }
    var n = pattern.size()
    while(n > i && pattern.at(n - 1) == '/') { n = n - 1 }
    if(n > i) {
        var seg = unsafe(string_view(pattern.data() + i, n - i))
        out.append_view(&seg)
        out.append('/')
    }
    out.append_view("index.html")
    return out
}

// True when `route_method` accepts this request: `*` accepts anything, otherwise the
// two are compared case-insensitively (`get` and `GET` are the same method).
func method_matches(route_method : &string_view, wildcard : &string_view, req : &http::Request) : bool {
    if(route_method.equals(wildcard)) { return true }
    var want = req.method.to_view()
    if(route_method.size() != want.size()) { return false }
    var i : ubigint = 0
    while(i < want.size()) {
        if(lower_ascii(route_method.at(i)) != lower_ascii(want.at(i))) { return false }
        i = i + 1
    }
    return true
}

func lower_ascii(c : char) : char {
    if(c >= 'A' && c <= 'Z') { return (c + 32) as char }
    return c
}

// An endpoint's default cache policy, applied only when the handler did not choose
// one. A handler that sets `Cache-Control` (or `Vary`) wins.
func apply_endpoint_policy(res : &mut http::Response, route : &AppEndpoint) {
    var cache = route.cache_control.to_view()
    if(!cache.empty() && !res.headers.contains("Cache-Control")) {
        res.set_header_view("Cache-Control", &cache)
    }
    var vary = route.vary.to_view()
    if(!vary.empty()) {
        res.set_header_view("Vary", &vary)
    }
}

// The path exists for other methods: answer 405 with `Allow` instead of a 404 that
// tells the client nothing.
func method_not_allowed(allow : &string) : http::Response {
    var res = http::Response()
    res.status = 405u
    var allow_view = allow.to_view()
    res.set_header_view("Allow", &allow_view)
    var no_store = string_view("no-store")
    res.set_header_view("Cache-Control", &no_store)
    var body = string_view("405 Method Not Allowed\n")
    res.text(&body)
    return res
}

// True when the pattern contains a `{param}` segment.
func has_params(pattern : &string_view) : bool {
    var i : ubigint = 0
    while(i < pattern.size()) {
        if(pattern.at(i) == '{') { return true }
        i = i + 1
    }
    return false
}

// Exact-path comparison ignoring one trailing slash on either side and leading
// slashes, so `/about` and `/about/` are the same route.
func path_equals(pattern : &string_view, path : &string_view) : bool {
    var plen = trimmed_len(pattern)
    var qlen = trimmed_len(path)
    var pi : ubigint = 0
    var qi : ubigint = 0
    while(pi < plen && pattern.at(pi) == '/') { pi = pi + 1 }
    while(qi < qlen && path.at(qi) == '/') { qi = qi + 1 }
    if(plen - pi != qlen - qi) { return false }
    while(pi < plen) {
        if(pattern.at(pi) != path.at(qi)) { return false }
        pi = pi + 1
        qi = qi + 1
    }
    return true
}

func trimmed_len(s : &string_view) : ubigint {
    var n = s.size()
    while(n > 0 && s.at(n - 1) == '/') { n = n - 1 }
    return n
}

// ── Pattern matching (`/invoices/{id}`, `/files/{path*}`) ──────────────────
func next_seg_end(s : &string_view, from : ubigint) : ubigint {
    var i = from
    while(i < s.size() && s.at(i) != '/') { i = i + 1 }
    return i
}

/// True when `path` matches `pattern`, appending one `(name, value)` pair per
/// captured segment. `{name}` captures one segment, `{name*}` the rest of the
/// path.
public func match_route_pattern(pattern : &string_view, path : &string_view,
                                params : &mut vector<pair<string, string>>) : bool {
    var pi : ubigint = 0
    var qi : ubigint = 0
    while(pi < pattern.size() && pattern.at(pi) == '/') { pi = pi + 1 }
    while(qi < path.size() && path.at(qi) == '/') { qi = qi + 1 }

    while(pi < pattern.size()) {
        var pe = next_seg_end(pattern, pi)
        var qe = next_seg_end(path, qi)
        var pseg = unsafe(string_view(pattern.data() + pi, pe - pi))
        var qseg = unsafe(string_view(path.data() + qi, qe - qi))

        if(pseg.size() > 2 && pseg.at(0) == '{' && pseg.at(pseg.size() - 1) == '}') {
            var name = unsafe(string_view(pseg.data() + 1, pseg.size() - 2))
            var star = name.size() > 0 && name.at(name.size() - 1) == '*'
            if(star) {
                var bare = unsafe(string_view(name.data(), name.size() - 1))
                // The rest of the path, slashes included: a catch-all.
                var rest = unsafe(string_view(path.data() + qi, path.size() - qi))
                push_param(bare, rest, params)
                return true
            }
            push_param(name, qseg, params)
        } else {
            if(!pseg.equals(&qseg)) { return false }
        }

        pi = pe
        qi = qe
        while(pi < pattern.size() && pattern.at(pi) == '/') { pi = pi + 1 }
        while(qi < path.size() && path.at(qi) == '/') { qi = qi + 1 }
    }
    return qi >= path.size()
}

func push_param(name : string_view, value : string_view, params : &mut vector<pair<string, string>>) {
    var n = string()
    n.append_view(&name)
    var v = string()
    v.append_view(&value)
    params.push_back(pair<string, string> { first : n, second : v })
}

// `req.path` minus the mount prefix and the query/fragment. Empty means "not this
// app" (a path outside the mount).
func request_path_without_base(base : &string, raw : string_view) : string {
    var out = string()
    var i : ubigint = 0
    // Cut the query/fragment first: `/a?b` is the route `/a`.
    var n = raw.size()
    while(i < n) {
        var c = raw.at(i)
        if(c == '?' || c == '#') {
            n = i
            break
        }
        i = i + 1
    }
    var path = unsafe(string_view(raw.data(), n))

    if(!base.empty()) {
        var base_view = base.to_view()
        if(!prefix_of(path, base_view)) {
            return out
        }
        var rest = unsafe(string_view(path.data() + base_view.size(), path.size() - base_view.size()))
        path = rest
    }
    if(path.empty()) {
        out.append('/')
    } else {
        out.append_view(&path)
    }
    return out
}

func prefix_of(s : string_view, prefix : string_view) : bool {
    if(prefix.size() > s.size()) { return false }
    var i : ubigint = 0
    while(i < prefix.size()) {
        if(s.at(i) != prefix.at(i)) { return false }
        i = i + 1
    }
    return true
}

// ── Responses ─────────────────────────────────────────────────────────────
/// Map a `SiteResponse` onto an `http::Response`, **copying** the body (the
/// response owns it, the site keeps its buffers). This is the default: a body from
/// `render_dynamic`/`shell.emit` is a `string` local to the render and would dangle
/// if borrowed.
public func response_of(out : &SiteResponse, vary : string_view) : http::Response {
    var res = http::Response()
    res.status = out.status as u16
    var content_type = out.content_type
    var cache_control = out.cache_control
    var etag = out.etag
    var body = out.body
    res.set_header_view("Cache-Control", &cache_control)
    if(!vary.empty()) {
        res.set_header_view("Vary", &vary)
    }
    if(!etag.empty()) {
        res.set_header_view("ETag", &etag)
    }
    var encoding = out.content_encoding
    if(!encoding.empty()) {
        res.set_header_view("Content-Encoding", &encoding)
    }
    if(!body.empty()) {
        res.html(&body)
    }
    // `html()` only sets a content type when absent; the site's own type is more
    // specific (JSON, XML, JS), so it is applied afterwards.
    res.set_header_view("Content-Type", &content_type)
    return res
}

/// The same mapping without the body copy: the response borrows the bytes
/// (`http::ResponseBody.View`). `unsafe` because it asserts something the compiler
/// cannot see — that the bytes outlive the response being written. True for a
/// frozen artifact (the site holds it for the process's life); **false** for any
/// body rendered per request, which lives in a local `string`.
///
/// Answering a 100 KB bundle asset this way is ~2.1x faster than copying it and is
/// the difference between a per-byte serving cost and a per-request one
/// (`scripts/static-site-test.sh` gates both the `view=1` fact and the ratio).
public unsafe func response_of_borrowed(out : &SiteResponse, vary : string_view) : http::Response {
    var res = http::Response()
    res.status = out.status as u16
    var content_type = out.content_type
    var cache_control = out.cache_control
    var etag = out.etag
    var body = out.body
    res.set_header_view("Cache-Control", &cache_control)
    if(!vary.empty()) {
        res.set_header_view("Vary", &vary)
    }
    if(!etag.empty()) {
        res.set_header_view("ETag", &etag)
    }
    var encoding = out.content_encoding
    if(!encoding.empty()) {
        res.set_header_view("Content-Encoding", &encoding)
    }
    if(!body.empty()) {
        res.body = http::ResponseBody.View(body)
    }
    res.set_header_view("Content-Type", &content_type)
    return res
}

/// A JSON response for an already-serialized body (`json::stringify(…)`), with
/// `Content-Type: application/json` and `no-store`.
public func json(status : u16, serialized : &string_view) : http::Response {
    var res = http::response_json_text(status, serialized)
    var no_store = string_view("no-store")
    res.set_header_view("Cache-Control", &no_store)
    return res
}

/// A plain-text response (`text/plain; charset=utf-8`) with `no-store`, the right
/// answer for a rejected form POST (400/403/409). Use `http::response_html` for a
/// 200 page.
public func text(status : u16, body : &string_view) : http::Response {
    var res = http::response_text(status, body)
    var no_store = string_view("no-store")
    res.set_header_view("Cache-Control", &no_store)
    return res
}

/// A redirect with `Location`, `no-store` and no body. `303` is the
/// post/redirect/get answer to a form POST; the route table's `app.redirect` uses
/// `308` to keep the method.
public func redirect(status : u16, location : string_view) : http::Response {
    return redirect_response(status, location)
}

func redirect_response(status : u16, location : string_view) : http::Response {
    var res = http::Response()
    res.status = status
    res.set_header_view("Location", &location)
    var no_store = string_view("no-store")
    res.set_header_view("Cache-Control", &no_store)
    var body = string_view("")
    res.html(&body)
    return res
}

func error_response(status : u16, message : &string_view) : http::Response {
    var res = http::Response()
    res.status = status
    var no_store = string_view("no-store")
    res.set_header_view("Cache-Control", &no_store)
    res.text(message)
    return res
}

// ---------------------------------------------------------------------------
// Process-wide install, so an app serves from an `http::Server` route:
//
//     unsafe { web::install(&raw mut app) }
//     srv.router.add("GET", "/:path*", ||(req) => web::serve_request(req))
//
// One app per process, installed once next to the listener — the same lifetime
// contract as `webserve::install`.
// ---------------------------------------------------------------------------
private var installed : *mut App = null

/// Install the app `serve_request` answers with. `unsafe` because the pointed-to
/// app must outlive every request.
public unsafe func install(app : *mut App) {
    unsafe { installed = app }
}

public func uninstall() {
    unsafe { installed = null }
}

public func is_installed() : bool {
    return unsafe(installed != null)
}

/// The `web::Router` handler form, serving the installed app.
public func serve_request(req : http.Request) : http::Response {
    var request = req
    var app = unsafe(installed)
    if(app == null) {
        var not_ready = string_view("no app installed")
        return error_response(503u, &not_ready)
    }
    var app_mut = unsafe(&mut *app)
    return app_mut.handle(&mut request)
}

} // namespace web
