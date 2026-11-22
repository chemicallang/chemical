// Probe: storefront driver — route table, sessions, POST mutations, JSON, and an
// in-process request suite that prints machine-checkable lines.
//
// What the API forces an app author to write by hand today:
//   * `web::App` routes by **path only** (`App.handle` never looks at `req.method`)
//     and a page handler can only write HTML, so it cannot answer 303/JSON/403;
//   * a `PageHandler` gets `(page, req)` and nothing else — the resolved session is
//     published through a pointer to a request-local so handlers can read it;
//   * `application/x-www-form-urlencoded` has no parser anywhere in the stack;
//   * a request cannot be constructed in a test (`RequestBody` cannot be filled
//     with bytes), so the body travels as a parameter.
// Each of these is a gap in `lang/docs/web-stack-gaps.md`.
using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;

// ── Small helpers ─────────────────────────────────────────────────────────
func string_of(v : string_view) : string {
    var out = string()
    out.append_view(&v)
    return out
}

func header_text(res : &http::Response, name : &string_view) : string {
    var i : size_t = 0
    while(i < res.headers.len()) {
        var n = res.headers.name_at(i)
        if(n.equals(name)) {
            return string_of(res.headers.value_at(i))
        }
        i = i + 1
    }
    return string()
}

func body_text(res : &http::Response) : string {
    var text = res.body.text()
    if(text is Option.Some) {
        var Some(v) = text else unsafe { unreachable }
        return string_of(v)
    }
    return string()
}

func has_text(haystack : &string, needle : string_view) : bool {
    return haystack.to_view().contains(&needle)
}

// The parser fills `req.query`, but a `Request` built by a test has only a path.
// Returns an owned string: a view into a parsed pair would dangle.
func query_of(ctx : &web::RequestContext, name : string_view) : string {
    var got = ctx.query(name)
    if(got is Option.Some) {
        var Some(v) = got else unsafe { unreachable }
        return string_of(v)
    }
    var path = ctx.path()
    var qmark = path.find(&string_view("?"))
    if(qmark is Option.None) { return string() }
    var Some(pos) = qmark else unsafe { unreachable }
    var query = path.subview(pos + 1, path.size())
    var pairs = query.split('&')
    var i : ubigint = 0
    while(i < pairs.size()) {
        var pair = unsafe(&*(pairs.get_ptr(i)))
        var parts = pair.split('=')
        if(parts.size() >= 2) {
            var key = unsafe(&*parts.get_ptr(0))
            if(key.to_view().equals(&name)) {
                // the parser percent-decodes; this fallback must too
                var value = unsafe(&*parts.get_ptr(1))
                var value_view = value.to_view()
                return http::url_decode(&value_view)
            }
        }
        i = i + 1
    }
    return string()
}

// ── Mutations ─────────────────────────────────────────────────────────────
// The cart lives in the session; the handler's `ctx` is the only session
// dependency, so nothing here touches a store or a global.
func add_to_cart(ctx : &mut web::RequestContext, slug : string_view) {
    var existing = ctx.session_text("cart")
    var next = string()
    var added = false
    if(!existing.empty()) {
        var items = existing.to_view().split(',')
        var i : ubigint = 0
        while(i < items.size()) {
            var item = unsafe(&*(items.get_ptr(i)))
            var parts = item.split(':')
            if(parts.size() >= 2) {
                var key = unsafe(&*parts.get_ptr(0))
                var qty = 1
                var qty_field = unsafe(&*parts.get_ptr(1))
                var parsed = qty_field.to_view().to_int()
                if(parsed is Result.Ok) {
                    var Ok(n) = parsed else unsafe { unreachable }
                    qty = n
                }
                if(key.to_view().equals(&slug)) {
                    qty = qty + 1
                    added = true
                }
                if(next.size() > 0) { next.append(',') }
                next.append_string(key)
                next.append(':')
                next.append_integer(qty as bigint)
            }
            i = i + 1
        }
    }
    if(!added) {
        if(next.size() > 0) { next.append(',') }
        next.append_view(&slug)
        next.append_view(":1")
    }
    var next_view = next.to_view()
    ctx.set_session("cart", next_view)
}

