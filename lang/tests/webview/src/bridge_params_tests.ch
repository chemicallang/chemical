using std::string;
using std::string_view;
using JsonParser;
using ASTJsonHandler;
using JsonValue;

// ===========================================================================
// Tests for bridge params array handling and promise reliability.
//
// The webview bridge JS sends: {id, method, params: [body]} where body is
// JSON.stringify(someObject). The native side receives the raw params value
// as a JSON array string like ["{\"tool\":\"yt-dlp\"}"]. These tests verify
// that the native handler can correctly extract fields from this format.
// ===========================================================================

// ---- shared state ----

var g_params_ok : bool
@never_destructed var g_params_field : string
var g_params_count : int
var g_promise_chain_ok : bool
var g_promise_chain_count : int
var g_eval_reliable_ok : bool
var g_wv_params : *mut webview::WebView

// ---- helpers ----

func reset_params_state() {
    g_params_ok = false
    g_params_field = string("")
    g_params_count = 0
    g_promise_chain_ok = false
    g_promise_chain_count = 0
    g_eval_reliable_ok = false
    g_wv_params = null
}

// Simulate the resolve_bridge_args + json_field logic from Bridge.ch.
// The webview bridge wraps every call body in a params array:
//   {id, method, params: [body]}
// So `args` arrives as a JSON array like ["{"tool":"yt-dlp"}"].
// This function extracts element 0 (the stringified body) and returns it.
func resolve_and_extract_field(args : string_view, key : string_view) : string {
    var parser = JsonParser(128, 4096)
    var ph = ASTJsonHandler.make()
    parser.parse(args.data(), args.size(), &mut ph)
    // If already a JSON object, search directly.
    if(ph.root is JsonValue.Object) {
        var Object(map) = ph.root else unreachable
        var k = string(key.data(), key.size())
        var vp = map.get_ptr(&k)
        if(vp != null && vp is JsonValue.String) {
            var String(v) = *vp else unreachable
            return v.copy()
        }
        return string()
    }
    // If an array, extract element 0 (the stringified body).
    if(ph.root is JsonValue.Array) {
        var Array(arr) = ph.root else unreachable
        if(arr.size() > 0) {
            var elem = arr.get_ptr(0)
            if(elem is JsonValue.String) {
                var String(s) = *elem else unreachable
                // Re-parse the inner string as JSON.
                var inner = JsonParser(128, 4096)
                var inner_ph = ASTJsonHandler.make()
                inner.parse(s.data(), s.size(), &mut inner_ph)
                if(inner_ph.root is JsonValue.Object) {
                    var Object(inner_map) = inner_ph.root else unreachable
                    var ik = string(key.data(), key.size())
                    var ivp = inner_map.get_ptr(&ik)
                    if(ivp != null && ivp is JsonValue.String) {
                        var String(iv) = *ivp else unreachable
                        return iv.copy()
                    }
                }
            }
        }
    }
    return string()
}

// ===========================================================================
// Test 1: Params array unwrapping — stringified JSON object in array
// ===========================================================================

func verify_params_unwrap(env : &mut TestEnv) {
    if(!g_params_ok) {
        env.error("params unwrap bridge round-trip did not complete")
        return
    }
    if(g_params_count < 2) {
        env.error("expected at least 2 calls (extract + done)")
        return
    }
}

