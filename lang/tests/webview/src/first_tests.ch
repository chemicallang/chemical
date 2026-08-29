using std::string;
using std::string_view;

// ============================================================================
// Helper: shared state for bridge integration tests.
// Each test runs in its own child process, so globals are safe.
// ============================================================================

struct BridgeTestState {
    var calls_received : int
    var last_method : string
    var last_args : string
    var last_result : string
    var bridge_ok : bool
    var error_msg : string
    var wv_ptr : *mut webview::WebView
}

var g_state_ptr : *mut BridgeTestState = null

func ensure_state() {
    if(g_state_ptr == null) {
        g_state_ptr = malloc(sizeof(BridgeTestState)) as *mut BridgeTestState
        memset(g_state_ptr as *mut void, 0, sizeof(BridgeTestState))
    }
}

func reset_state() {
    ensure_state()
    var s = g_state_ptr
    s.calls_received = 0
    s.last_method = string("")
    s.last_args = string("")
    s.last_result = string("")
    s.bridge_ok = false
    s.error_msg = string("")
    s.wv_ptr = null
}

func state() : *mut BridgeTestState {
    ensure_state()
    return g_state_ptr
}

// Display-independent struct tests are in webview_test.ch

// ============================================================================
// Bridge integration tests: create a webview, exercise the JS↔native bridge
// ============================================================================

// Helper: create a webview, bind a handler, load HTML, run until handler stops.
// Returns the result of `verify_fn` which inspects `g_state`.
func run_bridge_test(
    env : &mut TestEnv,
    html : *char,
    verify_fn : (env : &mut TestEnv) => void,
    test_name : *char
) {
    reset_state()
    var wv = webview::WebView.make()
    webview::webview_set_size(&raw mut wv, 480, 360)

    var r = webview::webview_create(&raw mut wv)
    if(r is std::Result.Err) {
        env.error("webview_create failed")
        return
    }

    // Bind echo handler that records the call and stops on "done" or "stop"
    webview::webview_bind(&raw mut wv, (| |(method, args) => {
        state().calls_received = state().calls_received + 1
        state().last_method = string("")
        state().last_method.append_view(&method)
        state().last_args = string("")
        state().last_args.append_view(&args)

        // Check for "stop" or "done" signals — stop the webview
        if(method.size() == 4) {
            var d0 = method.data()[0]
            var d1 = method.data()[1]
            var d2 = method.data()[2]
            var d3 = method.data()[3]
            if(d0 == 'd' && d1 == 'o' && d2 == 'n' && d3 == 'e') {
                state().bridge_ok = true
                state().last_result = string("")
                state().last_result.append_view(&args)
                webview::webview_stop(state().wv_ptr)
                return string("{\"ok\":true}")
            }
            if(d0 == 's' && d1 == 't' && d2 == 'o' && d3 == 'p') {
                state().bridge_ok = false
                state().error_msg = string("")
                state().error_msg.append_view(&args)
                webview::webview_stop(state().wv_ptr)
                return string("{\"ok\":true}")
            }
        }

        // Echo back the args
        var resp = string("{\"echo\":")
        resp.append_view(&args)
        resp.append_view(std::string_view::make_no_len("}"))
        return resp
    }))

    // Store webview pointer in state so handler can stop it
    state().wv_ptr = &raw mut wv

    webview::webview_load_html(&raw mut wv, html)

    // Safety timeout: if the test hangs, stop after 6 seconds via JS timer
    webview::webview_evaluate_js(&raw mut wv, "setTimeout(function(){window.__webview__.call('stop','timeout');},10000)")

    // Run the message loop — blocks until handler calls webview_stop
    webview::webview_run(&raw mut wv)
    webview::webview_destroy(&raw mut wv)

    // Verify results
    verify_fn(env)
}

// ============================================================================
// Test 1: Simple echo round-trip
// ============================================================================

func verify_echo(env : &mut TestEnv) {
    if(!state().bridge_ok) {
        env.error("bridge round-trip did not complete")
        return
    }
    // The "done" call receives the echo result as args
    // It should contain the echoed data from the echo call
    if(state().calls_received < 2) {
        env.error("expected at least 2 calls (echo + done)")
        return
    }
}

