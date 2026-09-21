// universal_test runner: renders every #universal_test fixture into one page
// (production SSR + hydration) and drives them sequentially in a single WebView.
// Tests marked `isolate` get their own page + WebView.

using std::string
using std::string_view
using std::vector
using JsonParser
using ASTJsonHandler
using JsonValue

public struct UTFunction {
    var id : int
    var name : std::string_view
    var group : std::string_view
    var isolate : bool
    var fixture_fn : (page : &mut HtmlPage) => void
    var steps : std::string_view
}

struct UTResult {
    var name : string
    var ok : bool
    var msg : string
}

comptime func ut_all() : []UTFunction {
    return intrinsics::get_universal_tests<UTFunction>() as []UTFunction
}

var g_wv : *mut webview::WebView = null
@never_destructed var g_results : vector<UTResult>

func ut_sv_eq(a : &string, b : string_view) : bool {
    if(a.size() != b.size()) { return false }
    var i : size_t = 0
    while(i < a.size()) {
        if(a.get(i) != b.get(i)) { return false }
        i += 1
    }
    return true
}

func ut_parse_result(args : string_view, out : *mut UTResult) {
    out.name = string()
    out.ok = false
    out.msg = string()
    var parser = JsonParser(256, 8192)
    var ph = ASTJsonHandler.make()
    parser.parse(args.data(), args.size(), &mut ph)
    if(ph.root is JsonValue.Array) {
        var Array(arr) = ph.root else unreachable
        if(arr.size() >= 3) {
            var e0 = arr.get_ptr(0)
            if(e0 is JsonValue.String) {
                var String(s) = *e0 else unreachable
                out.name = s.copy()
            }
            var e1 = arr.get_ptr(1)
            if(e1 is JsonValue.Bool) {
                var Bool(b) = *e1 else unreachable
                out.ok = b
            }
            var e2 = arr.get_ptr(2)
            if(e2 is JsonValue.String) {
                var String(s2) = *e2 else unreachable
                out.msg = s2.copy()
            }
        }
    }
}

func ut_bridge(method : string_view, args : string_view) : string {
    if(method.size() == 6) {
        // "result" — one test finished
        var r = UTResult { name : string(), ok : false, msg : string() }
        ut_parse_result(args, &raw mut r)
        g_results.push(r)
    } else if(method.size() == 8) {
        // "all_done" — stop the message loop
        if(g_wv != null) { webview::webview_stop(g_wv) }
    }
    return string("{}")
}

// Runs the given tests as one page in one WebView. Results accumulate in
// g_results keyed by test name.
func ut_execute(tests : *mut *mut UTFunction, count : size_t, headed : bool) {
    if(count == 0) { return }
    var page = HtmlPage()
    page.appendTitle("universal tests")
    page.defaultPrepare()
    page.defaultUniversalSetup()
    page.append_js_char_ptr(UT_HARNESS)

    var i : size_t = 0
    while(i < count) {
        const t = tests[i]
        t.fixture_fn(&mut page)
        i += 1
    }

    var html = page.toString()

    var wv_res = webview::create("universal tests\0" as *char, 900, 700)
    if(wv_res is std::Result.Err) {
        printf("universal_test: failed to create webview (is a display available?)\n")
        return
    }
    var Ok(wv) = wv_res else unreachable
    g_wv = &raw mut wv

    webview::webview_bind(&raw mut wv, (method, args) => {
        return ut_bridge(method, args)
    })

    webview::webview_load_html(&raw mut wv, html.data() as *char)
    if(headed) { webview::webview_show(&raw mut wv) }
    webview::webview_run(&raw mut wv)
    webview::webview_destroy(&raw mut wv)
    g_wv = null
}

func ut_name_selected(names : &vector<string_view>, name : string_view) : bool {
    var i : size_t = 0
    while(i < names.size()) {
        const sp = names.get_ptr(i)
        if(sp.equals(&name)) { return true }
        i += 1
    }
    return false
}

func ut_id_selected(ids : &vector<int>, id : int) : bool {
    var i : size_t = 0
    while(i < ids.size()) {
        const vp = ids.get_ptr(i)
        if(*vp == id) { return true }
        i += 1
    }
    return false
}

