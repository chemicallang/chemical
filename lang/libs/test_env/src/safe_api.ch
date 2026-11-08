// test_env::safe — additive safe facade over the raw `TestEnv` CBI.
//
// The existing `TestEnv` interface and its `*char` extension methods stay
// available unchanged. This file adds:
//
//   * `test_env::safe::TestEnvHandle<'a>` — a borrowing handle with no public
//     raw pointers and `&str` message methods.
//   * `*_str` extension methods on `&mut TestEnv` for one-line migration.
//
// Messages are NUL-terminated internally because the underlying CBI carries a
// `*char`; a message containing an interior NUL byte is reported with a fixed
// fallback instead of truncating silently.

public namespace test_env {

public namespace safe {

/**
 * A safe, borrowing view of the test environment.
 *
 * The handle borrows the runner-owned `TestEnv`, so it cannot outlive the
 * current test invocation. Every method takes `&str` and returns owned data
 * (or simple copies); no raw pointer is exposed.
 */
public struct TestEnvHandle<'a> {
    internal var env : &'a mut TestEnv

    /// The current test's numeric id (negative when unknown).
    public func test_id(&self) : int {
        return self.env.get_test_id()
    }

    /// The current test's group name (empty when ungrouped).
    public func group_name(&self) : std::string_view {
        return self.env.get_group_name()
    }

    public func info(&self, message : &str) {
        log_message(self.env, LogType.Information, message, 0, 0)
    }

    public func warning(&self, message : &str) {
        log_message(self.env, LogType.Warning, message, 0, 0)
    }

    public func warn(&self, message : &str) {
        log_message(self.env, LogType.Warning, message, 0, 0)
    }

    public func success(&self, message : &str) {
        log_message(self.env, LogType.Success, message, 0, 0)
    }

    public func error(&self, message : &str) {
        log_message(self.env, LogType.Error, message, 0, 0)
    }

    public func panic(&self, message : &str) {
        log_message(self.env, LogType.Panic, message, 0, 0)
    }

    /// Log with an explicit source line/character (0 when unknown).
    public func log_at(&self, type : LogType, message : &str, line : uint, character : uint) {
        log_message(self.env, type, message, line, character)
    }

    /// Stop the current test group; the runner keeps other groups running.
    public func quit_current_group(&self, reason : &str) {
        var made = std::cstring_make(reason)
        if(made is std::Result.Err) { self.env.quit_current_group("<reason contained an interior NUL byte>"); return }
        var Ok(owned) = made else unsafe { unreachable }
        self.env.quit_current_group(unsafe(owned.as_ptr()))
    }

    /// Stop every running test.
    public func quit_all_tests(&self, reason : &str) {
        var made = std::cstring_make(reason)
        if(made is std::Result.Err) { self.env.quit_all_tests("<reason contained an interior NUL byte>"); return }
        var Ok(owned) = made else unsafe { unreachable }
        self.env.quit_all_tests(unsafe(owned.as_ptr()))
    }
}

/// Log one `&str` message through the raw C-string CBI.
///
/// The owned C string lives only for the duration of the call; `logIt` copies
/// the payload before returning.
@retained
public func log_message(env : &TestEnv, type : LogType, message : &str, line : uint, character : uint) {
    var made = std::cstring_make(message)
    if(made is std::Result.Err) {
        env.logIt(type, "<test message contained an interior NUL byte>", line, character)
        return
    }
    var Ok(owned) = made else unsafe { unreachable }
    env.logIt(type, unsafe(owned.as_ptr()), line, character)
}


/// Borrow a test environment as a safe handle.
public func <'a> handle(env : &'a mut TestEnv) : TestEnvHandle<'a> {
    return TestEnvHandle<'a> { env : env }
}

} // namespace safe

// -------------------------------------------------------------------------
// Method-call convenience aliases (additive; raw `info`/`error`/... remain).
// -------------------------------------------------------------------------

public func (env : &mut TestEnv) info_str(message : &str) {
    safe::handle(env).info(message)
}

public func (env : &mut TestEnv) warning_str(message : &str) {
    safe::handle(env).warning(message)
}

public func (env : &mut TestEnv) warn_str(message : &str) {
    safe::handle(env).warn(message)
}

public func (env : &mut TestEnv) success_str(message : &str) {
    safe::handle(env).success(message)
}

public func (env : &mut TestEnv) error_str(message : &str) {
    safe::handle(env).error(message)
}

public func (env : &mut TestEnv) panic_str(message : &str) {
    safe::handle(env).panic(message)
}

public func (env : &mut TestEnv) log_str(type : LogType, message : &str) {
    safe::handle(env).log_at(type, message, 0, 0)
}

public func (env : &mut TestEnv) quit_current_group_str(reason : &str) {
    safe::handle(env).quit_current_group(reason)
}

public func (env : &mut TestEnv) quit_all_tests_str(reason : &str) {
    safe::handle(env).quit_all_tests(reason)
}

/// Borrow this environment as a safe handle: `env.safe_handle().error("...")`.
public func <'a> (env : &'a mut TestEnv) safe_handle() : safe::TestEnvHandle<'a> {
    return safe::handle(env)
}

} // namespace test_env
