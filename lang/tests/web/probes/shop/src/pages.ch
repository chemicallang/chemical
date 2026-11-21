// Probe: a small storefront — the "real app" shape the web stack must support.
//
// One app, three delivery modes: two pre-rendered pages (`/`, `/products`), a
// per-request product page and cart, plus a JSON endpoint in `main.ch`. Mutations
// are POSTs behind CSRF, answered with 303 (post/redirect/get).
//
// The point is to find what the *public* API cannot express. Everything here uses
// only documented public functions; each workaround is recorded in
// `lang/docs/web-stack-gaps.md`.
using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;

// ── Catalog (a database in a real shop) ───────────────────────────────────
// `string_view` fields make the catalog static data: the products are literals, so
// a lookup never copies (or frees) a string.
@direct_init
struct Product {
    var slug : string_view
    var name : string_view
    var price_cents : int
    var stock : int
}

func catalog() : vector<Product> {
    var out = vector<Product>()
    out.push_back(Product { slug : "lamp", name : "Desk lamp", price_cents : 4500, stock : 12 })
    out.push_back(Product { slug : "chair", name : "Oak chair", price_cents : 12900, stock : 3 })
    out.push_back(Product { slug : "mug", name : "Enamel mug", price_cents : 1200, stock : 0 })
    out.push_back(Product { slug : "shelf", name : "Wall shelf", price_cents : 7900, stock : 7 })
    return out
}

func find_product(slug : string_view) : Option<Product> {
    var all = catalog()
    var i : ubigint = 0
    while(i < all.size()) {
        var p = unsafe(&*all.get_ptr(i))
        if(p.slug.equals(&slug)) { return Option.Some<Product>(unsafe(*p)) }
        i = i + 1
    }
    return Option.None<Product>()
}

// ── Money ────────────────────────────────────────────────────────────────
func money(cents : int) : string {
    var out = string()
    var whole = cents / 100
    var frac = cents % 100
    out.append_view("$")
    out.append_integer(whole as bigint)
    out.append_view(".")
    if(frac < 10) { out.append('0') }
    out.append_integer(frac as bigint)
    return out
}

// ── Cart model (session-backed) ──────────────────────────────────────────
@direct_init
struct CartLine {
    var slug : string_view
    var name : string_view
    var unit_cents : int
    var quantity : int
}

// The session holds `slug:qty,slug:qty`. A real app wants a typed session value
// and a `web::Form`; both are gaps.
func cart_lines(ctx : &web::RequestContext) : vector<CartLine> {
    var out = vector<CartLine>()
    var text = ctx.session_text("cart")
    if(text.empty()) { return out }
    var items = text.to_view().split(',')
    var i : ubigint = 0
    while(i < items.size()) {
        var item = unsafe(&*(items.get_ptr(i)))
        var parts = item.split(':')
        if(parts.size() >= 2) {
            var slug = unsafe(&*parts.get_ptr(0))
            var qty = 1
            var qty_field = unsafe(&*parts.get_ptr(1))
            var parsed = qty_field.to_view().to_int()
            if(parsed is Result.Ok) {
                var Ok(n) = parsed else unsafe { unreachable }
                qty = n
            }
            var found = find_product(slug.to_view())
            if(found is Option.Some) {
                var Some(p) = found else unsafe { unreachable }
                out.push_back(CartLine { slug : p.slug, name : p.name, unit_cents : p.price_cents, quantity : qty })
            }
        }
        i = i + 1
    }
    return out
}

func cart_total(lines : &vector<CartLine>) : int {
    var total = 0
    var i : ubigint = 0
    while(i < lines.size()) {
        var line = unsafe(&*lines.get_ptr(i))
        total = total + line.unit_cents * line.quantity
        i = i + 1
    }
    return total
}

// ── Pages ────────────────────────────────────────────────────────────────
public func Home(ctx : &mut web::RequestContext, page : &mut HtmlPage) {
    page.appendTitle("Probe Supply — home")
    #html {
        <main class="home">
            <h1>Objects for small rooms</h1>
            <p class="lede">Four things, made well, shipped from one warehouse.</p>
            <a class="cta" href="/products">Browse products</a>
        </main>
    }
}

