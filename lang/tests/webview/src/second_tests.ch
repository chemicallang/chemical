using std::string
using std::string_view

// ---------------------------------------------------------------------------
// Second webview test file.
//
// These tests live in a separate file (next to webview_test.ch) on purpose so
// they can be added/removed without touching the existing suite — handy when
// collaborating to avoid merge churn.
//
// `test_webview_bind_before_init` is display-independent and runs everywhere.
// `test_webview_bind_roundtrip` creates a real window and exercises the
// JS<->native bridge end to end; it needs a graphical display and quits itself
// as soon as the bound handler fires (no manual interaction required).
// ---------------------------------------------------------------------------

// Module-level state is kept non-destructible (bools / pointers) to avoid the
// top-level-destructor restriction on `string`.
unsafe var g_method_ok : bool
unsafe var g_args_ok : bool
unsafe var g_wv : *mut webview::WebView

@test
public func test_webview_bind_before_init(env : &mut TestEnv) {
    // Binding before the webview is initialized (web_view == null) must fail.
    var wv = webview::WebView.make()
    var res = webview::webview_bind(&raw mut wv, (m, a) => string(""))
    if(res is std::Result.Ok) {
        env.error("webview_bind should fail when the webview is not initialized")
        return
    }
}

@test
public func test_webview_bind_roundtrip(env : &mut TestEnv) {
    g_method_ok = false
    g_args_ok = false
    g_wv = null

    var wv_result = webview::create("WebView Bind Test\0" as *char, 400, 300)
    if(wv_result is std::Result.Err) {
        env.error("webview::create failed (is a display available?)")
        return
    }
    var Ok(wv) = wv_result else unreachable
    g_wv = &raw mut wv

    var bind_result = webview::webview_bind(&raw mut wv, (method, args) => {
        g_method_ok = method.find(string_view::make_no_len("ping")) == 0 && method.size() == 4
        g_args_ok = args.find(string_view::make_no_len("42")) != args.size() &&
                    args.find(string_view::make_no_len("hello")) != args.size()
        if(g_wv != null) {
            webview::webview_stop(g_wv)
        }
        return string("{\"ok\":true}")
    })
    if(bind_result is std::Result.Err) {
        env.error("webview_bind failed")
        webview::webview_destroy(&raw mut wv)
        return
    }

    // The page calls the bridge on load. JSON.stringify keeps the args free of
    // raw double quotes so the HTML attribute stays valid.
    var html = "<html><body><script>window.webview_bridge.call('ping', JSON.stringify({n:42, msg:'hello'}));</script></body></html>\0" as *char
    webview::webview_load_html(&raw mut wv, html)
    webview::webview_show(&raw mut wv)
    webview::webview_run(&raw mut wv)
    webview::webview_destroy(&raw mut wv)

    if(!g_method_ok) {
        env.error("bound handler was not invoked with method 'ping'")
        return
    }
    if(!g_args_ok) {
        env.error("bound handler did not receive expected args (42 / hello)")
        return
    }
}
