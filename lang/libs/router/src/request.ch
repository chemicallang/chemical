// Request-context extension (lang/docs/universal-router-design.md §3.3).
//
// `set_request` stores a pointer to a caller-owned `RouteRequest` on the page
// parameter store; the struct must outlive the render (§3.2 lifetime contract).
using std::string_view;

@direct_init
public struct RouteRequest {
    var method : std::string_view       // "GET"
    var path : std::string_view         // "/projects/42"
    var query : std::string_view        // raw query string, "" if none
    var body : std::string_view         // "" unless read by the caller
}

public func (page : &mut HtmlPage) set_request(req : &RouteRequest) {
    page.add_parameter_object(std::string_view("__request"), req as *mut void)
}

public func get_request(page : &mut HtmlPage) : *mut RouteRequest {
    return get_parameter_object<RouteRequest>(page, std::string_view("__request"))
}
