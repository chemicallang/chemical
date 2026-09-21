using std::string
using std::string_view

// ---------------------------------------------------------------------------
// Regression tests for the Windows/WebView2 fixed-buffer and message-queue bugs.
//
// The Windows backend used to widen every string into a fixed-size stack buffer
// (widen_to_buf + a literal `[N]ushort` array). When the input did not fit,
// MultiByteToWideChar failed and widen_to_buf wrote an empty string, so the HTML
// page, an evaluated script or an init script silently became empty. The
// universal-test page alone is ~475 KB, which loaded blank and then blocked the
// message loop forever (the page's harness never ran, so `all_done` was never
// sent). These tests load/evaluate scripts comfortably larger than the old caps
// and prove the JS actually executed by having it call back through the bridge.
//
// The sequential-recreate test covers the other half: `WM_DESTROY` used to post
// `WM_QUIT` unconditionally, so the stray quit left behind by a programmatic
// `webview_destroy` made the *next* WebView's initialization loop quit
// immediately (-6), which broke every `isolate`d test.
//
// All of this goes through the public, cross-platform API, so the same source
// runs on Linux. On success each test stops its own message loop; a JS safety
// timer stops it (and the test fails on the unset flag) if something goes wrong.
// ---------------------------------------------------------------------------

// Shared state (non-destructible, like the rest of the suite).
var g_reg_ok : bool
var g_reg_wv : *mut webview::WebView

func reg_stop() {
    if(g_reg_wv != null) { webview::webview_stop(g_reg_wv) }
}

// A string of `n` 'A' characters, appended in chunks (cheap, no per-char
// reallocation of the destination).
func reg_pad(n : int) : string {
    var chunk = string("")
    var c = 0
    while(c < 1024) {
        chunk.append('A')
        c += 1
    }
    var out = string("")
    var k = 0
    while(k < n / 1024) {
        out.append_view(string_view::make_view(&chunk))
        k += 1
    }
    // append the remainder so the result is exactly `n` characters long
    var rem = n - (n / 1024) * 1024
    c = 0
    while(c < rem) {
        out.append('A')
        c += 1
    }
    return out
}

// Creates a webview, loads a page that calls `ping` on load, runs the loop and
// destroys it. Returns true when the bridge call was received. Used twice in a
// row by the sequential-recreate test: before the fix the second create failed
// because of the stale WM_QUIT left by the first destroy.
func reg_run_bridge(title : *char) : bool {
    g_reg_ok = false
    var wv_result = webview::create(title, 400, 300)
    if(wv_result is std::Result.Err) {
        g_reg_wv = null
        return false
    }
    var Ok(wv) = wv_result else unreachable
    g_reg_wv = &raw mut wv

    webview::webview_bind(&raw mut wv, (method, args) => {
        if(method.find(string_view::make_no_len("ping")) == 0) {
            g_reg_ok = true
            reg_stop()
        } else if(method.find(string_view::make_no_len("stop")) == 0) {
            reg_stop()
        }
        return string("{\"ok\":true}")
    })

    var html = "<html><body><script>window.__webview__.call('ping','{}');setTimeout(function(){window.__webview__.call('stop','timeout');},6000);</script></body></html>\0" as *char
    webview::webview_load_html(&raw mut wv, html)
    webview::webview_show(&raw mut wv)
    webview::webview_run(&raw mut wv)
    webview::webview_destroy(&raw mut wv)
    g_reg_wv = null
    return g_reg_ok
}

// ---- webview_load_html: page larger than the old 65536-UTF-16 cap ----------

@test
public func test_webview_load_html_large_page(env : &mut TestEnv) {
    g_reg_ok = false
    g_reg_wv = null

    var html = string("<html><head><script>")
    html.append_view(string_view::make_no_len("function go(){try{var n=document.getElementById('pad').textContent.length;window.__webview__.call('padlen',String(n));}catch(e){window.__webview__.call('padlen','ERR:'+e.message);}}"))
    html.append_view(string_view::make_no_len("setTimeout(function(){window.__webview__.call('stop','timeout');},6000)"))
    html.append_view(string_view::make_no_len("</script></head><body onload='go()'><div id='pad'>"))
    // ~89 KB of padding: beyond the old 65536 UTF-16 element cap.
    var pad = reg_pad(90000)
    html.append_view(string_view::make_view(&pad))
    html.append_view(string_view::make_no_len("</div></body></html>"))

    var wv_result = webview::create("Large HTML\0" as *char, 480, 360)
    if(wv_result is std::Result.Err) {
        env.error("webview::create failed (is a display available?)")
        return
    }
    var Ok(wv) = wv_result else unreachable
    g_reg_wv = &raw mut wv

    webview::webview_bind(&raw mut wv, (method, args) => {
        if(method.find(string_view::make_no_len("padlen")) == 0) {
            // The page must have executed and seen all ~90 KB of padding.
            g_reg_ok = args.contains(string_view::make_no_len("90000"))
            if(args.contains(string_view::make_no_len("ERR"))) { g_reg_ok = false }
            reg_stop()
        } else if(method.find(string_view::make_no_len("stop")) == 0) {
            reg_stop()
        }
        return string("{\"ok\":true}")
    })

    webview::webview_load_html(&raw mut wv, html.data() as *char)
    webview::webview_show(&raw mut wv)
    webview::webview_run(&raw mut wv)
    webview::webview_destroy(&raw mut wv)
    g_reg_wv = null

    if(!g_reg_ok) {
        env.error("large HTML page was not loaded/executed (fixed-buffer truncation regression)")
    }
}

