// Shared test reporting abstraction.
//
// A *test runner* discovers and executes a set of tests and streams each
// finished test to a *TestReporter*. Because every runner speaks the same
// reporter, several runners can share one reporter so a single executable can
// combine them under one unified, coloured output stream and one final
// summary (per-runner counts + grand totals).
//
// The presentation is deliberately identical to the historical `test` library
// output (`  - name id`, indented logs, `    PASS/FAIL`, `Test run summary`,
// `Summary: N tests - P passed, F failed`) so existing tooling
// (scripts/test.sh) keeps parsing it.

using std::string
using std::string_view
using std::vector

/* ------------------------------------------------------------------ */
/*  Colors (moved from the test library's printing.ch)                 */
/* ------------------------------------------------------------------ */

public comptime const ANSI_RESET =  "\x1b[0m"
public comptime const ANSI_BOLD =   "\x1b[1m"
public comptime const ANSI_RED =    "\x1b[31m"
public comptime const ANSI_GREEN =  "\x1b[32m"
public comptime const ANSI_YELLOW = "\x1b[33m"
public comptime const ANSI_BLUE =   "\x1b[34m"
public comptime const ANSI_MAGENTA = "\x1b[35m"
public comptime const ANSI_CYAN =   "\x1b[36m"
public comptime const ANSI_GRAY =   "\x1b[90m"

var colors_enabled : bool = false

public func enable_terminal_colors() : bool {
    // TODO: enable VT processing on Windows 10+ so ANSI codes work.
    return true
}

func col_reset() : *char {
    if(colors_enabled) return ANSI_RESET else return "";
}
func col_bold() : *char  {
    if(colors_enabled) return ANSI_BOLD else return "";
}
func col_red() : *char   {
    if(colors_enabled) return ANSI_RED else return "";
}
func col_green() : *char {
    if(colors_enabled) return ANSI_GREEN else return "";
}
func col_yellow() : *char{
    if(colors_enabled) return ANSI_YELLOW else return "";
}
func col_blue() : *char  {
    if(colors_enabled) return ANSI_BLUE else return "";
}
func col_mag() : *char   {
    if(colors_enabled) return ANSI_MAGENTA else return "";
}
func col_cyan() : *char  {
    if(colors_enabled) return ANSI_CYAN else return "";
}
func col_gray() : *char  {
    if(colors_enabled) return ANSI_GRAY else return "";
}

/* ------------------------------------------------------------------ */
/*  Shared data types                                                  */
/* ------------------------------------------------------------------ */

/** A single structured log line attached to a finished test. */
public struct TestLog {

    public var type : LogType = LogType.Success

    public var message : string

    public var line : ubigint = 0

    public var character : ubigint = 0

}

/** Controls what the console reporter prints (counting is unaffected). */
public struct TestDisplayConfig {
    /** display successful tests only */
    public var successful_only : bool = false;
    /** display failed tests only */
    public var failure_only : bool = false;
    /** when false, no logs will be displayed */
    public var display_logs : bool = true;
}

/**
 * One finished test, as streamed from a runner to a reporter.
 *
 * Every field is trivially copyable (views + raw pointers), so an outcome is
 * cheap to build on the stack; borrowed strings/pointers only need to stay
 * alive for the duration of the synchronous `on_test` call.
 */
public struct TestOutcome {
    /** which runner produced this test (e.g. "test", "universal_test") */
    public var runner : string_view
    /** the test's display name */
    public var name : string_view
    /** the group the test belongs to (may be empty) */
    public var group : string_view
    /** the test's numeric id (may be 0 / negative when not applicable) */
    public var id : int
    /** did the test pass */
    public var passed : bool
    /** was the test skipped */
    public var skipped : bool
    /** process exit code, when the runner has one */
    public var exit_code : int
    /** failure detail (borrowed; empty on success) */
    public var message : string_view
    /** borrowed array of structured logs (may be null) */
    public var logs_ptr : *TestLog
    /** number of entries in `logs_ptr` */
    public var logs_count : size_t
    /** wall-clock duration in milliseconds (0 when unknown) */
    public var duration_ms : ubigint
}

/** Build an outcome with sensible defaults; assign the rest afterwards. */
public func make_test_outcome(runner : string_view, name : string_view, group : string_view, id : int) : TestOutcome {
    return TestOutcome {
        runner : runner,
        name : name,
        group : group,
        id : id,
        passed : false,
        skipped : false,
        exit_code : 0,
        message : string_view(""),
        logs_ptr : null,
        logs_count : 0,
        duration_ms : 0
    }
}

