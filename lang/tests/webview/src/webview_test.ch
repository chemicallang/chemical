using std::string;
using std::string_view;

// These tests exercise the display-independent parts of the webview API
// (struct construction and field setters). The actual window/UI creation
// (webview::create / show / run) requires a graphical display and is not
// covered here.

@test
public func test_webview_defaults(env : &mut TestEnv) {
    var wv = webview::WebView.make()
    if(wv.width != 800) { env.error("default width should be 800"); return }
    if(wv.height != 600) { env.error("default height should be 600"); return }
    if(!string_eq(&raw wv.title, string_view("Chemical WebView"))) {
        env.error("default title mismatch");
        return
    }
    if(wv.initialized) { env.error("webview should not be initialized by default"); return }
}

@test
public func test_webview_set_title_size(env : &mut TestEnv) {
    var wv = webview::WebView.make()
    webview::webview_set_title(&raw mut wv, "My App")
    if(!string_eq(&raw wv.title, string_view("My App"))) {
        env.error("set_title did not update title");
        return
    }
    webview::webview_set_size(&raw mut wv, 1024, 768)
    if(wv.width != 1024) { env.error("set_size did not update width"); return }
    if(wv.height != 768) { env.error("set_size did not update height"); return }
}