// ---- webview_evaluate_js: script larger than the old 32768 cap -------------

@test
public func test_webview_evaluate_js_large_script(env : &mut TestEnv) {
    g_reg_ok = false
    g_reg_wv = null

    var wv_result = webview::create("Large Eval\0" as *char, 480, 360)
    if(wv_result is std::Result.Err) {
        env.error("webview::create failed (is a display available?)")
        return
    }
    var Ok(wv) = wv_result else unreachable
    g_reg_wv = &raw mut wv

    webview::webview_bind(&raw mut wv, (method, args) => {
        if(method.find(string_view::make_no_len("ready")) == 0) {
            // > 32768 chars: a long comment (so truncation loses the statement)
            // followed by the actual call that must still reach the bridge.
            var big = string("/*")
            var pad = reg_pad(40000)
            big.append_view(string_view::make_view(&pad))
            big.append_view(string_view::make_no_len("*/window.__webview__.call('evok','1');"))
            webview::webview_evaluate_js(g_reg_wv, big.data() as *char)
        } else if(method.find(string_view::make_no_len("evok")) == 0) {
            g_reg_ok = true
            reg_stop()
        } else if(method.find(string_view::make_no_len("stop")) == 0) {
            reg_stop()
        }
        return string("{}")
    })

    var html = "<html><body><script>window.__webview__.call('ready','{}');setTimeout(function(){window.__webview__.call('stop','timeout');},6000);</script></body></html>\0" as *char
    webview::webview_load_html(&raw mut wv, html)
    webview::webview_show(&raw mut wv)
    webview::webview_run(&raw mut wv)
    webview::webview_destroy(&raw mut wv)
    g_reg_wv = null

    if(!g_reg_ok) {
        env.error("large evaluate_js script was not executed (fixed-buffer truncation regression)")
    }
}

// ---- webview_init: init script larger than the old 32768 cap ---------------

@test
public func test_webview_init_large_script(env : &mut TestEnv) {
    g_reg_ok = false
    g_reg_wv = null

    var wv_result = webview::create("Large Init\0" as *char, 480, 360)
    if(wv_result is std::Result.Err) {
        env.error("webview::create failed (is a display available?)")
        return
    }
    var Ok(wv) = wv_result else unreachable
    g_reg_wv = &raw mut wv

    webview::webview_bind(&raw mut wv, (method, args) => {
        if(method.find(string_view::make_no_len("initok")) == 0) {
            g_reg_ok = true
            reg_stop()
        } else if(method.find(string_view::make_no_len("stop")) == 0) {
            reg_stop()
        }
        return string("{}")
    })

    // Registered before the page loads; runs on document creation. The trailing
    // bridge call is only reached if the whole script survived.
    var big = string("/*")
    var pad = reg_pad(40000)
    big.append_view(string_view::make_view(&pad))
    big.append_view(string_view::make_no_len("*/window.__webview__.call('initok','1');"))
    webview::webview_init(&raw mut wv, big.data() as *char)

    var html = "<html><body><script>setTimeout(function(){window.__webview__.call('stop','timeout');},6000);</script></body></html>\0" as *char
    webview::webview_load_html(&raw mut wv, html)
    webview::webview_show(&raw mut wv)
    webview::webview_run(&raw mut wv)
    webview::webview_destroy(&raw mut wv)
    g_reg_wv = null

    if(!g_reg_ok) {
        env.error("large init script was not executed (fixed-buffer truncation regression)")
    }
}

// ---- sequential create/run/destroy: stale WM_QUIT regression ---------------

@test
public func test_webview_sequential_recreate(env : &mut TestEnv) {
    // A single test process creates, runs and destroys two WebViews in a row.
    // Before the fix, the WM_DESTROY of the first destroy posted a stray
    // WM_QUIT that the second WebView's init loop consumed immediately, so the
    // second create failed with "message loop quit during initialization".
    var first = reg_run_bridge("Seq First\0" as *char)
    var second = reg_run_bridge("Seq Second\0" as *char)

    if(!first) {
        env.error("first webview in a sequence did not round-trip")
        return
    }
    if(!second) {
        env.error("recreating a webview after destroy failed (stale WM_QUIT regression)")
        return
    }
}
