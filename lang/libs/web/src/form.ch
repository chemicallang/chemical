// A form POST, from the request to a field lookup.
//
// `http::Form` is the parser; this is the request-shaped way to get one:
//
//   app.endpoint("POST", "/cart/add", AddToCart)
//
//   public func AddToCart(req : &http::Request) : http::Response {
//       var f = web::form(req)
//       var sku = f.get("sku")
//       if(sku is Option.None) { return web::error(400u, "missing sku") }
//       ...
//   }
//
// A request with another content type (JSON, multipart) yields an empty form, so a
// handler that accepts several encodings branches on `req.is_content_type(...)`
// before calling this.
public namespace web {

using std::Option;
using std::string;
using std::string_view;

/// The request's `application/x-www-form-urlencoded` body. A body sent without a
/// `Content-Type` is parsed too (some clients omit it); any other media type is an
/// empty form rather than a parse of binary bytes.
public func form(req : &http::Request) : http::Form {
    return parse_request_form(req)
}

/// The same decision, for a caller that already holds the request
/// (`RequestContext.form`): a non-urlencoded body is an empty form, never a
/// mis-parse of binary bytes.
func parse_request_form(req : &http::Request) : http::Form {
    var ct = req.content_type()
    var ct_view = ct.to_view()
    if(!ct_view.empty()) {
        var want = string_view("application/x-www-form-urlencoded")
        if(!ct_view.equals(&want)) {
            return http::parse_form(string_view(""))
        }
    }
    return http::parse_form(req.body_text())
}

} // namespace web
