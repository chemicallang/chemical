using std::string
using std::string_view

// ---------------------------------------------------------------------------
// Creation pattern tests.
//
// These tests verify that both WebView creation patterns work correctly:
//
//   Pattern A ("stack-local"):
//       var wv = webview::WebView.make()
//       webview::webview_set_title(&raw mut wv, ...)
//       webview::webview_set_size(&raw mut wv, ...)
//       webview::webview_create(&raw mut wv)
//       // wv lives on this stack frame — user data pointers stay valid
//
//   Pattern B ("by-value via webview::create"):
//       var wv_result = webview::create(title, w, h)  // returns Result<WebView, ...>
//       var Ok(wv) = wv_result
//       // wv is a copy — HWND user data pointed to create()'s stack frame,
//       // but webview_rebind() corrects it automatically in show/run/set_size
//
// Before the webview_rebind() fix, Pattern B crashed with "invalid memory
// access" because the native HWNDs held a dangling pointer to create()'s
// stack frame. These tests ensure both patterns keep working.
// ---------------------------------------------------------------------------

// ---- shared state ----

var g_create_pat_ok : bool
var g_create_pat_wv : *mut webview::WebView

func create_pat_cb(data : *mut void, result : *char) : void {
    if(result == null) {
        g_create_pat_ok = false
        if(g_create_pat_wv != null) { webview::webview_stop(g_create_pat_wv) }
        return
    }
    var rv = string_view::make_no_len(result)
    g_create_pat_ok = rv.size() == 1 && rv.find(string_view::make_no_len("5")) == 0
    if(g_create_pat_wv != null) { webview::webview_stop(g_create_pat_wv) }
}

// ---------------------------------------------------------------------------
// Pattern A — stack-local WebView.make() + webview_create()
// ---------------------------------------------------------------------------

@test
public func test_create_pattern_a_bridge(env : &mut TestEnv) {
    // Pattern A: WebView.make() + webview_create(&raw mut wv).
    // Exercises bridge round-trip to prove user data pointers are valid.
    g_create_pat_ok = false
    g_create_pat_wv = null

    var wv = webview::WebView.make()
    webview::webview_set_title(&raw mut wv, "PatA Bridge\0" as *char)
    webview::webview_set_size(&raw mut wv, 400, 300)
    var cr = webview::webview_create(&raw mut wv)
    if(cr is std::Result.Err) {
        env.error("webview_create failed (Pattern A)")
        return
    }
    g_create_pat_wv = &raw mut wv

    webview::webview_bind(&raw mut wv, (method, args) => {
        if(method.find(string_view::make_no_len("ping")) == 0) {
            g_create_pat_ok = true
            if(g_create_pat_wv != null) { webview::webview_stop(g_create_pat_wv) }
        }
        return string("{\"ok\":true}")
    })

    var html = "<html><body><script>window.__webview__.call('ping', JSON.stringify({pattern:'A'}));</script></body></html>\0" as *char
    webview::webview_load_html(&raw mut wv, html)
    webview::webview_show(&raw mut wv)
    webview::webview_run(&raw mut wv)
    webview::webview_destroy(&raw mut wv)

    if(!g_create_pat_ok) {
        env.error("Pattern A bridge round-trip failed")
        return
    }
}

@test
public func test_create_pattern_a_evaluate_js(env : &mut TestEnv) {
    // Pattern A: verify evaluate_js_result works with stack-local webview.
    g_create_pat_ok = false
    g_create_pat_wv = null

    var wv = webview::WebView.make()
    webview::webview_set_title(&raw mut wv, "PatA Eval\0" as *char)
    webview::webview_set_size(&raw mut wv, 400, 300)
    var cr = webview::webview_create(&raw mut wv)
    if(cr is std::Result.Err) {
        env.error("webview_create failed (Pattern A eval)")
        return
    }
    g_create_pat_wv = &raw mut wv

    webview::webview_bind(&raw mut wv, (method, args) => {
        if(method.find(string_view::make_no_len("go")) == 0) {
            webview::webview_evaluate_js_result(g_create_pat_wv, "2+3\0" as *char, create_pat_cb, null)
        }
        return string("")
    })

    var html = "<html><body><script>window.__webview__.call('go', '{}');</script></body></html>\0" as *char
    webview::webview_load_html(&raw mut wv, html)
    webview::webview_show(&raw mut wv)
    webview::webview_run(&raw mut wv)
    webview::webview_destroy(&raw mut wv)

    if(!g_create_pat_ok) {
        env.error("Pattern A evaluate_js_result failed")
        return
    }
}

// ---------------------------------------------------------------------------
// Pattern B — webview::create() by-value return (tests webview_rebind)
// ---------------------------------------------------------------------------

