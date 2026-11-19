// Embedded-bundle fixture, **phase 2: the server**.
//
// This program contains no site, no page and no component: the only bytes it can
// serve are the ones the previous build compiled in (`assets_gen.ch`). That is the
// whole point — if a byte only existed in the output directory, this program could
// not answer.
//
// It registers every generated artifact into a `Bundle` and answers a few
// requests, so `scripts/web-embedded-test.sh` can compare each body with the file
// the build wrote.
using std::string;
using std::string_view;

func report(path : string_view, resp : &SiteResponse) {
    var body = resp.body
    var ct = resp.content_type
    var cc = resp.cache_control
    var etag = resp.etag
    unsafe {
        printf("request %.*s -> %d | %.*s | %.*s | %llu B | etag=%.*s\n",
            path.size() as int, path.data(), resp.status,
            ct.size() as int, ct.data(),
            cc.size() as int, cc.data(),
            body.size(), etag.size() as int, etag.data())
        printf("---body %.*s---\n", path.size() as int, path.data())
        printf("%.*s", body.size() as int, body.data())
        printf("\n---end---\n")
    }
}

public func main() : int {
    var bundle = Bundle.make()
    var count = embedded_asset_count()
    var i : ubigint = 0
    while(i < count) {
        var path = embedded_asset_path(i)
        var bytes = embedded_asset_bytes(i)
        var ty = embedded_asset_content_type(i)
        if(!bundle.add_owned(path, bytes, ty, embedded_asset_immutable(i))) {
            unsafe { printf("consumer: %s\n", bundle.error_message().data()) }
            return 1
        }
        i = i + 1
    }
    unsafe { printf("embedded artifacts=%llu\n", bundle.count()) }

    var home = bundle.respond(string_view("/"))
    report(string_view("/"), &home)
    var stats = bundle.respond(string_view("/stats/"))
    report(string_view("/stats/"), &stats)

    // Conditional requests and the directory form of a route.
    var etag_view = home.etag
    var conditional = bundle.respond_if_none_match(string_view("/"), etag_view)
    report(string_view("/ conditional"), &conditional)
    var bare = bundle.respond(string_view("/stats"))
    unsafe {
        printf("stats bare -> %d\n", bare.status)
    }

    // A content-hashed asset is immutable; an unknown path is a 404.
    var asset_path = embedded_asset_path(0)
    var asset = bundle.respond(asset_path)
    report(string_view("/first-asset"), &asset)
    var missing = bundle.respond(string_view("/nope"))
    unsafe { printf("missing -> %d\n", missing.status) }
    return 0
}
