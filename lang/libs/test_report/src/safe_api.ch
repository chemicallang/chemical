// test_report — additive safe facade over the raw reporting structs.
//
// The raw `TestOutcome` deliberately carries borrowed views plus a raw
// `logs_ptr` so a runner can stream a test without allocating. This file adds
// checked accessors (`Option`, `&T`), `&str`-facing reporter methods, and an
// owned snapshot (`TestOutcomeSnapshot`) for callers that want to keep a result
// past the synchronous `on_test` call.
//
// Nothing here changes or replaces the raw surface.

public namespace test_report {

using std::Option;
using std::string;
using std::str;
using std::vector;

// -------------------------------------------------------------------------
// Invalid-UTF-8-aware string views
// -------------------------------------------------------------------------

/**
 * A borrowed UTF-8 view of an outcome field.
 *
 * Returns `None` when the field is empty or not valid UTF-8, so callers never
 * observe a `str` with an unvalidated backing buffer.
 */
public func name(outcome : &TestOutcome) : Option<str> {
    return utf8_view(outcome.test_name())
}

public func group(outcome : &TestOutcome) : Option<str> {
    return utf8_view(outcome.group_name())
}

public func runner(outcome : &TestOutcome) : Option<str> {
    return utf8_view(outcome.runner_name())
}

public func message(outcome : &TestOutcome) : Option<str> {
    return utf8_view(outcome.failure_message())
}

public func log_message(log : &TestLog) : Option<str> {
    return utf8_view(log.message.to_view())
}

func utf8_view(view : std::string_view) : Option<str> {
    if(view.empty()) {
        return Option.None<str>()
    }
    var made = std::try_from_utf8(view)
    if(made is std::Result.Err) {
        return Option.None<str>()
    }
    var Ok(value) = made else unsafe { unreachable }
    return Option.Some<str>(value)
}

// -------------------------------------------------------------------------
// Checked log access
// -------------------------------------------------------------------------

/// Number of structured logs attached to an outcome.
public func log_count(outcome : &TestOutcome) : size_t {
    return outcome.log_count()
}

/// Checked access to one structured log entry.
public func log_at(outcome : &TestOutcome, index : size_t) : Option<&TestLog> {
    return outcome.log_at(index)
}

// -------------------------------------------------------------------------
// Owned snapshot
// -------------------------------------------------------------------------

/** Owned copy of one structured log line. */
public struct TestLogSnapshot {
    public var type : LogType
    public var message : string
    public var line : ubigint
    public var character : ubigint
}

/**
 * Owned snapshot of a finished test.
 *
 * Safe to keep after `TestReporter::on_test` returns; it owns every string and
 * the log list and exposes no raw pointer.
 */
public struct TestOutcomeSnapshot {
    public var runner : string
    public var name : string
    public var group : string
    public var message : string
    public var id : int
    public var passed : bool
    public var skipped : bool
    public var exit_code : int
    public var duration_ms : ubigint
    public var logs : vector<TestLogSnapshot>
}

func copy_view(view : std::string_view) : string {
    var out = string()
    if(!view.empty()) {
        out.append_view(&view)
    }
    return out
}

/// Copy an outcome (and every attached log) into owned storage.
public func snapshot(outcome : &TestOutcome) : TestOutcomeSnapshot {
    var logs = vector<TestLogSnapshot>()
    const count = log_count(outcome)
    var i : size_t = 0
    while(i < count) {
        var entry = log_at(outcome, i)
        if(entry is Option.Some) {
            var Some(log) = entry else unsafe { unreachable }
            logs.push(TestLogSnapshot {
                type : log.type,
                message : copy_view(log.message.to_view()),
                line : log.line,
                character : log.character
            })
        }
        i += 1
    }

    return TestOutcomeSnapshot {
        runner : copy_view(outcome.runner_name()),
        name : copy_view(outcome.test_name()),
        group : copy_view(outcome.group_name()),
        message : copy_view(outcome.failure_message()),
        id : outcome.test_id(),
        passed : outcome.did_pass(),
        skipped : outcome.was_skipped(),
        exit_code : outcome.process_exit_code(),
        duration_ms : outcome.duration(),
        logs : logs
    }
}

// -------------------------------------------------------------------------
// Reporter helpers
// -------------------------------------------------------------------------

/// Configure a reporter from an owned display config (no out-pointer).
public func configure(reporter : &mut TestReporter, display : TestDisplayConfig) {
    reporter.configure(display)
}

/// Start a runner from a `&str` name.
public func begin_runner(reporter : &mut TestReporter, name : &str) {
    reporter.begin_runner(name.to_view())
}

/// Stream one outcome by reference (no raw pointer at the call site).
public func on_test(reporter : &mut TestReporter, outcome : &TestOutcome) {
    reporter.on_test(outcome)
}

/// Finish the reporter and return its process-status result.
public func finish(reporter : &mut TestReporter) : int {
    return reporter.finish()
}

// -------------------------------------------------------------------------
// Method-call convenience aliases (additive; raw fields remain public).
// -------------------------------------------------------------------------

/// Owned, raw-pointer-free snapshot of this outcome.
public func (outcome : &TestOutcome) snapshot() : TestOutcomeSnapshot {
    return snapshot(outcome)
}

/// Configure a reporter from an owned display config.
public func (reporter : &mut TestReporter) configure_owned(display : TestDisplayConfig) {
    configure(reporter, display)
}

/// Start a runner from a `&str` name.
public func (reporter : &mut TestReporter) begin_runner_str(name : &str) {
    begin_runner(reporter, name)
}

/// Stream one outcome by reference.
public func (reporter : &mut TestReporter) on_test_ref(outcome : &TestOutcome) {
    on_test(reporter, outcome)
}

public namespace ffi {

using std::string_view;
using std::vector;

public struct TestRunnerHandle {
    internal var name : string_view
    internal var tests_ptr : *void
    internal var tests_count : size_t
    internal var run_fn : (h : *mut TestRunnerHandle, reporter : &mut TestReporter, argc : int, argv : **char) => int
}

/** Runner type-erased test array pointer (raw runner-hook access). */
public func (h : &TestRunnerHandle) tests_ptr_value() : *void {
    return h.tests_ptr
}

/** Number of type-erased tests in the runner handle. */
public func (h : &TestRunnerHandle) tests_count_value() : size_t {
    return h.tests_count
}

/** Build a runner handle without exposing its raw fields. */
public func make_runner_handle(name : string_view, tests_ptr : *void, tests_count : size_t, run_fn : (h : *mut TestRunnerHandle, reporter : &mut TestReporter, argc : int, argv : **char) => int) : TestRunnerHandle {
    return TestRunnerHandle {
        name : name,
        tests_ptr : tests_ptr,
        tests_count : tests_count,
        run_fn : run_fn
    }
}

/** Does `argv` contain `flag` (searched from index 1, like main)? */
public func argv_has_flag(argv : **char, argc : int, flag : *char) : bool {
        unsafe {
        var i : int = 1
        while(i < argc) {
            if(unsafe(strcmp(argv[i], flag)) == 0) { return true }
            i += 1
        }
        return false

        }
    }

/** Safe `&str` spelling of `argv_has_flag`. */
public func argv_has_flag_str(argv : **char, argc : int, flag : &str) : bool {
    if(argv == null || argc <= 0) { return false }
    var made = std::cstring_make(flag)
    if(made is std::Result.Err) { return false }
    var Ok(c) = made else unsafe { unreachable }
    return argv_has_flag(argv, argc, unsafe(c.as_ptr()))
}

/** Compare a runner name against a C string without allocating. */
func runner_name_is(name : string_view, expected : *char) : bool {
        unsafe {
        const elen = unsafe(strlen(expected))
        if(name.size() != elen) { return false }
        var i : size_t = 0
        while(i < elen) {
            if(name.at(i) != expected[i]) { return false }
            i += 1
        }
        return true

        }
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
        unsafe {
        return MultiTestRunner {
            runners : vector<TestRunnerHandle>()
        }

        }
    }

public func (runner : &mut MultiTestRunner) add(handle : TestRunnerHandle) {
        unsafe {
        runner.runners.push(handle)

        }
    }

public func (runner : &mut MultiTestRunner) run(argc : int, argv : **char) : int {
        unsafe {
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
                unsafe { printf("\n%s%s==> runner: %.*s%s\n", col_bold(), col_cyan(), h.name.size() as int, h.name.data(), col_reset()) }
            }
            h.run_fn(h, &mut console, argc, argv)
            i += 1
        }

        if(is_child) {
            return 0
        }
        return finish_reporter(&mut console)

        }
    }

} // namespace ffi

} // namespace test_report