public func Products(ctx : &mut web::RequestContext, page : &mut HtmlPage) {
    page.appendTitle("Probe Supply — products")
    var all = catalog()
    // `@{ ... }` holds exactly one statement, so the loop counter lives here.
    var i : ubigint = 0
    #html {
        <main class="products">
            <h1>Products</h1>
            <ul class="grid">
                @{
                    while(i < all.size()) {
                        var p = unsafe(&*all.get_ptr(i))
                        var name = p.name
                        var price = money(p.price_cents)
                        var price_view = price.to_view()
                        var href = string("/product/")
                        href.append_view(&p.slug)
                        var href_view = href.to_view()
                        #html {
                            <li class="card">
                                <a href={href_view}>{name}</a>
                                <span class="price">{price_view}</span>
                            </li>
                        }
                        i = i + 1
                    }
                }
            </ul>
        </main>
    }
}

public func ProductPage(ctx : &mut web::RequestContext, page : &mut HtmlPage) {
    var slug = string_view("")
    var got = ctx.param("slug")
    if(got is Option.Some) {
        var Some(v) = got else unsafe { unreachable }
        slug = v
    }
    var found = find_product(slug)
    if(found is Option.None) {
        page.appendTitle("Probe Supply — not found")
        #html { <main><h1>No such product</h1><a href="/products">Back</a></main> }
        return
    }
    var Some(product) = found else unsafe { unreachable }
    page.appendTitle("Probe Supply — product")
    var name = product.name
    var price = money(product.price_cents)
    var price_view = price.to_view()
    var in_stock = product.stock > 0
    var stock_text = string("In stock: ")
    stock_text.append_integer(product.stock as bigint)
    var stock_view = stock_text.to_view()
    var token_view = ctx.csrf_token()
    var action = string("/cart/add?slug=")
    action.append_view(&slug)
    var action_view = action.to_view()
    #html {
        <main class="product">
            <h1>{name}</h1>
            <p class="price">{price_view}</p>
            <p class="stock">{stock_view}</p>
            @{
                if(in_stock) {
                    #html {
                        <form method="post" action={action_view}>
                            <input type="hidden" name="csrf" value={token_view} />
                            <button type="submit">Add to cart</button>
                        </form>
                    }
                } else {
                    #html { <p class="sold-out">Sold out</p> }
                }
            }
            <a href="/products">All products</a>
        </main>
    }
}

public func CartPage(ctx : &mut web::RequestContext, page : &mut HtmlPage) {
    page.appendTitle("Probe Supply — cart")
    var lines = cart_lines(ctx)
    var total = cart_total(&lines)
    var total_text = money(total)
    var total_view = total_text.to_view()
    var token_view = ctx.csrf_token()
    var line_index : ubigint = 0
    #html {
        <main class="cart">
            <h1>Your cart</h1>
            @{
                if(lines.empty()) {
                    #html { <p class="empty">Nothing here yet.</p> }
                } else {
                    #html {
                        <ul class="lines">
                            @{
                                while(line_index < lines.size()) {
                                    var line = unsafe(&*lines.get_ptr(line_index))
                                    var name = line.name
                                    var qty_text = string("x")
                                    qty_text.append_integer(line.quantity as bigint)
                                    var qty_view = qty_text.to_view()
                                    var subtotal = money(line.unit_cents * line.quantity)
                                    var subtotal_view = subtotal.to_view()
                                    #html {
                                        <li>
                                            <span class="name">{name}</span>
                                            <span class="qty">{qty_view}</span>
                                            <span class="subtotal">{subtotal_view}</span>
                                        </li>
                                    }
                                    line_index = line_index + 1
                                }
                            }
                        </ul>
                        <p class="total">Total {total_view}</p>
                        <form method="post" action="/checkout">
                            <input type="hidden" name="csrf" value={token_view} />
                            <button type="submit">Checkout</button>
                        </form>
                    }
                }
            }
            <a href="/products">Keep shopping</a>
        </main>
    }
}

public func Thanks(ctx : &mut web::RequestContext, page : &mut HtmlPage) {
    page.appendTitle("Probe Supply — thanks")
    #html { <main class="thanks"><h1>Thank you</h1><p>Your order is on its way.</p></main> }
}

public func Missing(ctx : &mut web::RequestContext, page : &mut HtmlPage) {
    page.appendTitle("Probe Supply — not found")
    #html { <main class="missing"><h1>Not found</h1><a href="/">Home</a></main> }
}