@test
public func test_bridge_params_array_unwrap(env : &mut TestEnv) {
    // This test replicates the exact scenario that caused the CDM tool
    // download to hang: JS calls bridge.call('method', JSON.stringify({tool:'yt-dlp'}))
    // which sends params:["{\"tool\":\"yt-dlp\"}"]. The native handler must
    // extract the tool field from the array-wrapped params.
    reset_params_state()

    var wv = webview::WebView.make()
    webview::webview_set_size(&raw mut wv, 480, 360)
    var cr = webview::webview_create(&raw mut wv)
    if(cr is std::Result.Err) {
        env.error("webview_create failed (is a display available?)")
        return
    }
    g_wv_params = &raw mut wv

    webview::webview_bind(&raw mut wv, (method, args) => {
        g_params_count = g_params_count + 1

        // Check for "extract" method — this is the key test
        if(method.size() == 7) {
            var d = method.data()
            if(d[0] == 'e' && d[1] == 'x' && d[2] == 't' && d[3] == 'r' && d[4] == 'a' && d[5] == 'c' && d[6] == 't') {
                // Try to extract "tool" from the params array
                var val = resolve_and_extract_field(args, string_view::make_no_len("tool"))
                g_params_field = val.copy()
                g_params_ok = (val.size() > 0)
                // Also verify the value is correct
                if(g_params_ok) {
                    g_params_ok = val.find(string_view::make_no_len("yt-dlp")) != val.size()
                }
                if(g_wv_params != null) { webview::webview_stop(g_wv_params) }
                return string("{\"ok\":true}")
            }
        }

        // Echo for other calls
        var resp = string("{\"echo\":")
        resp.append_view(&args)
        resp.append_view(string_view::make_no_len("}"))
        return resp
    })

    // The JS sends the body as JSON.stringify (stringified), which the bridge
    // wraps in params: ["{...}"]. This is the exact pattern that was broken.
    var html = string("<html><head><script>")
    html.append_view(string_view::make_no_len("function go(){try{"))
    html.append_view(string_view::make_no_len("window.__webview__.call('extract', JSON.stringify({tool:'yt-dlp'})).then(function(r){"))
    html.append_view(string_view::make_no_len("window.__webview__.call('done', JSON.stringify({ok:true,extracted:r}));"))
    html.append_view(string_view::make_no_len("}).catch(function(e){window.__webview__.call('done', JSON.stringify({error:e.message}));});"))
    html.append_view(string_view::make_no_len("}catch(e){window.__webview__.call('done', JSON.stringify({error:e.message}));}}"))
    html.append_view(string_view::make_no_len("</script></head><body onload='go()'><p>test</p></body></html>"))

    webview::webview_load_html(&raw mut wv, html.data() as *char)
    webview::webview_show(&raw mut wv)
    webview::webview_run(&raw mut wv)
    webview::webview_destroy(&raw mut wv)

    if(!g_params_ok) {
        env.error("failed to extract 'tool' field from params array — resolve_bridge_args logic is broken")
        return
    }
}

// ===========================================================================
// Test 2: Multiple fields from params array
// ===========================================================================

func verify_multi_field_params(env : &mut TestEnv) {
    if(!g_params_ok) {
        env.error("multi-field params test did not complete")
        return
    }
}

@test
public func test_bridge_params_multi_field(env : &mut TestEnv) {
    // Test extracting multiple fields from a params array containing a
    // stringified JSON object with multiple fields.
    reset_params_state()

    var wv = webview::WebView.make()
    webview::webview_set_size(&raw mut wv, 480, 360)
    var cr = webview::webview_create(&raw mut wv)
    if(cr is std::Result.Err) {
        env.error("webview_create failed")
        return
    }
    g_wv_params = &raw mut wv

    webview::webview_bind(&raw mut wv, (method, args) => {
        g_params_count = g_params_count + 1

        if(method.size() == 7) {
            var d = method.data()
            if(d[0] == 'e' && d[1] == 'x' && d[2] == 't' && d[3] == 'r' && d[4] == 'a' && d[5] == 'c' && d[6] == 't') {
                var tool_val = resolve_and_extract_field(args, string_view::make_no_len("tool"))
                var format_val = resolve_and_extract_field(args, string_view::make_no_len("format"))
                var priority_val = resolve_and_extract_field(args, string_view::make_no_len("priority"))
                g_params_ok = tool_val.find(string_view::make_no_len("yt-dlp")) != tool_val.size() &&
                              format_val.find(string_view::make_no_len("best")) != format_val.size() &&
                              priority_val.find(string_view::make_no_len("100")) != priority_val.size()
                if(g_wv_params != null) { webview::webview_stop(g_wv_params) }
                return string("{\"ok\":true}")
            }
        }
        return string("{\"ok\":true}")
    })

    var html = string("<html><head><script>")
    html.append_view(string_view::make_no_len("function go(){try{"))
    html.append_view(string_view::make_no_len("window.__webview__.call('extract', JSON.stringify({tool:'yt-dlp',format:'best',priority:100})).then(function(r){"))
    html.append_view(string_view::make_no_len("window.__webview__.call('done', r);"))
    html.append_view(string_view::make_no_len("}).catch(function(e){window.__webview__.call('done', JSON.stringify({error:e.message}));});"))
    html.append_view(string_view::make_no_len("}catch(e){window.__webview__.call('done', JSON.stringify({error:e.message}));}}"))
    html.append_view(string_view::make_no_len("</script></head><body onload='go()'><p>test</p></body></html>"))

    webview::webview_load_html(&raw mut wv, html.data() as *char)
    webview::webview_show(&raw mut wv)
    webview::webview_run(&raw mut wv)
    webview::webview_destroy(&raw mut wv)

    if(!g_params_ok) {
        env.error("failed to extract multiple fields from params array")
        return
    }
}

// ===========================================================================
// Test 3: Promise chain reliability — many sequential calls all resolve
// ===========================================================================