/* ------------------------------------------------------------------ */
/*  The reporter interface                                             */
/* ------------------------------------------------------------------ */

/**
 * A sink for streamed test events.
 *
 * Lifecycle, driven by the runner(s):
 *   begin_runner(name) -> on_test(outcome) * N -> end_runner()  (per runner)
 *   ... repeat for each runner ...
 *   finish()                                                     (once)
 *
 * `finish` returns non-zero when at least one test failed, so it can be used
 * directly as a process exit status.
 */
@static
public interface TestReporter {

    /** configure how tests are displayed (counting is unaffected) */
    func configure(&mut self, display : *TestDisplayConfig)

    /** a new runner is about to stream its tests */
    func begin_runner(&mut self, name : string_view)

    /** one test finished (streamed live) */
    func on_test(&mut self, outcome : *TestOutcome)

    /** the current runner finished streaming */
    func end_runner(&mut self)

    /** every runner is done; prints the final summary and returns failure count status */
    func finish(&mut self) : int

}

/* ------------------------------------------------------------------ */
/*  Log formatting helpers                                             */
/* ------------------------------------------------------------------ */

public func log_type_name(t : LogType) : *char {
    switch (t) {
        LogType.Information => return "INFORMATION";
        LogType.Warning => return "WARNING"
        LogType.Success => return "SUCCESS";
        LogType.Error => return "ERROR"
        LogType.UnknownFailure => return "UNKNOWN";
        LogType.ConfigFailure => return "CONFIG";
        LogType.IOFailure => return "I/O";
        LogType.NetworkFailure => return "NETWORK";
        LogType.MemoryFailure => return "MEMORY";
        LogType.RuntimeFailure => return "RUNTIME";
        LogType.OutOfBoundFailure => return "OUT_OF_BOUND";
        LogType.ResourceFailure => return "RESOURCE";
        LogType.TodoFailure => return "TODO";
        LogType.SecurityFailure => return "SECURITY";
        LogType.WTFFailure => return "WTF";
        default => return "UNKNOWN";
    }
}

/* Short icon for log severity */
func log_type_icon(t : LogType) : *char {
    switch (t) {
        LogType.Warning => return "\x1b[33m!\x1b[0m";
        LogType.Information => return "\x1b[34mi\x1b[0m"
        LogType.Success =>       return "\x1b[32m+\x1b[0m";   // green +
        LogType.TodoFailure =>    return "\x1b[36m...\x1b[0m"; // cyan dots
        LogType.WTFFailure =>     return "\x1b[91m!!\x1b[0m";  // bright red !!
        LogType.SecurityFailure => return "\x1b[93m##\x1b[0m";  // yellow ##
        LogType.UnknownFailure, LogType.Error => return "\x1b[31mx\x1b[0m";   // red x
        default =>                     return "\x1b[31mx\x1b[0m";   // red x
    }
}

/* Color by severity */
func log_type_color(t : LogType) : *char {
    switch (t) {
        LogType.Success => {
            return col_green();
        }
        LogType.TodoFailure => {
            return col_yellow();
        }
        LogType.UnknownFailure, LogType.WTFFailure, LogType.SecurityFailure, LogType.ConfigFailure,
        LogType.IOFailure, LogType.NetworkFailure, LogType.MemoryFailure, LogType.RuntimeFailure,
        LogType.OutOfBoundFailure, LogType.ResourceFailure => {
            return col_red();
        }
        default => {
            return col_reset();
        }
    }
}

func print_log_multiline(out : *mut FILE, msg : *char) {
    if (!out || !msg) return;

    var p = msg;
    while (*p) {
        const nl = strchr(p, '\n');
        var len : size_t
        if(nl) {
            len = (nl - p) as size_t
        } else {
            len = strlen(p);
        }
        fprintf(out, "       %s%.*s%s\n", col_gray(), len as int, p, col_reset());
        if (!nl) break;
        p = nl + 1;
    }
    fflush(out);
}

/* ------------------------------------------------------------------ */
/*  Console reporter                                                   */
/* ------------------------------------------------------------------ */

/** Owned copy of a string_view (avoids storing borrowed views long-term). */
func copy_view(v : string_view) : string {
    var s = string()
    s.append_view(&v)
    return s
}

func sv_eq_str(s : &string, v : string_view) : bool {
    if(s.size() != v.size()) { return false }
    var i : size_t = 0
    while(i < s.size()) {
        if(s.get(i) != v.get(i)) { return false }
        i += 1
    }
    return true
}

