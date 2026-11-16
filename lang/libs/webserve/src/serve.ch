// HTTP bridge: serve a built `StaticSite` through `http::Server`.
//
// `StaticSite.build()` produces the whole site in memory and `respond()`
// answers a path with status, headers and a borrowed body. This module maps that
// onto `http::Response`, so a server is a route registration:
//
//   var site = StaticSite.make("out", "app", "assets")
//   // ... add pages, render ...
//   if(!site.build()) { /* site.error_message() */ }
//
//   unsafe { webserve.install(&raw mut site) }
//   var srv = http::Server.bind(cfg)?
//   srv.router.add("GET", "/*path", webserve.respond_to_request)
//   srv.serve()
//
// The site must outlive the server (a process-lifetime build), which is what the
// `unsafe` on `install` asserts.
public namespace webserve {

using std::Option;
using std::string;
using std::string_view;

// The site this process serves. One per process: `install` runs once at startup,
// next to the listener, so the lifetime is the server's.
private var installed : *mut StaticSite = null

/// Install the site that `respond_to_request` serves. `unsafe` because the
/// pointer's target must outlive every request.
public unsafe func install(site : *mut StaticSite) {
    unsafe { installed = site }
}

/// Drop the installed site (the pointer's target is not touched).
public func uninstall() {
    unsafe { installed = null }
}

public func is_installed() : bool {
    return unsafe(installed != null)
}

/// Map one request onto `site`'s artifacts, honouring `If-None-Match`.
///
/// The body is copied into the response because `http::Response` owns it; the
/// site keeps its buffers. That is one copy per request — the alternative, a
/// borrowed body variant in `http`, is not worth the API change yet.
///
/// `site` may be null: the answer is a 503, never a crash.
public func serve(site : *mut StaticSite, req : &http::Request) : http::Response {
    var res = http::Response()
    if(site == null) {
        res.status = 503u
        var not_ready = std::string_view("no static site installed")
        res.text(&not_ready)
        var no_store = std::string_view("no-store")
        res.set_header_view("Cache-Control", &no_store)
        return res
    }

    var target = unsafe(&*site)

    // Conditional request: the client sends back the validator we gave it.
    var if_none_match = std::string_view("")
    var inm = req.headers.get("If-None-Match")
    if(inm is Option.Some) {
        var Some(value) = inm else unsafe { unreachable }
        if_none_match = value
    }

    var path_view = req.path.to_view()
    var out = target.respond_if_none_match(path_view, if_none_match)

    res.status = out.status as u16
    var content_type = out.content_type
    res.set_header_view("Content-Type", &content_type)
    var cache_control = out.cache_control
    res.set_header_view("Cache-Control", &cache_control)
    if(!out.etag.empty()) {
        var etag = out.etag
        res.set_header_view("ETag", &etag)
    }
    if(out.body.size() > 0) {
        var body = string()
        var body_view = out.body
        body.append_view(&body_view)
        res.body = http::ResponseBody.Text(body)
    }
    return res
}

/// The `web.Router` handler form, serving the installed site:
/// `router.add("GET", "/*path", respond_to_request)`.
public func respond_to_request(req : http.Request) : http::Response {
    return serve(unsafe(installed), &req)
}

} // namespace webserve