func verify_promise_chain_long(env : &mut TestEnv) {
    if(!g_promise_chain_ok) {
        env.error("long promise chain did not complete")
        return
    }
}

@test
public func test_bridge_promise_chain_long(env : &mut TestEnv) {
    // Verify that a long chain of sequential bridge calls all resolve correctly.
    // This tests that webview_evaluate_js pumps the main loop enough for each
    // onReply to execute, even with many calls in succession.
    reset_params_state()

    var wv = webview::WebView.make()
    webview::webview_set_size(&raw mut wv, 480, 360)
    var cr = webview::webview_create(&raw mut wv)
    if(cr is std::Result.Err) {
        env.error("webview_create failed")
        return
    }
    g_wv_params = &raw mut wv

    webview::webview_bind(&raw mut wv, (method, args) => {
        g_params_count = g_params_count + 1

        if(method.size() == 4) {
            var d = method.data()
            if(d[0] == 'd' && d[1] == 'o' && d[2] == 'n' && d[3] == 'e') {
                g_promise_chain_ok = true
                if(g_wv_params != null) { webview::webview_stop(g_wv_params) }
                return string("{\"ok\":true}")
            }
        }

        // Echo back the args as-is
        var resp = string("{\"echo\":")
        resp.append_view(&args)
        resp.append_view(string_view::make_no_len("}"))
        return resp
    })

    // Build a chain of 10 sequential calls
    var html = string("<html><head><script>")
    html.append_view(string_view::make_no_len("function go(){try{"))
    html.append_view(string_view::make_no_len("window.__webview__.call('step',{i:1}).then(function(){"))
    html.append_view(string_view::make_no_len("return window.__webview__.call('step',{i:2});"))
    html.append_view(string_view::make_no_len("}).then(function(){"))
    html.append_view(string_view::make_no_len("return window.__webview__.call('step',{i:3});"))
    html.append_view(string_view::make_no_len("}).then(function(){"))
    html.append_view(string_view::make_no_len("return window.__webview__.call('step',{i:4});"))
    html.append_view(string_view::make_no_len("}).then(function(){"))
    html.append_view(string_view::make_no_len("return window.__webview__.call('step',{i:5});"))
    html.append_view(string_view::make_no_len("}).then(function(){"))
    html.append_view(string_view::make_no_len("return window.__webview__.call('step',{i:6});"))
    html.append_view(string_view::make_no_len("}).then(function(){"))
    html.append_view(string_view::make_no_len("return window.__webview__.call('step',{i:7});"))
    html.append_view(string_view::make_no_len("}).then(function(){"))
    html.append_view(string_view::make_no_len("return window.__webview__.call('step',{i:8});"))
    html.append_view(string_view::make_no_len("}).then(function(){"))
    html.append_view(string_view::make_no_len("return window.__webview__.call('step',{i:9});"))
    html.append_view(string_view::make_no_len("}).then(function(){"))
    html.append_view(string_view::make_no_len("return window.__webview__.call('step',{i:10});"))
    html.append_view(string_view::make_no_len("}).then(function(){"))
    html.append_view(string_view::make_no_len("window.__webview__.call('done',{count:10});"))
    html.append_view(string_view::make_no_len("}).catch(function(e){window.__webview__.call('done',{error:e.message});});"))
    html.append_view(string_view::make_no_len("}catch(e){window.__webview__.call('done',{error:e.message});}}"))
    html.append_view(string_view::make_no_len("</script></head><body onload='go()'><p>test</p></body></html>"))

    webview::webview_load_html(&raw mut wv, html.data() as *char)
    webview::webview_show(&raw mut wv)
    webview::webview_run(&raw mut wv)
    webview::webview_destroy(&raw mut wv)

    if(!g_promise_chain_ok) {
        env.error("long promise chain (10 sequential calls) did not complete — webview_evaluate_js may not be pumping enough")
        return
    }
}

// ===========================================================================
// Test 4: evaluate_js from handler context delivers result to JS
// ===========================================================================

func eval_reliable_cb(data : *mut void, result : *char) : void {
    if(result == null) {
        g_eval_reliable_ok = false
        if(g_wv_params != null) { webview::webview_stop(g_wv_params) }
        return
    }
    var rv = string_view::make_no_len(result)
    g_eval_reliable_ok = rv.find(string_view::make_no_len("42")) != rv.size()
    if(g_wv_params != null) { webview::webview_stop(g_wv_params) }
}

func verify_eval_reliable(env : &mut TestEnv) {
    if(!g_eval_reliable_ok) {
        env.error("evaluate_js from handler context did not deliver result")
        return
    }
}