func clear_cart(ctx : &mut web::RequestContext) {
    ctx.remove_session("cart")
}

// ── Method-aware endpoints ────────────────────────────────────────────────
// Registered with `app.endpoint("POST", …)`, so the route table answers the
// method. The handler gets the context, which is where CSRF lives.
func csrf_ok(ctx : &web::RequestContext, form : &http::Form) : bool {
    var token = form.get("csrf")
    if(token is Option.None) { return false }
    var Some(t) = token else unsafe { unreachable }
    return ctx.verify_csrf(t)
}

public func CartAdd(ctx : &mut web::RequestContext) : http::Response {
    var form = ctx.form()
    if(!csrf_ok(ctx, &form)) { return web::text(403u, &string_view("bad csrf")) }
    var slug = form.get("slug")
    if(slug is Option.None) {
        var from_query = query_of(ctx, string_view("slug"))
        var qv = from_query.to_view()
        if(qv.empty()) { return web::text(400u, &string_view("slug required")) }
        add_to_cart(ctx, qv)
        return web::redirect(303u, string_view("/cart"))
    }
    var Some(value) = slug else unsafe { unreachable }
    add_to_cart(ctx, value)
    return web::redirect(303u, string_view("/cart"))
}

public func CartClear(ctx : &mut web::RequestContext) : http::Response {
    var form = ctx.form()
    if(!csrf_ok(ctx, &form)) { return web::text(403u, &string_view("bad csrf")) }
    clear_cart(ctx)
    return web::redirect(303u, string_view("/cart"))
}

public func Checkout(ctx : &mut web::RequestContext) : http::Response {
    var form = ctx.form()
    if(!csrf_ok(ctx, &form)) { return web::text(403u, &string_view("bad csrf")) }
    clear_cart(ctx)
    return web::redirect(303u, string_view("/thanks"))
}

public func CartJson(ctx : &mut web::RequestContext) : http::Response {
    var out = string("{\"cart\":\"")
    var cart = ctx.session_text("cart")
    out.append_view(cart.to_view())
    out.append_view("\"}")
    var view = out.to_view()
    return web::json(200u, &view)
}

// A search page: the only place user input reaches the HTML. `#html` does NOT
// auto-escape `{value}` (page.ch documents the choice), so every untrusted value
// must go through `escape_html_view` first. This is the escaping contract an app
// author has to know, and the reason `web` needs a safe-by-default story.
public func Search(ctx : &mut web::RequestContext, page : &mut HtmlPage) {
    var q_text = query_of(ctx, string_view("q"))
    var escaped = escape_html_view(q_text.to_view())
    var q = escaped.to_view()
    page.appendTitle("Probe Supply — search")
    #html {
        <main class="search">
            <h1>Search</h1>
            <p class="query">You searched for: {q}</p>
            <form method="get" action="/search">
                <input type="text" name="q" value={q} />
                <button type="submit">Search</button>
            </form>
        </main>
    }
}

// ── Suite plumbing ────────────────────────────────────────────────────────
func request_with(app : &mut web::App, method : string_view, path : string_view,
                   cookie : string_view, body : string_view) : http::Response {
    // `http::request` is the documented way to build one: the query string is parsed
    // off the path, and the body/cookie are one call each.
    var req = http::request(method, path)
    if(!cookie.empty()) { req.with_cookie(cookie) }
    if(!body.empty()) { req.with_form(body) }
    return app.handle(&raw mut req)
}

func report(label : string_view, res : &http::Response) {
    var body = body_text(res)
    var view = body.to_view()
    unsafe {
        printf("route %.*s | %d | %zu bytes\n", label.size() as int, label.data(),
            res.status as int, view.size())
    }
}

