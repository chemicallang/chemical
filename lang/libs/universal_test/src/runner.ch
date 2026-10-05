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
    /**
     * true when this universal test comes from a remote (downloaded) module.
     * the runner skips remote tests by default; pass --include-remote to run them.
     */
    var is_remote : bool
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

// ── Host-side watchdog ──────────────────────────────────────────────────────
//
// `webview_run` blocks in the platform message loop and returns only when the
// page calls the `all_done` bridge method. Nothing else ends it, and the
// page-side per-test timeout cannot cover the gap: it is itself JavaScript, so
// it is exactly what a bundle that fails to parse cannot run. Without a host
// bound, one broken fixture leaves the whole suite blocked with no output.
//
// The bound is a SILENCE budget rather than a total-runtime budget: every
// bridge call proves the page is alive and resets it, so the budget only ever
// has to cover one test's worth of inactivity. That keeps it independent of how
// many tests share the page.
comptime const UT_SILENCE_MS : int = 60000
comptime const UT_WATCHDOG_TICK_MS : int = 500

var g_silence_ms : int = 0          // resolved lazily by ut_silence_budget_ms()
var g_silence_left_ms : int = 0     // remaining budget; 0 = disarmed
var g_watchdog_id : int = 0
// Names on the page currently running, so a stall can be charged to them.
@never_destructed var g_page_names : vector<string>

// Milliseconds of bridge silence before a page is declared stalled.
// `UT_TIMEOUT_MS` overrides it (ms) for slow machines.
func ut_silence_budget_ms() : int {
    if(g_silence_ms > 0) { return g_silence_ms }
    var budget = UT_SILENCE_MS
    const env = getenv("UT_TIMEOUT_MS\0" as *char)
    if(env != null) {
        const parsed = atoi(env)
        if(parsed > 0) { budget = parsed }
    }
    g_silence_ms = budget
    return g_silence_ms
}

func ut_stall_message() : string {
    var m = string("no result: the page sent no bridge call for ")
    m.append_integer(ut_silence_budget_ms() / 1000)
    m.append_view("s and was stopped (its JS bundle probably fails to parse -- dump it with UT_DUMP_JS=1 and check it with `node --check`)")
    return m
}

// Runs from the platform timer inside `webview_run`'s message loop.
func ut_watchdog_tick(data : *mut void) {
    if(g_silence_left_ms <= 0) { return }
    g_silence_left_ms -= UT_WATCHDOG_TICK_MS
    if(g_silence_left_ms > 0) { return }
    g_silence_left_ms = 0
    printf("universal_test: page sent no bridge call for %d s -- stopping the webview and failing its tests\n",
        ut_silence_budget_ms() / 1000)
    // Synthesize a result for every test on the stalled page. Tests that did
    // report already have one, and the report keeps the first result per name,
    // so only the genuinely silent ones are charged with the stall.
    const reason = ut_stall_message()
    var i : size_t = 0
    while(i < g_page_names.size()) {
        const nm = g_page_names.get_ptr(i)
        var r = UTResult { name : nm.copy(), ok : false, msg : reason.copy() }
        g_results.push(r)
        i += 1
    }
    if(g_wv != null) { webview::webview_stop(g_wv) }
}