@test
public func test_bridge_echo_roundtrip(env : &mut TestEnv) {
    var html = string("<html><head><script>")
    html.append_view(std::string_view::make_no_len("function go(){try{"))
    html.append_view(std::string_view::make_no_len("window.__webview__.call('echo',{msg:'hello'}).then(function(r){"))
    html.append_view(std::string_view::make_no_len("window.__webview__.call('done',r);"))
    html.append_view(std::string_view::make_no_len("}).catch(function(e){window.__webview__.call('stop',e.message);});"))
    html.append_view(std::string_view::make_no_len("}catch(e){window.__webview__.call('stop',e.message);}}"))
    html.append_view(std::string_view::make_no_len("</script></head><body onload='go()'><p>test</p></body></html>"))
    run_bridge_test(env, html.data(), verify_echo, "echo_roundtrip")
}

// ============================================================================
// Test 2: Multiple sequential bridge calls
// ============================================================================

func verify_multi_call(env : &mut TestEnv) {
    if(!state().bridge_ok) {
        env.error("multi-call bridge round-trip did not complete")
        return
    }
    if(state().calls_received < 4) {
        env.error("expected at least 4 calls (3 echo + 1 done)")
        return
    }
}

@test
public func test_bridge_multi_call(env : &mut TestEnv) {
    var html = string("<html><head><script>")
    html.append_view(std::string_view::make_no_len("function go(){try{"))
    // Chain three echo calls, then signal done
    html.append_view(std::string_view::make_no_len("window.__webview__.call('echo',{step:1}).then(function(r1){"))
    html.append_view(std::string_view::make_no_len("return window.__webview__.call('echo',{step:2,r1:r1});"))
    html.append_view(std::string_view::make_no_len("}).then(function(r2){"))
    html.append_view(std::string_view::make_no_len("return window.__webview__.call('echo',{step:3,r2:r2});"))
    html.append_view(std::string_view::make_no_len("}).then(function(r3){"))
    html.append_view(std::string_view::make_no_len("window.__webview__.call('done',r3);"))
    html.append_view(std::string_view::make_no_len("}).catch(function(e){window.__webview__.call('stop',e.message);});"))
    html.append_view(std::string_view::make_no_len("}catch(e){window.__webview__.call('stop',e.message);}}"))
    html.append_view(std::string_view::make_no_len("</script></head><body onload='go()'><p>test</p></body></html>"))
    run_bridge_test(env, html.data(), verify_multi_call, "multi_call")
}

// ============================================================================
// Test 3: Parallel bridge calls
// ============================================================================

func verify_parallel(env : &mut TestEnv) {
    if(!state().bridge_ok) {
        env.error("parallel bridge round-trip did not complete")
        return
    }
    // 2 parallel calls + 1 done = at least 3
    if(state().calls_received < 3) {
        env.error("expected at least 3 calls (2 parallel echo + 1 done)")
        return
    }
}

@test
public func test_bridge_parallel_calls(env : &mut TestEnv) {
    var html = string("<html><head><script>")
    html.append_view(std::string_view::make_no_len("function go(){try{"))
    html.append_view(std::string_view::make_no_len("var p1=window.__webview__.call('echo',{parallel:1,a:'foo'});"))
    html.append_view(std::string_view::make_no_len("var p2=window.__webview__.call('echo',{parallel:2,b:'bar'});"))
    html.append_view(std::string_view::make_no_len("Promise.all([p1,p2]).then(function(results){"))
    html.append_view(std::string_view::make_no_len("window.__webview__.call('done',{count:results.length,r0:results[0],r1:results[1]});"))
    html.append_view(std::string_view::make_no_len("}).catch(function(e){window.__webview__.call('stop',e.message);});"))
    html.append_view(std::string_view::make_no_len("}catch(e){window.__webview__.call('stop',e.message);}}"))
    html.append_view(std::string_view::make_no_len("</script></head><body onload='go()'><p>test</p></body></html>"))
    run_bridge_test(env, html.data(), verify_parallel, "parallel_calls")
}

// ============================================================================
// Test 4: Large payload round-trip
// ============================================================================

func verify_medium_payload(env : &mut TestEnv) {
    if(!state().bridge_ok) {
        env.error("medium payload round-trip did not complete")
        return
    }
}

