using std::string;
using std::string_view;

// ===========================================================================
// Tests for new webview API features
// ===========================================================================

// ---- webview_version ----

@test
public func test_webview_version(env : &mut TestEnv) {
    var ver = webview::raw::webview_version()
    // Version should be non-zero
    if(ver.major == 0 && ver.minor == 0 && ver.patch == 0) {
        env.error("webview_version returned all zeros")
        return
    }
}

// ---- webview_get_native_handle (uninitialized) ----

@test
public func test_webview_get_native_handle_uninitialized(env : &mut TestEnv) {
    // get_native_handle on an uninitialized webview should return null for all kinds
    var wv = webview::WebView.make()
    var hwnd = webview::raw::webview_get_native_handle(&raw mut wv, webview::NATIVE_HANDLE_WINDOW)
    var w_widget = webview::raw::webview_get_native_handle(&raw mut wv, webview::NATIVE_HANDLE_WIDGET)
    var w_ctrl = webview::raw::webview_get_native_handle(&raw mut wv, webview::NATIVE_HANDLE_BROWSER_CONTROLLER)
    if(hwnd != null) {
        env.error("get_native_handle(Window) should be null for uninitialized webview")
        return
    }
    if(w_widget != null) {
        env.error("get_native_handle(Widget) should be null for uninitialized webview")
        return
    }
    if(w_ctrl != null) {
        env.error("get_native_handle(BrowserController) should be null for uninitialized webview")
        return
    }
}

// ---- webview_get_native_handle (initialized) ----

@test
public func test_webview_get_native_handle_initialized(env : &mut TestEnv) {
    // get_native_handle on an initialized webview should return non-null Window handle
    var wv = webview::WebView.make()
    var cr = webview::raw::webview_create(&raw mut wv)
    if(cr is std::Result.Err) {
        env.error("webview_create failed (is a display available?)")
        return
    }
    var hwnd = webview::raw::webview_get_native_handle(&raw mut wv, webview::NATIVE_HANDLE_WINDOW)
    if(hwnd == null) {
        env.error("get_native_handle(Window) returned null for initialized webview")
        webview::raw::webview_destroy(&raw mut wv)
        return
    }
    var w_widget = webview::raw::webview_get_native_handle(&raw mut wv, webview::NATIVE_HANDLE_WIDGET)
    if(w_widget == null) {
        env.error("get_native_handle(Widget) returned null for initialized webview")
        webview::raw::webview_destroy(&raw mut wv)
        return
    }
    var w_ctrl = webview::raw::webview_get_native_handle(&raw mut wv, webview::NATIVE_HANDLE_BROWSER_CONTROLLER)
    if(w_ctrl == null) {
        env.error("get_native_handle(BrowserController) returned null for initialized webview")
        webview::raw::webview_destroy(&raw mut wv)
        return
    }
    webview::raw::webview_destroy(&raw mut wv)
}

// ---- webview_set_size_hints ----

@test
public func test_webview_set_size_hints(env : &mut TestEnv) {
    // set_size_hints should update the struct fields (display-independent check)
    var wv = webview::WebView.make()
    webview::raw::webview_set_size_hints(&raw mut wv, 1024, 768, webview::SIZE_HINT_NONE)
    if(wv.width != 1024) {
        env.error("set_size_hints did not update width")
        return
    }
    if(wv.height != 768) {
        env.error("set_size_hints did not update height")
        return
    }
}

@test
public func test_webview_set_size_hints_live(env : &mut TestEnv) {
    // set_size_hints on an initialized webview should not crash
    var wv = webview::WebView.make()
    var cr = webview::raw::webview_create(&raw mut wv)
    if(cr is std::Result.Err) {
        env.error("webview_create failed (is a display available?)")
        return
    }
    webview::raw::webview_set_size_hints(&raw mut wv, 1024, 768, webview::SIZE_HINT_NONE)
    if(wv.width != 1024) {
        env.error("set_size_hints did not update width on live webview")
        webview::raw::webview_destroy(&raw mut wv)
        return
    }
    if(wv.height != 768) {
        env.error("set_size_hints did not update height on live webview")
        webview::raw::webview_destroy(&raw mut wv)
        return
    }
    webview::raw::webview_destroy(&raw mut wv)
}

// ---- webview_init (user script injection) ----

@test
public func test_webview_init_script(env : &mut TestEnv) {
    // webview_init should not crash on an initialized webview
    var wv = webview::WebView.make()
    var cr = webview::raw::webview_create(&raw mut wv)
    if(cr is std::Result.Err) {
        env.error("webview_create failed (is a display available?)")
        return
    }
    webview::raw::webview_init(&raw mut wv, "var __test_init = 1;\0" as *char)
    webview::raw::webview_destroy(&raw mut wv)
}

// ---- webview_dispatch ----

@test
public func test_webview_dispatch(env : &mut TestEnv) {
    // webview_dispatch should not crash on an initialized webview
    // (dispatch requires a running message loop, so we just verify it doesn't crash)
    var wv = webview::WebView.make()
    var cr = webview::raw::webview_create(&raw mut wv)
    if(cr is std::Result.Err) {
        env.error("webview_create failed (is a display available?)")
        return
    }
    // Dispatch a no-op; since no message loop is running, this just queues
    webview::raw::webview_dispatch(&raw mut wv, (arg) => { /* noop */ }, null)
    webview::raw::webview_destroy(&raw mut wv)
}

// ---- webview_dispatch thread safety ----

@test
public func test_webview_dispatch_queue_limit(env : &mut TestEnv) {
    // webview_dispatch should handle multiple dispatches without crashing
    var wv = webview::WebView.make()
    var cr = webview::raw::webview_create(&raw mut wv)
    if(cr is std::Result.Err) {
        env.error("webview_create failed (is a display available?)")
        return
    }
    // Queue several dispatches (they won't execute until message loop runs)
    webview::raw::webview_dispatch(&raw mut wv, (arg) => { /* noop */ }, null)
    webview::raw::webview_dispatch(&raw mut wv, (arg) => { /* noop */ }, null)
    webview::raw::webview_dispatch(&raw mut wv, (arg) => { /* noop */ }, null)
    webview::raw::webview_destroy(&raw mut wv)
}

// ---- webview_unbind ----

@test
public func test_webview_unbind(env : &mut TestEnv) {
    // unbind should not crash when no binding exists
    var wv = webview::WebView.make()
    var cr = webview::raw::webview_create(&raw mut wv)
    if(cr is std::Result.Err) {
        env.error("webview_create failed (is a display available?)")
        return
    }
    webview::raw::webview_unbind(&raw mut wv, "nonexistent\0" as *char)
    webview::raw::webview_destroy(&raw mut wv)
}

// ---- webview_return ----

@test
public func test_webview_return(env : &mut TestEnv) {
    // return should not crash when webview is initialized
    var wv = webview::WebView.make()
    var cr = webview::raw::webview_create(&raw mut wv)
    if(cr is std::Result.Err) {
        env.error("webview_create failed (is a display available?)")
        return
    }
    webview::raw::webview_return(&raw mut wv, "test_id\0" as *char, 0, "{\"ok\":true}\0" as *char)
    webview::raw::webview_destroy(&raw mut wv)
}