// Escapes `v` for embedding inside a double-quoted JS string literal.
func ut_js_quote(v : std::string_view, out : &mut std::string) {
    for(var i : size_t = 0; i < v.size(); i++) {
        const c = v.get(i)
        if(c == '"') { out.append_view(std::string_view("\\\"")) }
        else if(c == '\\') { out.append_view(std::string_view("\\\\")) }
        else if(c == '\n') { out.append_view(std::string_view("\\n")) }
        else if(c == '\r') { out.append_view(std::string_view("\\r")) }
        else if(c == '\t') { out.append_view(std::string_view("\\t")) }
        else { out.append(c) }
    }
}

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
        // "result" — one test finished. Any bridge call is proof the page is
        // alive, so it refills the watchdog's silence budget.
        g_silence_left_ms = ut_silence_budget_ms()
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

    // Fixtures render into the body: SSR markup plus the client bundle and the
    // `__ut_register` call that hands each test's steps to the harness.
    var i : size_t = 0
    g_page_names = vector<string>()
    while(i < count) {
        const t = tests[i]
        t.fixture_fn(&mut page)
        var nm = string()
        nm.append_view(&t.name)
        g_page_names.push(nm)
        i += 1
    }

    // The harness goes into the HEAD script block, which `toString` emits as its
    // own `<script>` BEFORE the body bundle holding the fixtures' steps. Keeping
    // it out of that bundle is what makes a broken fixture survivable: a script
    // that fails to parse is skipped whole, so anything sharing its block -- the
    // harness's own error handler and `all_done` included -- would be lost, and
    // the host would block forever. The head block has already run, so it can
    // report the failure and end the loop instead.
    //
    // Names the page is expected to run go in just ahead of it, so a fixture
    // whose `__ut_register` never executed is still reported by name.
    var expected = std::string("\nwindow.__ut_expected = [")
    i = 0
    while(i < count) {
        const t = tests[i]
        if(i > 0) { expected.append_view(",") }
        expected.append_view("\"")
        ut_js_quote(t.name, &mut expected)
        expected.append_view("\"")
        i += 1
    }
    expected.append_view("];\n;\n")
    page.append_head_js_view(expected.to_view())
    page.append_head_js_view(std::string_view(UT_HARNESS))

    var html = page.toString()

    // Debug: dump the generated page JS (UT_DUMP_JS=1) so it can be syntax-checked.
    // Returns without opening a WebView. Covers every script element `toString`
    // emits -- head block, body bundle and body tail -- since a syntax error in
    // any of them is the failure this dump exists to find.
    if(getenv("UT_DUMP_JS\0" as *char) != null) {
        var jsdump = page.toStringHeadJsOnly()
        jsdump.append_view("\n/* ==== body js bundle ==== */\n")
        jsdump.append_string(&page.toStringJsOnly())
        jsdump.append_view("\n/* ==== body js tail ==== */\n")
        jsdump.append_string(&page.toStringJsEndOnly())
        printf("===UT_JS_START===\n%.*s\n===UT_JS_END===\n", jsdump.size() as int, jsdump.data())
        return
    }

    // Debug: dump the generated page HTML (UT_DUMP_HTML=1) so the SSR markup
    // (boundary ids, fixture containers, duplicated ids/names) can be inspected.
    // Returns without opening a WebView.
    if(getenv("UT_DUMP_HTML\0" as *char) != null) {
        printf("===UT_HTML_START===\n%.*s\n===UT_HTML_END===\n", html.size() as int, html.data())
        return
    }

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
    // Hidden by default: the WebView runs JS off-screen so no window flashes.
    // `--ut-headed` shows it for debugging.
    if(headed) { webview::webview_show(&raw mut wv) }

    // Arm the watchdog so a page that never reports cannot block forever.
    g_silence_left_ms = ut_silence_budget_ms()
    g_watchdog_id = window::window_set_timer(UT_WATCHDOG_TICK_MS, ut_watchdog_tick as window::TimerCallback, null)
    if(g_watchdog_id == 0) {
        // Every timer slot is taken. The page-side error reporting still holds,
        // but say so rather than leaving the host unbounded and unexplained.
        printf("universal_test: could not arm the webview watchdog (no free timer slot) -- a silent page will hang this run\n")
        g_silence_left_ms = 0
    }

    webview::webview_run(&raw mut wv)

    if(g_watchdog_id > 0) {
        window::window_cancel_timer(g_watchdog_id)
        g_watchdog_id = 0
    }
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

// Streams the collected results (in declaration order) to the reporter and
// returns the number of failed tests.
func ut_emit(tests : *mut *mut UTFunction, count : size_t, reporter : *mut TestReporter) : int {
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

        var outcome = make_test_outcome(string_view("universal_test"), t.name, t.group, t.id)
        if(!found) {
            msg.append_view("no result: the page never reported this test")
            outcome.message = msg.to_view()
            failed += 1
        } else if(ok) {
            outcome.passed = true
        } else {
            outcome.message = msg.to_view()
            failed += 1
        }

        if(reporter != null) {
            reporter.on_test(&raw outcome)
        }
        i += 1
    }
    return failed
}