/** The default reporter: streams each test live, then prints totals. */
public struct ConsoleReporter {
    var display : TestDisplayConfig
    var runner_name : string
    var cur_pass : size_t
    var cur_fail : size_t
    var cur_group : string
    var first_group : bool
    var names : vector<string>
    var totals : vector<size_t>
    var passes : vector<size_t>
    var fails : vector<size_t>
    var grand_total : size_t
    var grand_pass : size_t
    var grand_fail : size_t
}

/** Create a console reporter with default display configuration. */
public func new_console_reporter() : ConsoleReporter {
    return ConsoleReporter {
        display : TestDisplayConfig(),
        runner_name : string(),
        cur_pass : 0,
        cur_fail : 0,
        cur_group : string(),
        first_group : true,
        names : vector<string>(),
        totals : vector<size_t>(),
        passes : vector<size_t>(),
        fails : vector<size_t>(),
        grand_total : 0,
        grand_pass : 0,
        grand_fail : 0
    }
}

impl TestReporter for ConsoleReporter {

    func configure(&mut self, display_ : *TestDisplayConfig) {
        self.display = *display_
    }

    func begin_runner(&mut self, name : string_view) {
        colors_enabled = enable_terminal_colors()
        self.runner_name = copy_view(name)
        self.cur_pass = 0
        self.cur_fail = 0
        self.cur_group = string()
        self.first_group = true
    }

    func on_test(&mut self, outcome : *TestOutcome) {
        if(outcome.passed) {
            self.cur_pass += 1
        } else {
            self.cur_fail += 1
        }

        // display filters never affect counting, only printing
        if(outcome.passed && self.display.failure_only) { return }
        if(!outcome.passed && self.display.successful_only) { return }

        // group header whenever the group changes
        const grp = string_view(outcome.group.data(), outcome.group.size())
        if(self.first_group || !sv_eq_str(&self.cur_group, grp)) {
            if(grp.size() == 0) {
                printf("%s%sGroup: %s(no-group)%s\n", col_bold(), col_blue(), col_gray(), col_reset())
            } else {
                printf("%s%sGroup: %s%.*s%s\n", col_bold(), col_blue(), col_gray(), grp.size() as int, grp.data(), col_reset())
            }
            self.cur_group.clear()
            self.cur_group.append_view(&grp)
            self.first_group = false
        }

        var status_color = if(outcome.passed) col_green() else col_red()
        var status_text = if(outcome.passed) "PASS" else "FAIL"

        printf("  %s- %s%.*s%s%s", status_color, col_bold(), outcome.name.size() as int, outcome.name.data(), col_reset(), col_reset())
        printf(" %d\n", outcome.id)

        if(outcome.message.size() > 0) {
            printf("     failed message parsing with message : %.*s\n", outcome.message.size() as int, outcome.message.data())
        }

        if(self.display.display_logs && outcome.logs_count > 0) {
            var li : size_t = 0
            while(li < outcome.logs_count) {
                const l = outcome.logs_ptr + li
                const lt_color = log_type_color(l.type)
                const icon = log_type_icon(l.type)
                const lname = log_type_name(l.type)
                if (l.line > 0 || l.character > 0) {
                    printf("     %s%s%s %s%s (line %zu:%zu)%s\n",
                           lt_color, icon, col_reset(),
                           lt_color, lname,
                           l.line, l.character,
                           col_reset());
                } else {
                    printf("     %s%s%s %s%s%s\n",
                           lt_color, icon, col_reset(),
                           lt_color, lname, col_reset());
                }
                if (l.message.size() > 0) {
                    print_log_multiline(get_stdout(), l.message.data())
                }
                li += 1
            }
        }

        printf("    ");
        if(outcome.exit_code != 0) {
            printf("[%s%d%s] ", status_color, outcome.exit_code, col_reset());
        }
        printf("%s%s%s\n", status_color, status_text, col_reset());
        printf("\n");
    }

    func end_runner(&mut self) {
        self.names.push(self.runner_name.copy())
        const t = self.cur_pass + self.cur_fail
        self.totals.push(t)
        self.passes.push(self.cur_pass)
        self.fails.push(self.cur_fail)
        self.grand_total += t
        self.grand_pass += self.cur_pass
        self.grand_fail += self.cur_fail
    }

