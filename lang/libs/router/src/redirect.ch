// Server-side redirects (lang/docs/universal-router-design.md §6.7, §6.7-F).
//
// A redirect is *not* a route-body construct (that would fight the guard model);
// it belongs in the request handler. `redirect_to` selects the target route on
// `router` — an `add_parameter` under the hood, exactly what a URL match would
// do — so the page, if it renders, already shows and (client-side) activates the
// target instead of the route the request URL matched.
//
// The handler decides the response: either return a real 302/303 with
// `Location: page.get_route_redirect()` (the common case, no render at all), or
// let the page render in place. Passing the target `path` makes the in-place
// case coherent: it is stored as `__route_url`, so the generated matcher selects
// the target route too rather than re-selecting the old one from the request URL.

// Selects `id` on `router`. When `path` is non-empty it is also recorded so the
// matcher follows it and `get_route_redirect()` can feed a `Location` header.
public func (page : &mut HtmlPage) redirect_to(router : std::string_view, id : std::string_view,
                                               path : std::string_view = "") {
    page.add_parameter(router, id)
    if(path.size() > 0) {
        // Page-owned copies: `path` may be computed at runtime and the key is
        // read back by the generated matcher via `get_parameter`.
        page.add_parameter_owned(std::string_view("__route_url"), path)
        page.add_parameter_owned(std::string_view("__route_redirect"), path)
    }
}

// The target path handed to `redirect_to` ("" when this is not a redirect).
public func (page : &mut HtmlPage) get_route_redirect() : std::string_view {
    return page.get_parameter(std::string_view("__route_redirect"))
}