// Parses `--test-names a,b`, `--test-ids 1,2`, and `--include-remote`.
func ut_parse_args(argc : int, argv : **char, names : &mut vector<string_view>, ids : &mut vector<int>, has_names : &mut bool, has_ids : &mut bool, headed : &mut bool, include_remote : &mut bool) {
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
        } else if(strcmp(arg, "--ut-headed") == 0 || strcmp(arg, "--headed") == 0) {
            *headed = true
        } else if(strcmp(arg, "--include-remote") == 0 || strcmp(arg, "-include-remote") == 0) {
            *include_remote = true
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

/**
 * Runs the #universal_test tests, streaming every finished test to `reporter`.
 * Returns the number of failed tests.
 */
@retained
public func run_universal_tests_reporting(tests : std::span<UTFunction>, reporter : &mut TestReporter, argc : int, argv : **char) : int {
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
    var headed = false
    var include_remote = false
    ut_parse_args(argc, argv, &mut names, &mut ids, &mut has_names, &mut has_ids, &mut headed, &mut include_remote)

    // select tests, preserving declaration order, and split shared/isolated
    var selected = vector<*mut UTFunction>()
    var isolated = vector<*mut UTFunction>()
    var all_ptr = span.data() as *mut UTFunction
    var i : size_t = 0
    while(i < span.size()) {
        const t = all_ptr + i
        // remote (downloaded) modules' tests are skipped unless --include-remote
        if(t.is_remote && !include_remote) {
            i += 1
            continue
        }
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

    reporter.begin_runner(string_view("universal_test"))

    // run the shared group in one page + webview (the fast default)
    ut_execute(selected.data() as *mut *mut UTFunction, selected.size(), headed)

    // each isolated test gets its own page + webview
    var k : size_t = 0
    while(k < isolated.size()) {
        var one = vector<*mut UTFunction>()
        const ip = isolated.get_ptr(k)
        one.push(*ip)
        ut_execute(one.data() as *mut *mut UTFunction, 1 as size_t, headed)
        k += 1
    }

    // report in declaration order over the selected tests
    var ordered = vector<*mut UTFunction>()
    i = 0
    while(i < selected.size()) { const sp = selected.get_ptr(i); ordered.push(*sp); i += 1 }
    k = 0
    while(k < isolated.size()) { const ip2 = isolated.get_ptr(k); ordered.push(*ip2); k += 1 }

    var failed = ut_emit(ordered.data() as *mut *mut UTFunction, ordered.size(), &raw mut reporter)
    reporter.end_runner()
    return failed
}

/** Single-runner entry point: streams to a default console reporter. */
@retained
public func run_universal_tests(tests : std::span<UTFunction>, argc : int, argv : **char) : int {
    var console = new_console_reporter()
    run_universal_tests_reporting(tests, &mut console, argc, argv)
    return finish_reporter(&mut console)
}

type UTHandleRunFn = (h : *mut TestRunnerHandle, reporter : &mut TestReporter, argc : int, argv : **char) => int

@retained
public func universal_handle_run(h : *mut TestRunnerHandle, reporter : &mut TestReporter, argc : int, argv : **char) : int {
    var ptr = h.tests_ptr as *mut UTFunction
    var span = std::span<UTFunction>(ptr, h.tests_count)
    return run_universal_tests_reporting(span, reporter, argc, argv)
}

@retained
public func make_universal_runner_handle(tests : std::span<UTFunction>) : TestRunnerHandle {
    return TestRunnerHandle {
        name : string_view("universal_test"),
        tests_ptr : tests.data() as *void,
        tests_count : tests.size(),
        run_fn : universal_handle_run as UTHandleRunFn
    }
}

/**
 * Comptime builder for a runnable #universal_test runner handle. Evaluated at
 * the call site so `ut_all()` sees the caller's declarations.
 */
public comptime func universal_test_runner_handle() : TestRunnerHandle {
    const tests = ut_all()
    return %runtime_value(make_universal_runner_handle(std::span<UTFunction>(tests)))
}