@test
public func test_create_pattern_b_bridge(env : &mut TestEnv) {
    // Pattern B: webview::create() returns Result<WebView, ...>.
    // The WebView is extracted from the Result (by-value copy), but
    // webview_rebind() inside webview_show/webview_run fixes the stale
    // HWND user data pointers automatically.
    g_create_pat_ok = false
    g_create_pat_wv = null

    var wv_result = webview::create("PatB Bridge\0" as *char, 400, 300)
    if(wv_result is std::Result.Err) {
        env.error("webview::create failed (Pattern B)")
        return
    }
    var Ok(wv) = wv_result else unreachable
    g_create_pat_wv = &raw mut wv

    var bind_result = webview::webview_bind(&raw mut wv, (method, args) => {
        if(method.find(string_view::make_no_len("ping")) == 0) {
            g_create_pat_ok = true
            if(g_create_pat_wv != null) { webview::webview_stop(g_create_pat_wv) }
        }
        return string("{\"ok\":true}")
    })
    if(bind_result is std::Result.Err) {
        env.error("webview_bind failed (Pattern B)")
        webview::webview_destroy(&raw mut wv)
        return
    }

    var html = "<html><body><script>window.__webview__.call('ping', JSON.stringify({pattern:'B'}));</script></body></html>\0" as *char
    webview::webview_load_html(&raw mut wv, html)
    webview::webview_show(&raw mut wv)
    webview::webview_run(&raw mut wv)
    webview::webview_destroy(&raw mut wv)

    if(!g_create_pat_ok) {
        env.error("Pattern B bridge round-trip failed")
        return
    }
}

@test
public func test_create_pattern_b_evaluate_js(env : &mut TestEnv) {
    // Pattern B: verify evaluate_js_result works with by-value webview.
    g_create_pat_ok = false
    g_create_pat_wv = null

    var wv_result = webview::create("PatB Eval\0" as *char, 400, 300)
    if(wv_result is std::Result.Err) {
        env.error("webview::create failed (Pattern B eval)")
        return
    }
    var Ok(wv) = wv_result else unreachable
    g_create_pat_wv = &raw mut wv

    var bind_result = webview::webview_bind(&raw mut wv, (method, args) => {
        if(method.find(string_view::make_no_len("go")) == 0) {
            webview::webview_evaluate_js_result(g_create_pat_wv, "2+3\0" as *char, create_pat_cb, null)
        }
        return string("")
    })
    if(bind_result is std::Result.Err) {
        env.error("webview_bind failed (Pattern B eval)")
        webview::webview_destroy(&raw mut wv)
        return
    }

    var html = "<html><body><script>window.__webview__.call('go', '{}');</script></body></html>\0" as *char
    webview::webview_load_html(&raw mut wv, html)
    webview::webview_show(&raw mut wv)
    webview::webview_run(&raw mut wv)
    webview::webview_destroy(&raw mut wv)

    if(!g_create_pat_ok) {
        env.error("Pattern B evaluate_js_result failed")
        return
    }
}

@test
public func test_create_pattern_b_set_size_live(env : &mut TestEnv) {
    // Pattern B: verify set_size triggers webview_rebind before MoveWindow,
    // so the resize callback reads the correct WebView pointer.
    var wv_result = webview::create("PatB Size\0" as *char, 640, 480)
    if(wv_result is std::Result.Err) {
        env.error("webview::create failed (Pattern B size)")
        return
    }
    var Ok(wv) = wv_result else unreachable

    // set_size on a by-value webview — triggers MoveWindow → WM_SIZE →
    // resize callback reads user data. webview_rebind corrects it.
    webview::webview_set_size(&raw mut wv, 1024, 768)
    if(wv.width != 1024) {
        env.error("Pattern B set_size did not update width")
        webview::webview_destroy(&raw mut wv)
        return
    }
    if(wv.height != 768) {
        env.error("Pattern B set_size did not update height")
        webview::webview_destroy(&raw mut wv)
        return
    }
    webview::webview_destroy(&raw mut wv)
}

@test
public func test_create_pattern_b_title_size_fields(env : &mut TestEnv) {
    // Pattern B: verify set_title and set_size update struct fields on
    // a by-value webview.
    var wv_result = webview::create("PatB Fields\0" as *char, 400, 300)
    if(wv_result is std::Result.Err) {
        env.error("webview::create failed (Pattern B fields)")
        return
    }
    var Ok(wv) = wv_result else unreachable

    webview::webview_set_title(&raw mut wv, "NewTitle\0" as *char)
    webview::webview_set_size(&raw mut wv, 800, 600)

    if(!(string_view::make_view(&wv.title).find(string_view::make_no_len("NewTitle")) == 0)) {
        env.error("Pattern B set_title did not update field")
        webview::webview_destroy(&raw mut wv)
        return
    }
    if(wv.width != 800 || wv.height != 600) {
        env.error("Pattern B set_size did not update fields")
        webview::webview_destroy(&raw mut wv)
        return
    }
    webview::webview_destroy(&raw mut wv)
}