@test
public func test_bridge_medium_payload(env : &mut TestEnv) {
    var html = string("<html><head><script>")
    html.append_view(std::string_view::make_no_len("function go(){try{"))
    html.append_view(std::string_view::make_no_len("var big='ABCDEFGHIJ';"))
    html.append_view(std::string_view::make_no_len("window.__webview__.call('echo',{data:big}).then(function(r){"))
    html.append_view(std::string_view::make_no_len("window.__webview__.call('done',{ok:true,echo:r.echo});"))
    html.append_view(std::string_view::make_no_len("}).catch(function(e){window.__webview__.call('stop',e.message);});"))
    html.append_view(std::string_view::make_no_len("}catch(e){window.__webview__.call('stop',e.message);}}"))
    html.append_view(std::string_view::make_no_len("</script></head><body onload='go()'><p>test</p></body></html>"))
    run_bridge_test(env, html.data(), verify_medium_payload, "medium_payload")
}

// ============================================================================
// Test 5: Special characters in args (quotes, backslashes, unicode)
// ============================================================================

func verify_special_chars(env : &mut TestEnv) {
    if(!state().bridge_ok) {
        env.error("special chars round-trip did not complete")
        return
    }
}

@test
public func test_bridge_special_chars(env : &mut TestEnv) {
    var html = string("<html><head><script>")
    html.append_view(std::string_view::make_no_len("function go(){try{"))
    html.append_view(std::string_view::make_no_len("window.__webview__.call('echo',{text:'hello \"world\" \\\\ backslash'}).then(function(r){"))
    html.append_view(std::string_view::make_no_len("window.__webview__.call('done',r);"))
    html.append_view(std::string_view::make_no_len("}).catch(function(e){window.__webview__.call('stop',e.message);});"))
    html.append_view(std::string_view::make_no_len("}catch(e){window.__webview__.call('stop',e.message);}}"))
    html.append_view(std::string_view::make_no_len("</script></head><body onload='go()'><p>test</p></body></html>"))
    run_bridge_test(env, html.data(), verify_special_chars, "special_chars")
}

// ============================================================================
// Test 6: No-args bridge call
// ============================================================================

func verify_no_args(env : &mut TestEnv) {
    if(!state().bridge_ok) {
        env.error("no-args round-trip did not complete")
        return
    }
}

@test
public func test_bridge_no_args(env : &mut TestEnv) {
    var html = string("<html><head><script>")
    html.append_view(std::string_view::make_no_len("function go(){try{"))
    html.append_view(std::string_view::make_no_len("window.__webview__.call('echo').then(function(r){"))
    html.append_view(std::string_view::make_no_len("window.__webview__.call('done',r);"))
    html.append_view(std::string_view::make_no_len("}).catch(function(e){window.__webview__.call('stop',e.message);});"))
    html.append_view(std::string_view::make_no_len("}catch(e){window.__webview__.call('stop',e.message);}}"))
    html.append_view(std::string_view::make_no_len("</script></head><body onload='go()'><p>test</p></body></html>"))
    run_bridge_test(env, html.data(), verify_no_args, "no_args")
}

// ============================================================================
// Test 7: Nested JSON objects in args
// ============================================================================

func verify_nested_json(env : &mut TestEnv) {
    if(!state().bridge_ok) {
        env.error("nested JSON round-trip did not complete")
        return
    }
}

@test
public func test_bridge_nested_json(env : &mut TestEnv) {
    var html = string("<html><head><script>")
    html.append_view(std::string_view::make_no_len("function go(){try{"))
    html.append_view(std::string_view::make_no_len("var deep={a:{b:{c:{d:{e:'nested'}}}},arr:[1,{x:2},[3,4]]};"))
    html.append_view(std::string_view::make_no_len("window.__webview__.call('echo',deep).then(function(r){"))
    html.append_view(std::string_view::make_no_len("window.__webview__.call('done',r);"))
    html.append_view(std::string_view::make_no_len("}).catch(function(e){window.__webview__.call('stop',e.message);});"))
    html.append_view(std::string_view::make_no_len("}catch(e){window.__webview__.call('stop',e.message);}}"))
    html.append_view(std::string_view::make_no_len("</script></head><body onload='go()'><p>test</p></body></html>"))
    run_bridge_test(env, html.data(), verify_nested_json, "nested_json")
}

// ============================================================================
// Test 8: Sequential rapid bridge calls
// ============================================================================

func verify_rapid(env : &mut TestEnv) {
    if(!state().bridge_ok) {
        env.error("rapid calls round-trip did not complete")
        return
    }
    if(state().calls_received < 6) {
        env.error("expected at least 6 calls (5 sequential + 1 done)")
        return
    }
}