func ut_report(tests : *mut *mut UTFunction, count : size_t) : int {
    var passed : int = 0
    var failed : int = 0
    var i : size_t = 0
    while(i < count) {
        const t = tests[i]
        var found = false
        var ok = false
        var msg = string()
        var j : size_t = 0
        while(j < g_results.size()) {
            const r = g_results.get_ptr(j)
            if(ut_sv_eq(&r.name, t.name)) {
                found = true
                ok = r.ok
                msg.append_string(&r.msg)
                break
            }
            j += 1
        }
        if(!found) {
            printf("FAIL %.*s (no result)\n", t.name.size() as int, t.name.data())
            failed += 1
        } else if(ok) {
            printf("PASS %.*s\n", t.name.size() as int, t.name.data())
            passed += 1
        } else {
            printf("FAIL %.*s: %.*s\n", t.name.size() as int, t.name.data(), msg.size() as int, msg.data())
            failed += 1
        }
        i += 1
    }
    printf("\nuniversal tests: %d passed, %d failed\n", passed, failed)
    if(failed > 0) {
        return 1
    }
    return 0
}

// Parses `--test-names a,b` and `--test-ids 1,2`.
func ut_parse_args(argc : int, argv : **char, names : &mut vector<string_view>, ids : &mut vector<int>, has_names : &mut bool, has_ids : &mut bool) {
    var i : int = 1
    while(i < argc) {
        const arg = argv[i]
        if(strcmp(arg, "--test-names") == 0 || strcmp(arg, "-test-names") == 0) {
            i += 1
            if(i < argc) {
                var p = argv[i]
                while(*p) {
                    if(*p == ',') {
                        p += 1
                        continue
                    }
                    const start = p
                    while(*p && *p != ',') { p += 1 }
                    names.push(string_view(start, (p - start) as size_t))
                    *has_names = true
                    if(*p == ',') { p += 1 }
                }
            }
        } else if(strcmp(arg, "--test-ids") == 0 || strcmp(arg, "-test-ids") == 0) {
            i += 1
            if(i < argc) {
                ids.push(atoi(argv[i]))
                *has_ids = true
            }
        }
        i += 1
    }
}

// `universal_test_runner` is comptime so `ut_all()` is evaluated at the user's
// call site -- i.e. after the module containing the #universal_test declarations
// has been parsed and collected (mirrors the `test` library's `test_runner`).
public comptime func universal_test_runner(argc : %maybe_runtime<int>, argv : %runtime<**char>) : int {
    const tests = ut_all()
    return %runtime_value(run_universal_tests(std::span<UTFunction>(tests), argc, argv)) as int
}

@retained
public func run_universal_tests(tests : std::span<UTFunction>, argc : int, argv : **char) : int {
    var span = tests
    if(span.size() == 0) {
        printf("universal_test: no #universal_test declarations found\n")
        return 0
    }
    g_results = vector<UTResult>()

    var names = vector<string_view>()
    var ids = vector<int>()
    var has_names = false
    var has_ids = false
    ut_parse_args(argc, argv, &mut names, &mut ids, &mut has_names, &mut has_ids)

    // select tests, preserving declaration order, and split shared/isolated
    var selected = vector<*mut UTFunction>()
    var isolated = vector<*mut UTFunction>()
    var all_ptr = span.data() as *mut UTFunction
    var i : size_t = 0
    while(i < span.size()) {
        const t = all_ptr + i
        var keep = true
        if(has_names || has_ids) {
            keep = false
            if(has_names && ut_name_selected(&names, t.name)) { keep = true }
            if(has_ids && ut_id_selected(&ids, t.id)) { keep = true }
        }
        if(keep) {
            if(t.isolate) { isolated.push(t) } else { selected.push(t) }
        }
        i += 1
    }

    // run the shared group in one page + webview (the fast default)
    ut_execute(selected.data() as *mut *mut UTFunction, selected.size(), true)

    // each isolated test gets its own page + webview
    var k : size_t = 0
    while(k < isolated.size()) {
        var one = vector<*mut UTFunction>()
        const ip = isolated.get_ptr(k)
        one.push(*ip)
        ut_execute(one.data() as *mut *mut UTFunction, 1 as size_t, true)
        k += 1
    }

    // report in declaration order over the selected tests
    var ordered = vector<*mut UTFunction>()
    i = 0
    while(i < selected.size()) { const sp = selected.get_ptr(i); ordered.push(*sp); i += 1 }
    k = 0
    while(k < isolated.size()) { const ip2 = isolated.get_ptr(k); ordered.push(*ip2); k += 1 }
    return ut_report(ordered.data() as *mut *mut UTFunction, ordered.size())
}
