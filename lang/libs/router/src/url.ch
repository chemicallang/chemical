// Server URL hand-off (lang/docs/universal-router-design.md §6.1, §6.3.1).
//
// The app hands the page the raw request URL *before* rendering; matching is
// deferred to the generated router function, which owns the compile-time
// patterns and runs while it renders.
public func (page : &mut HtmlPage) set_route_url(path : std::string_view, base : std::string_view = "") {
    page.add_parameter(std::string_view("__route_url"), path)     // raw; matched at render time
    page.add_parameter(std::string_view("__route_base"), base)
}

public func (page : &mut HtmlPage) get_route_url() : std::string_view {
    return page.get_parameter(std::string_view("__route_url"))
}

public func (page : &mut HtmlPage) get_route_base() : std::string_view {
    return page.get_parameter(std::string_view("__route_base"))
}