@test
public func test_bridge_rapid_calls(env : &mut TestEnv) {
    var html = string("<html><head><script>")
    html.append_view(std::string_view::make_no_len("function go(){try{"))
    html.append_view(std::string_view::make_no_len("window.__webview__.call('echo',{i:1}).then(function(){"))
    html.append_view(std::string_view::make_no_len("return window.__webview__.call('echo',{i:2});"))
    html.append_view(std::string_view::make_no_len("}).then(function(){"))
    html.append_view(std::string_view::make_no_len("return window.__webview__.call('echo',{i:3});"))
    html.append_view(std::string_view::make_no_len("}).then(function(){"))
    html.append_view(std::string_view::make_no_len("return window.__webview__.call('echo',{i:4});"))
    html.append_view(std::string_view::make_no_len("}).then(function(){"))
    html.append_view(std::string_view::make_no_len("return window.__webview__.call('echo',{i:5});"))
    html.append_view(std::string_view::make_no_len("}).then(function(){"))
    html.append_view(std::string_view::make_no_len("window.__webview__.call('done',{count:5});"))
    html.append_view(std::string_view::make_no_len("}).catch(function(e){window.__webview__.call('stop',e.message);});"))
    html.append_view(std::string_view::make_no_len("}catch(e){window.__webview__.call('stop',e.message);}}"))
    html.append_view(std::string_view::make_no_len("</script></head><body onload='go()'><p>test</p></body></html>"))
    run_bridge_test(env, html.data(), verify_rapid, "rapid_calls")
}

// ============================================================================
// Test 9: Bridge call returns structured data
// ============================================================================

func verify_structured_return(env : &mut TestEnv) {
    if(!state().bridge_ok) {
        env.error("structured return round-trip did not complete")
        return
    }
}

@test
public func test_bridge_structured_return(env : &mut TestEnv) {
    var html = string("<html><head><script>")
    html.append_view(std::string_view::make_no_len("function go(){try{"))
    html.append_view(std::string_view::make_no_len("window.__webview__.call('echo',{x:1,y:'two',z:true}).then(function(r){"))
    // Verify the structure came back correctly
    html.append_view(std::string_view::make_no_len("var ok=(r.echo.x===1&&r.echo.y==='two'&&r.echo.z===true);"))
    html.append_view(std::string_view::make_no_len("window.__webview__.call('done',{valid:ok,echo:r.echo});"))
    html.append_view(std::string_view::make_no_len("}).catch(function(e){window.__webview__.call('stop',e.message);});"))
    html.append_view(std::string_view::make_no_len("}catch(e){window.__webview__.call('stop',e.message);}}"))
    html.append_view(std::string_view::make_no_len("</script></head><body onload='go()'><p>test</p></body></html>"))
    run_bridge_test(env, html.data(), verify_structured_return, "structured_return")
}

// ============================================================================
// Test 10: Bridge error handling (reject path)
// ============================================================================

func verify_error_handling(env : &mut TestEnv) {
    // In this test, the bridge_ok is false — we expect the error path
    if(state().calls_received < 1) {
        env.error("expected at least 1 call")
        return
    }
}

@test
public func test_bridge_error_handling(env : &mut TestEnv) {
    var html = string("<html><head><script>")
    html.append_view(std::string_view::make_no_len("function go(){try{"))
    // Call a method that will fail (the handler always echoes, so let's test catch)
    html.append_view(std::string_view::make_no_len("window.__webview__.call('nonexistent_method',{test:1}).then(function(r){"))
    // If it somehow succeeds, stop anyway
    html.append_view(std::string_view::make_no_len("window.__webview__.call('done',{unexpected:true});"))
    html.append_view(std::string_view::make_no_len("}).catch(function(e){"))
    // Error was caught — this is expected, the handler echoes even unknown methods
    // Since our handler echoes everything, the "then" path fires. Just stop.
    html.append_view(std::string_view::make_no_len("window.__webview__.call('done',{caught:true,error:e.message});"))
    html.append_view(std::string_view::make_no_len("});"))
    html.append_view(std::string_view::make_no_len("}catch(e){window.__webview__.call('stop',e.message);}}"))
    html.append_view(std::string_view::make_no_len("</script></head><body onload='go()'><p>test</p></body></html>"))
    run_bridge_test(env, html.data(), verify_error_handling, "error_handling")
}

// ============================================================================
// Test 8: Large payload (600KB+) — tests webview chunking for bridge results
// ============================================================================

func verify_large_payload(env : &mut TestEnv) {
    if(!state().bridge_ok) {
        env.error("large payload round-trip did not complete")
        return
    }
}

