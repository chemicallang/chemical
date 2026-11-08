// test_report::safe — additive safe facade over the raw reporting structs.
//
// The raw `TestOutcome` deliberately carries borrowed views plus a raw
// `logs_ptr` so a runner can stream a test without allocating. This file adds
// checked accessors (`Option`, `&T`), `&str`-facing reporter methods, and an
// owned snapshot (`SafeTestOutcome`) for callers that want to keep a result
// past the synchronous `on_test` call.
//
// Nothing here changes or replaces the raw surface.

public namespace test_report {

public namespace safe {

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
    return utf8_view(outcome.name)
}

public func group(outcome : &TestOutcome) : Option<str> {
    return utf8_view(outcome.group)
}

public func runner(outcome : &TestOutcome) : Option<str> {
    return utf8_view(outcome.runner)
}

public func message(outcome : &TestOutcome) : Option<str> {
    return utf8_view(outcome.message)
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
    if(outcome.logs_ptr == null) {
        return 0
    }
    return outcome.logs_count
}

/// Checked access to one structured log entry.
public func log_at(outcome : &TestOutcome, index : size_t) : Option<&TestLog> {
    if(outcome.logs_ptr == null || index >= outcome.logs_count) {
        return Option.None<&TestLog>()
    }
    const entry = unsafe(outcome.logs_ptr + index)
    return Option.Some<&TestLog>(unsafe(&*entry))
}

// -------------------------------------------------------------------------
// Owned snapshot
// -------------------------------------------------------------------------

/** Owned copy of one structured log line. */
public struct SafeTestLog {
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
public struct SafeTestOutcome {
    public var runner : string
    public var name : string
    public var group : string
    public var message : string
    public var id : int
    public var passed : bool
    public var skipped : bool
    public var exit_code : int
    public var duration_ms : ubigint
    public var logs : vector<SafeTestLog>
}

func copy_view(view : std::string_view) : string {
    var out = string()
    if(!view.empty()) {
        out.append_view(&view)
    }
    return out
}

/// Copy an outcome (and every attached log) into owned storage.
public func snapshot(outcome : &TestOutcome) : SafeTestOutcome {
    var logs = vector<SafeTestLog>()
    const count = log_count(outcome)
    var i : size_t = 0
    while(i < count) {
        var entry = log_at(outcome, i)
        if(entry is Option.Some) {
            var Some(log) = entry else unsafe { unreachable }
            logs.push(SafeTestLog {
                type : log.type,
                message : copy_view(log.message.to_view()),
                line : log.line,
                character : log.character
            })
        }
        i += 1
    }

    return SafeTestOutcome {
        runner : copy_view(outcome.runner),
        name : copy_view(outcome.name),
        group : copy_view(outcome.group),
        message : copy_view(outcome.message),
        id : outcome.id,
        passed : outcome.passed,
        skipped : outcome.skipped,
        exit_code : outcome.exit_code,
        duration_ms : outcome.duration_ms,
        logs : logs
    }
}

// -------------------------------------------------------------------------
// Reporter helpers
// -------------------------------------------------------------------------

/// Configure a reporter from an owned display config (no out-pointer).
public func configure(reporter : &mut TestReporter, display : TestDisplayConfig) {
    reporter.configure(&raw display)
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

} // namespace safe

// -------------------------------------------------------------------------
// Method-call convenience aliases (additive; raw fields remain public).
// -------------------------------------------------------------------------

/// Owned, raw-pointer-free snapshot of this outcome.
public func (outcome : &TestOutcome) snapshot() : safe::SafeTestOutcome {
    return safe::snapshot(outcome)
}

/// Checked log entry access.
public func (outcome : &TestOutcome) log_at(index : size_t) : std::Option<&TestLog> {
    return safe::log_at(outcome, index)
}

/// Configure a reporter from an owned display config.
public func (reporter : &mut TestReporter) configure_owned(display : TestDisplayConfig) {
    safe::configure(reporter, display)
}

/// Start a runner from a `&str` name.
public func (reporter : &mut TestReporter) begin_runner_str(name : &str) {
    safe::begin_runner(reporter, name)
}

/// Stream one outcome by reference.
public func (reporter : &mut TestReporter) on_test_ref(outcome : &TestOutcome) {
    safe::on_test(reporter, outcome)
}

} // namespace test_report
