using std::string
using std::string_view

// ---------------------------------------------------------------------------
// Second webview test file.
//
// These tests live in a separate file (next to webview_test.ch) on purpose so
// they can be added/removed without touching the existing suite — handy when
// collaborating to avoid merge churn.
//
// Display-independent tests run everywhere. The live tests create a real window
// and exercise the JS<->native bridge end to end; they need a graphical display
// and quit themselves as soon as the bound handler fires (no manual interaction
// required). All public webview API surface is cross-platform, so the same
// source runs on Linux and Windows.
// ---------------------------------------------------------------------------

// Module-level state is kept non-destructible (bools / pointers) to avoid the
// top-level-destructor restriction on `string`.
unsafe var g_method_ok : bool
unsafe var g_args_ok : bool
unsafe var g_result_ok : bool
unsafe var g_eval_ok : bool
unsafe var g_alpha : bool
unsafe var g_beta : bool
unsafe var g_gamma : bool
unsafe var g_wv : *mut webview::WebView

// Result callback for webview_evaluate_js_result: records that JS evaluation
// produced "5" (2 + 3) and stops the run loop.
func eval_cb(data : *mut void, result : *char) : void {
    if(result == null) {
        g_eval_ok = false
        if(g_wv != null) { webview::webview_stop(g_wv) }
        return
    }
    var rv = string_view::make_no_len(result)
    g_eval_ok = rv.size() == 1 && rv.find(string_view::make_no_len("5")) == 0
    if(g_wv != null) { webview::webview_stop(g_wv) }
}

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
public func test_webview_noop_on_uninitialized(env : &mut TestEnv) {
    // Every operation that touches a native window must be a safe no-op (and not
    // crash) when the webview was never initialized.
    var wv = webview::WebView.make()
    webview::webview_load_html(&raw mut wv, "x\0" as *char)
    webview::webview_load_url(&raw mut wv, "about:blank\0" as *char)
    webview::webview_show(&raw mut wv)
    webview::webview_hide(&raw mut wv)
    webview::webview_evaluate_js(&raw mut wv, "1\0" as *char)
    webview::webview_set_bounds(&raw mut wv, 0, 0, 10, 10)
    webview::webview_evaluate_js_result(&raw mut wv, "1\0" as *char, eval_cb, null)
    webview::webview_destroy(&raw mut wv)
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

@test
public func test_webview_bind_result_delivery(env : &mut TestEnv) {
    // The handler returns a value; JS must receive it (the _resolve path) and
    // pass it back in a second call so we can verify the round trip of the
    // returned value.
    g_result_ok = false
    g_wv = null

    var wv_result = webview::create("WebView Result Test\0" as *char, 400, 300)
    if(wv_result is std::Result.Err) {
        env.error("webview::create failed (is a display available?)")
        return
    }
    var Ok(wv) = wv_result else unreachable
    g_wv = &raw mut wv

    var bind_result = webview::webview_bind(&raw mut wv, (method, args) => {
        if(method.find(string_view::make_no_len("get")) == 0) {
            return string("delivered")
        }
        if(method.find(string_view::make_no_len("verify")) == 0) {
            g_result_ok = args.find(string_view::make_no_len("delivered")) != args.size()
            if(g_wv != null) { webview::webview_stop(g_wv) }
        }
        return string("{\"ok\":true}")
    })
    if(bind_result is std::Result.Err) {
        env.error("webview_bind failed")
        webview::webview_destroy(&raw mut wv)
        return
    }

    var html = "<html><body><script>window.webview_bridge.call('get', '{}').then(function(v){ window.webview_bridge.call('verify', JSON.stringify({v:v})); });</script></body></html>\0" as *char
    webview::webview_load_html(&raw mut wv, html)
    webview::webview_show(&raw mut wv)
    webview::webview_run(&raw mut wv)
    webview::webview_destroy(&raw mut wv)

    if(!g_result_ok) {
        env.error("handler return value was not delivered back to JavaScript")
        return
    }
}

@test
public func test_webview_bind_method_routing(env : &mut TestEnv) {
    // Sequential promise chaining through the bridge must route each method to
    // the correct native branch.
    g_alpha = false
    g_beta = false
    g_gamma = false
    g_wv = null

    var wv_result = webview::create("WebView Routing Test\0" as *char, 400, 300)
    if(wv_result is std::Result.Err) {
        env.error("webview::create failed (is a display available?)")
        return
    }
    var Ok(wv) = wv_result else unreachable
    g_wv = &raw mut wv

    var bind_result = webview::webview_bind(&raw mut wv, (method, args) => {
        if(method.find(string_view::make_no_len("alpha")) == 0) {
            g_alpha = true
            return string("A")
        }
        if(method.find(string_view::make_no_len("beta")) == 0) {
            g_beta = true
            return string("B")
        }
        if(method.find(string_view::make_no_len("gamma")) == 0) {
            g_gamma = true
            if(g_wv != null) { webview::webview_stop(g_wv) }
        }
        return string("")
    })
    if(bind_result is std::Result.Err) {
        env.error("webview_bind failed")
        webview::webview_destroy(&raw mut wv)
        return
    }

    var html = "<html><body><script>window.webview_bridge.call('alpha', '{}').then(function(){ return window.webview_bridge.call('beta', '{}'); }).then(function(){ return window.webview_bridge.call('gamma', '{}'); });</script></body></html>\0" as *char
    webview::webview_load_html(&raw mut wv, html)
    webview::webview_show(&raw mut wv)
    webview::webview_run(&raw mut wv)
    webview::webview_destroy(&raw mut wv)

    if(!g_alpha) { env.error("method 'alpha' was not routed to the handler"); return }
    if(!g_beta) { env.error("method 'beta' was not routed to the handler"); return }
    if(!g_gamma) { env.error("method 'gamma' was not routed to the handler"); return }
}

@test
public func test_webview_evaluate_js_result(env : &mut TestEnv) {
    // Native -> JS evaluation: the handler triggers an async JS evaluation and
    // the result callback must receive the computed value ("5").
    g_eval_ok = false
    g_wv = null

    var wv_result = webview::create("WebView Eval Test\0" as *char, 400, 300)
    if(wv_result is std::Result.Err) {
        env.error("webview::create failed (is a display available?)")
        return
    }
    var Ok(wv) = wv_result else unreachable
    g_wv = &raw mut wv

    var bind_result = webview::webview_bind(&raw mut wv, (method, args) => {
        if(method.find(string_view::make_no_len("eval")) == 0) {
            webview::webview_evaluate_js_result(g_wv, "2+3\0" as *char, eval_cb, null)
        }
        return string("")
    })
    if(bind_result is std::Result.Err) {
        env.error("webview_bind failed")
        webview::webview_destroy(&raw mut wv)
        return
    }

    var html = "<html><body><script>window.webview_bridge.call('eval', '{}');</script></body></html>\0" as *char
    webview::webview_load_html(&raw mut wv, html)
    webview::webview_show(&raw mut wv)
    webview::webview_run(&raw mut wv)
    webview::webview_destroy(&raw mut wv)

    if(!g_eval_ok) {
        env.error("webview_evaluate_js_result did not deliver the expected value")
        return
    }
}

@test
public func test_webview_set_title_size_live(env : &mut TestEnv) {
    // On a live (created) webview, set_title / set_size must update the struct
    // fields (the display-independent path is covered by webview_test.ch; here
    // we exercise the created-window path).
    var wv_result = webview::create("TitleSize Test\0" as *char, 640, 480)
    if(wv_result is std::Result.Err) {
        env.error("webview::create failed (is a display available?)")
        return
    }
    var Ok(wv) = wv_result else unreachable

    webview::webview_set_title(&raw mut wv, "LiveTitle\0" as *char)
    webview::webview_set_size(&raw mut wv, 1234, 567)

    if(!(string_view::make_view(&wv.title).find(string_view::make_no_len("LiveTitle")) == 0)) {
        env.error("set_title did not update the title field on a created webview")
        webview::webview_destroy(&raw mut wv)
        return
    }
    if(wv.width != 1234) {
        env.error("set_size did not update width on a created webview")
        webview::webview_destroy(&raw mut wv)
        return
    }
    if(wv.height != 567) {
        env.error("set_size did not update height on a created webview")
        webview::webview_destroy(&raw mut wv)
        return
    }
    webview::webview_destroy(&raw mut wv)
}