    func finish(&mut self) : int {
        // Nothing was reported: stay silent. This matches the historical
        // behaviour of the test runner when a module registers no tests (e.g.
        // the sequential `--tcc` suite, which uses inline `test()` calls only).
        if(self.grand_total == 0) {
            return 0
        }

        colors_enabled = enable_terminal_colors()

        var failed_col : *char
        if(self.grand_fail) {
            failed_col = col_red()
        } else {
            failed_col = col_gray()
        }

        printf("%s%sTest run summary%s\n", col_bold(), col_cyan(), col_reset());
        printf("  Total: %zu | %sPassed: %s%zu%s | %sFailed: %s%zu%s\n\n",
               self.grand_total,
               col_green(), col_green(), self.grand_pass, col_reset(),
               failed_col, col_red(), self.grand_fail, col_reset());

        if(self.names.size() > 1) {
            var i : size_t = 0
            while(i < self.names.size()) {
                const nm = self.names.get_ptr(i)
                const tp = self.totals.get_ptr(i)
                const pp = self.passes.get_ptr(i)
                const fp = self.fails.get_ptr(i)
                var rcol : *char
                if(*fp) {
                    rcol = col_red()
                } else {
                    rcol = col_green()
                }
                printf("  %s%-18s%s : %zu tests - %s%zu passed%s, %s%zu failed%s\n",
                       col_bold(), nm.data(), col_reset(),
                       *tp,
                       col_green(), *pp, col_reset(),
                       rcol, *fp, col_reset())
                i += 1
            }
            printf("\n")
        }

        printf("%sSummary: %zu tests - %s%zu passed%s, %s%zu failed%s\n\n",
               col_bold(), self.grand_total,
               col_green(), self.grand_pass, col_reset(),
               failed_col, self.grand_fail, col_reset());

        if(self.grand_fail > 0) {
            return 1
        }
        return 0
    }

}

/** Convenience wrapper so callers can finish a reporter without an interface cast. */
public func finish_reporter(reporter : &mut TestReporter) : int {
    return reporter.finish()
}

/* ------------------------------------------------------------------ */
/*  Runner handles and the multi-runner                                */
/* ------------------------------------------------------------------ */

/**
 * A runnable test runner.
 *
 * `tests_ptr` / `tests_count` carry the runner's discovered test array (the
 * exact element type is owned by the runner library). `run_fn` receives the
 * handle so it can recover that array and stream outcomes to the reporter.
 */
public struct TestRunnerHandle {
    public var name : string_view
    public var tests_ptr : *void
    public var tests_count : size_t
    public var run_fn : (h : *mut TestRunnerHandle, reporter : &mut TestReporter, argc : int, argv : **char) => int
}

/** Does `argv` contain `flag` (searched from index 1, like main)? */
public func argv_has_flag(argv : **char, argc : int, flag : *char) : bool {
    var i : int = 1
    while(i < argc) {
        if(strcmp(argv[i], flag) == 0) { return true }
        i += 1
    }
    return false
}

/** Compare a runner name against a C string without allocating. */
func runner_name_is(name : string_view, expected : *char) : bool {
    const elen = strlen(expected)
    if(name.size() != elen) { return false }
    var i : size_t = 0
    while(i < elen) {
        if(name.get(i) != expected[i]) { return false }
        i += 1
    }
    return true
}

/**
 * Runs several runners in order under a single reporter and prints one
 * combined summary at the end.
 *
 * When the process is a spawned test-library child (`--comm-id` present) only
 * the `test` runner executes and nothing is printed, matching the historical
 * child-process behaviour.
 */
public struct MultiTestRunner {
    var runners : vector<TestRunnerHandle>
}

public func new_multi_test_runner() : MultiTestRunner {
    return MultiTestRunner {
        runners : vector<TestRunnerHandle>()
    }
}

public func (runner : &mut MultiTestRunner) add(handle : TestRunnerHandle) {
    runner.runners.push(handle)
}

public func (runner : &mut MultiTestRunner) run(argc : int, argv : **char) : int {
    var console = new_console_reporter()
    const is_child = argv_has_flag(argv, argc, "--comm-id")

    var i : size_t = 0
    while(i < runner.runners.size()) {
        const h = runner.runners.get_ptr(i)
        if(is_child && !runner_name_is(h.name, "test")) {
            i += 1
            continue
        }
        if(!is_child) {
            printf("\n%s%s==> runner: %.*s%s\n", col_bold(), col_cyan(), h.name.size() as int, h.name.data(), col_reset())
        }
        h.run_fn(h, &mut console, argc, argv)
        i += 1
    }

    if(is_child) {
        return 0
    }
    return finish_reporter(&mut console)
}
