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
//   srv.router.add("GET", "/:path*", ||(req) => {
//       return webserve.respond_to_request(req)
//   })
//   srv.serve()
//
// Two `web::Router` details that bite: the catch-all pattern is `/:name*` (not
// `/*name`), and a handler must be a *lambda* - a bare function reference is
// rejected with `SAFE-SEND-002` ("its capture witness was erased").
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
/// The body is **borrowed**, not copied (`http::ResponseBody.View`): the bytes are
/// the site's frozen artifacts and the site outlives every response, so the encoder
/// writes them straight to the socket. Copying a 100 KB asset per request was the
/// largest serving cost; `scripts/static-site-test.sh` gates the fact (`is_view()`)
/// and the ratio.
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

    // Content negotiation: a gzipped representation is served to a client that
    // accepts it, and `Vary` says so (see `respond_negotiated`).
    var accept_encoding = std::string_view("")
    var enc = req.headers.get("Accept-Encoding")
    if(enc is Option.Some) {
        var Some(value) = enc else unsafe { unreachable }
        accept_encoding = value
    }

    var path_view = req.path.to_view()
    var out = target.respond_negotiated(path_view, if_none_match, accept_encoding)

    res.status = out.status as u16
    var content_type = out.content_type
    res.set_header_view("Content-Type", &content_type)
    var cache_control = out.cache_control
    res.set_header_view("Cache-Control", &cache_control)
    if(!out.etag.empty()) {
        var etag = out.etag
        res.set_header_view("ETag", &etag)
    }
    if(!out.content_encoding.empty()) {
        var encoding = out.content_encoding
        res.set_header_view("Content-Encoding", &encoding)
    }
    if(!out.vary.empty()) {
        var vary = out.vary
        res.set_header_view("Vary", &vary)
    }
    if(out.body.size() > 0) {
        var body = out.body
        // No copy: the body is a view into the site's frozen buffers, and the site
        // outlives every response (`install` asserts exactly that).
        res.body = http::ResponseBody.View(body)
    }
    return res
}

/// The `web.Router` handler form, serving the installed site:
/// `router.add("GET", "/*path", respond_to_request)`.
public func respond_to_request(req : http.Request) : http::Response {
    return serve(unsafe(installed), &req)
}

} // namespace webserve