@test
public func test_bridge_evaluate_js_reliable(env : &mut TestEnv) {
    // Test that webview_evaluate_js (with the gtk_events_pending loop) reliably
    // delivers results back to JS when called from within a handler context.
    // This was the second root cause of the stuck download: the onReply JS
    // never executed because a single gtk_main_iteration() wasn't enough.
    reset_params_state()

    var wv = webview::WebView.make()
    webview::webview_set_size(&raw mut wv, 480, 360)
    var cr = webview::webview_create(&raw mut wv)
    if(cr is std::Result.Err) {
        env.error("webview_create failed")
        return
    }
    g_wv_params = &raw mut wv

    webview::webview_bind(&raw mut wv, (method, args) => {
        if(method.size() == 4) {
            var d = method.data()
            if(d[0] == 'g' && d[1] == 'o' && d[2] == 'o' && d[3] == 'd') {
                // Trigger evaluate_js from within the handler context
                webview::webview_evaluate_js_result(g_wv_params, "6*7\0" as *char, eval_reliable_cb, null)
                return string("{\"ok\":true}")
            }
        }
        return string("{\"ok\":true}")
    })

    var html = string("<html><head><script>")
    html.append_view(string_view::make_no_len("function go(){try{"))
    html.append_view(string_view::make_no_len("window.__webview__.call('good', '{}').then(function(r){"))
    html.append_view(string_view::make_no_len("/* handler triggered evaluate_js_result, callback will stop */"))
    html.append_view(string_view::make_no_len("}).catch(function(e){window.__webview__.call('done', e.message);});"))
    html.append_view(string_view::make_no_len("}catch(e){window.__webview__.call('done', e.message);}}"))
    html.append_view(string_view::make_no_len("</script></head><body onload='go()'><p>test</p></body></html>"))

    webview::webview_load_html(&raw mut wv, html.data() as *char)
    webview::webview_show(&raw mut wv)
    webview::webview_run(&raw mut wv)
    webview::webview_destroy(&raw mut wv)

    if(!g_eval_reliable_ok) {
        env.error("evaluate_js from handler context did not deliver result (42)")
        return
    }
}

// ===========================================================================
// Test 5: Bridge call with stringified nested JSON in params array
// ===========================================================================

func verify_nested_params(env : &mut TestEnv) {
    if(!g_params_ok) {
        env.error("nested params test did not complete")
        return
    }
}

@test
public func test_bridge_params_nested_json(env : &mut TestEnv) {
    // Test extracting a nested object from a params array containing a
    // stringified JSON object with nested structures.
    reset_params_state()

    var wv = webview::WebView.make()
    webview::webview_set_size(&raw mut wv, 480, 360)
    var cr = webview::webview_create(&raw mut wv)
    if(cr is std::Result.Err) {
        env.error("webview_create failed")
        return
    }
    g_wv_params = &raw mut wv

    webview::webview_bind(&raw mut wv, (method, args) => {
        g_params_count = g_params_count + 1

        if(method.size() == 7) {
            var d = method.data()
            if(d[0] == 'e' && d[1] == 'x' && d[2] == 't' && d[3] == 'r' && d[4] == 'a' && d[5] == 'c' && d[6] == 't') {
                // Extract a string field from the params array
                var name_val = resolve_and_extract_field(args, string_view::make_no_len("name"))
                g_params_ok = name_val.find(string_view::make_no_len("test-video")) != name_val.size()
                if(g_wv_params != null) { webview::webview_stop(g_wv_params) }
                return string("{\"ok\":true}")
            }
        }
        return string("{\"ok\":true}")
    })

    var html = string("<html><head><script>")
    html.append_view(string_view::make_no_len("function go(){try{"))
    html.append_view(string_view::make_no_len("window.__webview__.call('extract', JSON.stringify({name:'test-video',formats:['mp4','webm'],quality:1080})).then(function(r){"))
    html.append_view(string_view::make_no_len("window.__webview__.call('done', r);"))
    html.append_view(string_view::make_no_len("}).catch(function(e){window.__webview__.call('done', JSON.stringify({error:e.message}));});"))
    html.append_view(string_view::make_no_len("}catch(e){window.__webview__.call('done', JSON.stringify({error:e.message}));}}"))
    html.append_view(string_view::make_no_len("</script></head><body onload='go()'><p>test</p></body></html>"))

    webview::webview_load_html(&raw mut wv, html.data() as *char)
    webview::webview_show(&raw mut wv)
    webview::webview_run(&raw mut wv)
    webview::webview_destroy(&raw mut wv)

    if(!g_params_ok) {
        env.error("failed to extract field from nested JSON in params array")
        return
    }
}