func check(name : string_view, ok : bool) {
    var n = name
    if(ok) {
        unsafe { printf("ok   %.*s\n", n.size() as int, n.data()) }
    } else {
        unsafe { printf("FAIL %.*s\n", n.size() as int, n.data()) }
    }
}

// The CSRF token out of a rendered form (`name="csrf" value="…"`).
func csrf_of(res : &http::Response) : string {
    var body = body_text(res)
    var view = body.to_view()
    var needle = string_view("name=\"csrf\" value=\"")
    var at = view.find(&needle)
    if(at is Option.None) { return string() }
    var Some(pos) = at else unsafe { unreachable }
    var start = pos + needle.size()
    var end = start
    while(end < view.size() && view.at(end) != '"') { end = end + 1 }
    return string_of(view.subview(start, end))
}

func form_text(name : string_view, value : string_view) : string {
    var out = string()
    out.append_view(&name)
    out.append('=')
    out.append_view(&value)
    return out
}

// `Set-Cookie: name=value; Path=/; …` → what a client sends back: `name=value`.
func cookie_pair(cookie : &string) : string_view {
    var view = cookie.to_view()
    var semi = view.find(&string_view(";"))
    if(semi.is_some()) {
        var Some(pos) = semi else unsafe { unreachable }
        return view.subview(0, pos)
    }
    return view
}