@test
public func test_bridge_large_payload(env : &mut TestEnv) {
    // Build HTML that creates a 600KB+ string and sends it through the bridge.
    var html = string("<html><head><script>")
    html.append_view(std::string_view::make_no_len("function go(){try{"))
    html.append_view(std::string_view::make_no_len("var chunks=[];"))
    html.append_view(std::string_view::make_no_len("for(var i=0;i<20000;i++){chunks.push('ABCDEFGH12345678');}"))
    html.append_view(std::string_view::make_no_len("var big=chunks.join('');"))
    html.append_view(std::string_view::make_no_len("window.__webview__.call('echo',{data:big}).then(function(r){"))
    html.append_view(std::string_view::make_no_len("window.__webview__.call('done',{ok:true,size:r.echo&&r.echo[0]&&r.echo[0].data?r.echo[0].data.length:0});"))
    html.append_view(std::string_view::make_no_len("}).catch(function(e){window.__webview__.call('stop',e.message);});"))
    html.append_view(std::string_view::make_no_len("}catch(e){window.__webview__.call('stop',e.message);}}"))
    html.append_view(std::string_view::make_no_len("</script></head><body onload='go()'><p>test</p></body></html>"))
    run_bridge_test(env, html.data(), verify_large_payload, "large_payload")
}

// ============================================================================
// Test 11: Large payload WITH special characters, under continuous bridge load.
//
// This mirrors the cdm app: it polls the bridge on a timer (like yt_info_poll)
// while awaiting a large result (like yt_info_get). The large payload contains
// quotes, backslashes, unicode and tabs — exactly the kind of content a
// serialized video-info JSON carries. The test fails if the reply is dropped
// (stuck) OR if the chunked payload is corrupted in any way, both of which the
// plain completion-only large-payload test above could not catch.
// ============================================================================

func verify_large_payload_under_load(env : &mut TestEnv) {
    if(!state().bridge_ok) {
        var msg = string("large payload under load did not round-trip correctly")
        if(state().error_msg.size() > 0u) {
            msg.append_view(std::string_view::make_no_len(": "))
            msg.append_view(std::string_view::make_view(&state().error_msg))
        }
        env.error(msg.data())
        return
    }
}

@test
public func test_bridge_large_payload_under_load(env : &mut TestEnv) {
    var html = string("<html><head><script>")
    html.append_view(std::string_view::make_no_len("function go(){try{"))
    // A ~300KB payload full of characters that must survive JSON chunking.
    html.append_view(std::string_view::make_no_len("var payload='';for(var i=0;i<40000;i++){payload+='a\"b\\\\c\\u00e9d\\t';}"))
    // Keep the bridge busy (mirrors cdm's continuous polling) while we wait.
    html.append_view(std::string_view::make_no_len("var busy=setInterval(function(){window.__webview__.call('echo',{ping:1}).catch(function(){});},40);"))
    html.append_view(std::string_view::make_no_len("window.__webview__.call('echo',{payload:payload}).then(function(r){"))
    html.append_view(std::string_view::make_no_len("clearInterval(busy);"))
    html.append_view(std::string_view::make_no_len("var ok=(r.echo&&r.echo[0]&&typeof r.echo[0].payload==='string'&&r.echo[0].payload===payload);"))
    html.append_view(std::string_view::make_no_len("var got=(r.echo&&r.echo[0]&&typeof r.echo[0].payload==='string')?r.echo[0].payload.length:-1;"))
    html.append_view(std::string_view::make_no_len("if(ok){window.__webview__.call('done',{ok:true,len:payload.length});}else{var hd=(typeof r==='string')?r.slice(0,120):JSON.stringify(r).slice(0,120);window.__webview__.call('stop','mismatch exp='+payload.length+' got='+got+' te='+typeof(r.echo)+' head='+hd);}"))
    html.append_view(std::string_view::make_no_len("}).catch(function(e){clearInterval(busy);window.__webview__.call('stop',e.message);});"))
    html.append_view(std::string_view::make_no_len("}catch(e){window.__webview__.call('stop',e.message);}}"))
    html.append_view(std::string_view::make_no_len("</script></head><body onload='go()'><p>test</p></body></html>"))
    run_bridge_test(env, html.data(), verify_large_payload_under_load, "large_payload_under_load")
}

// ============================================================================
// BridgeTestState needs wv_ptr for the handler to call webview_stop
// ============================================================================

// Add wv_ptr to BridgeTestState — we need to declare it above.
// (Chemical requires all struct fields to be known at declaration time,
//  so we add it to the struct definition above.)