public func main() : int {
    var app = web::App.make("lang/compiled/shop_probe", "shop", "assets")
    app.set_minify(true)
    app.set_gzip(true)
    app.page("/", Home)
    app.page("/products", Products)
    app.live("/product/{slug}", ProductPage)
    app.live("/cart", CartPage)
    app.live("/thanks", Thanks)
    app.live("/search", Search)
    app.endpoint("POST", "/cart/add", CartAdd)
    app.endpoint("POST", "/cart/clear", CartClear)
    app.endpoint("POST", "/checkout", Checkout)
    app.endpoint("GET", "/api/cart", CartJson)
    app.redirect("/store", "/products")
    app.not_found(Missing)
    app.export_sitemap("https://shop.example", true)

    var built = app.build()
    var msg = app.error_message()
    unsafe { printf("build=%d msg=%.*s\n", built as int, msg.size() as int, msg.data()) }
    if(!built) { return 1 }

    // Capture the numbers as values: `static_site()` borrows the app, and the
    // later `finish()` needs it mutably.
    var site_artifacts = app.static_site().artifact_count()
    var site_pages = app.static_site().page_count()
    var bundle_name = string_of(app.static_site().bundle_js_url())
    var css_name = string_of(app.static_site().bundle_css_url())
    var bundle_view = bundle_name.to_view()
    var css_view = css_name.to_view()
    unsafe {
        printf("site artifacts=%zu pages=%zu bundle=%.*s css=%.*s gzip=%d\n",
            site_artifacts, site_pages,
            bundle_view.size() as int, bundle_view.data(),
            css_view.size() as int, css_view.data(),
            app.is_gzip_enabled() as int)
    }

    // 1. Pre-rendered home: frozen bytes, ETag, no-cache.
    var home = request_with(&mut app, string_view("GET"), string_view("/"), string_view(""), string_view(""))
    report(string_view("/"), &home)
    var etag = header_text(&home, &string_view("ETag"))
    var home_body = body_text(&home)
    check(string_view("home is 200 with an ETag"), home.status == 200u && etag.size() > 0)
    var home_cc = header_text(&home, &string_view("Cache-Control"))
    var home_cc_view = home_cc.to_view()
    check(string_view("home is no-cache"), home_cc_view.contains(&string_view("no-cache")))
    check(string_view("home renders the hero"), has_text(&home_body, string_view("Objects for small rooms")))
    check(string_view("a component-free page carries no script at all"),
        !has_text(&home_body, string_view("<script")) && !has_text(&home_body, string_view("assets/shop.")))

    // 2. Pre-rendered list.
    var products = request_with(&mut app, string_view("GET"), string_view("/products"), string_view(""), string_view(""))
    report(string_view("/products"), &products)
    var products_body = body_text(&products)
    check(string_view("products lists the catalog"),
        has_text(&products_body, string_view("Oak chair")) && has_text(&products_body, string_view("$129.00")))

    // 3. Per-request product page.
    var lamp = request_with(&mut app, string_view("GET"), string_view("/product/lamp"), string_view(""), string_view(""))
    report(string_view("/product/lamp"), &lamp)
    var lamp_body = body_text(&lamp)
    var lamp_cc = header_text(&lamp, &string_view("Cache-Control"))
    var lamp_cc_view = lamp_cc.to_view()
    check(string_view("product page is no-store"), lamp_cc_view.contains(&string_view("no-store")))
    check(string_view("product page has a CSRF field"), has_text(&lamp_body, string_view("name=\"csrf\"")))
    check(string_view("product page has the price"), has_text(&lamp_body, string_view("$45.00")))
    var cookie = header_text(&lamp, &string_view("Set-Cookie"))
    check(string_view("the first request sets a session cookie"), cookie.size() > 0)

    // 4. Unknown product: a page handler cannot set 404, so it answers 200.
    var unknown = request_with(&mut app, string_view("GET"), string_view("/product/nope"), string_view(""), string_view(""))
    report(string_view("/product/nope"), &unknown)
    var unknown_body = body_text(&unknown)
    check(string_view("unknown product answers 200: no status control in a page handler"),
        unknown.status == 200u && has_text(&unknown_body, string_view("No such product")))

    // 5. Add to cart: CSRF from the page, cookie from the same response.
    var token = csrf_of(&lamp)
    var pair = cookie_pair(&cookie)
    var body1 = form_text(string_view("csrf"), token.to_view())
    var body1_view = body1.to_view()
    var add = request_with(&mut app, string_view("POST"), string_view("/cart/add?slug=lamp"), pair, body1_view)
    report(string_view("POST /cart/add"), &add)
    check(string_view("add-to-cart answers 303"), add.status == 303u)
    var add_location = header_text(&add, &string_view("Location"))
    var add_location_view = add_location.to_view()
    check(string_view("add-to-cart redirects to the cart"), add_location_view.equals(&string_view("/cart")))

    // 6. The cart reads the session.
    var cart = request_with(&mut app, string_view("GET"), string_view("/cart"), pair, string_view(""))
    report(string_view("/cart"), &cart)
    var cart_body = body_text(&cart)
    check(string_view("cart shows the added line"), has_text(&cart_body, string_view("Desk lamp")))
    check(string_view("cart totals the line"), has_text(&cart_body, string_view("$45.00")))

    // 7. A second add accumulates.
    var first_view = body1.to_view()
    add = request_with(&mut app, string_view("POST"), string_view("/cart/add?slug=lamp"), pair, first_view)
    cart = request_with(&mut app, string_view("GET"), string_view("/cart"), pair, string_view(""))
    var cart_body2 = body_text(&cart)
    check(string_view("a second add accumulates"), has_text(&cart_body2, string_view("$90.00")))

    // 8. Wrong CSRF is refused.
    var forged_body = form_text(string_view("csrf"), string_view("deadbeef"))
    var forged_view = forged_body.to_view()
    var forged = request_with(&mut app, string_view("POST"), string_view("/cart/add?slug=mug"), pair, forged_view)
    check(string_view("a forged CSRF token is refused"), forged.status == 403u)

    // 9. JSON endpoint.
    var api = request_with(&mut app, string_view("GET"), string_view("/api/cart"), pair, string_view(""))
    report(string_view("/api/cart"), &api)
    var api_body = body_text(&api)
    check(string_view("the JSON endpoint answers the cart"),
        api.status == 200u && has_text(&api_body, string_view("lamp:2")))
    var api_type = header_text(&api, &string_view("Content-Type"))
    check(string_view("the JSON endpoint sets application/json"),
        api_type.to_view().equals(&string_view("application/json")))

    // 9b. A method the path does not answer is 405 with `Allow`, not 404 — the
    //     route table owns the method now, so the app does not fake it.
    var wrong_method = request_with(&mut app, string_view("GET"), string_view("/cart/add"), pair, string_view(""))
    report(string_view("GET /cart/add"), &wrong_method)
    var allow = header_text(&wrong_method, &string_view("Allow"))
    check(string_view("a wrong method answers 405 with Allow"),
        wrong_method.status == 405u && has_text(&allow, string_view("POST")))

    // 9c. Form decoding: `+` is a space and `%XX` is a byte, in the shared parser.
    var form_probe = http::parse_form(string_view("q=a+b%2Bc&flag"))
    var q_opt = form_probe.get("q")
    var q_text = string()
    if(q_opt is Option.Some) {
        var Some(qv) = q_opt else unsafe { unreachable }
        q_text = string_of(qv)
    }
    check(string_view("a form decodes + and %XX"),
        q_text.to_view().equals(&string_view("a b+c")) && form_probe.has("flag"))

    // 10. Checkout clears the cart.
    var checkout = request_with(&mut app, string_view("POST"), string_view("/checkout"), pair, first_view)
    check(string_view("checkout answers 303"), checkout.status == 303u)
    cart = request_with(&mut app, string_view("GET"), string_view("/cart"), pair, string_view(""))
    var cart_body3 = body_text(&cart)
    check(string_view("checkout empties the cart"), has_text(&cart_body3, string_view("Nothing here yet")))

    // 11. Escaping: a query parameter must not become markup.
    var xss = request_with(&mut app, string_view("GET"),
        string_view("/search?q=%3Cscript%3Ealert(1)%3C%2Fscript%3E"), pair, string_view(""))
    report(string_view("/search"), &xss)
    var xss_body = body_text(&xss)
    check(string_view("a query parameter is escaped into the page"),
        has_text(&xss_body, string_view("&lt;script&gt;")) && !has_text(&xss_body, string_view("<script>alert")))

    // 12. Redirect table and the 404 route.
    var moved = request_with(&mut app, string_view("GET"), string_view("/store"), string_view(""), string_view(""))
    var moved_location = header_text(&moved, &string_view("Location"))
    var moved_location_view = moved_location.to_view()
    check(string_view("the redirect table answers 308"),
        moved.status == 308u && moved_location_view.equals(&string_view("/products")))
    var missing = request_with(&mut app, string_view("GET"), string_view("/no-such-page"), string_view(""), string_view(""))
    check(string_view("an unknown path answers the 404 route"), missing.status == 404u)

    // 13. Bench mode: `SHOP_BENCH=N` renders N product pages and N/10 carts.
    var bench_name = string_view("SHOP_BENCH")
    var bench_env = std::get_env(&bench_name)
    var bench_count = 200
    if(bench_env is Option.Some) {
        var Some(raw) = bench_env else unsafe { unreachable }
        var parsed = raw.to_view().to_int()
        if(parsed is Result.Ok) {
            var Ok(n) = parsed else unsafe { unreachable }
            bench_count = n
        }
    }
    var i = 0
    var bench_cart = 0
    while(i < bench_count) {
        var r = request_with(&mut app, string_view("GET"), string_view("/product/lamp"), pair, string_view(""))
        var keep = r.status
        if(i % 10 == 0) {
            var r2 = request_with(&mut app, string_view("GET"), string_view("/cart"), pair, string_view(""))
            var keep2 = r2.status
            bench_cart = bench_cart + 1
        }
        i = i + 1
    }
    unsafe { printf("bench product=%d cart=%d\n", bench_count, bench_cart) }

    // The static export: `build()` renders into memory, `finish()` writes the
    // artifact set (assets + pre-rendered pages + sitemap/robots).
    var finished = app.finish()
    var finish_msg = app.error_message()
    unsafe {
        printf("finish=%d msg=%.*s artifacts=%zu\n", finished as int, finish_msg.size() as int,
            finish_msg.data(), site_artifacts)
    }
    return 0
}
