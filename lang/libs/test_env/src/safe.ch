// Safe message helpers for TestEnv. The raw interface still takes *char for
// CBI compatibility; these wrappers own a validated C string for the call.
public namespace test_env_safe {

public namespace safe {

func log(env : &mut TestEnv, kind : LogType, msg : &str) {
    var made = std::cstring_make(msg)
    if(made is std::Result.Err) { return }
    var Ok(c) = made else unsafe { unreachable }
    env.logIt(kind, unsafe(c.as_ptr()), 0, 0)
}

public func info(env : &mut TestEnv, msg : &str) { log(env, LogType.Information, msg) }
public func warning(env : &mut TestEnv, msg : &str) { log(env, LogType.Warning, msg) }
public func success(env : &mut TestEnv, msg : &str) { log(env, LogType.Success, msg) }
public func error(env : &mut TestEnv, msg : &str) { log(env, LogType.Error, msg) }
public func panic_msg(env : &mut TestEnv, msg : &str) { log(env, LogType.Panic, msg) }

public func quit_group(env : &mut TestEnv, reason : &str) {
    var made = std::cstring_make(reason)
    if(made is std::Result.Err) { return }
    var Ok(c) = made else unsafe { unreachable }
    env.quit_current_group(unsafe(c.as_ptr()))
}

public func quit_all(env : &mut TestEnv, reason : &str) {
    var made = std::cstring_make(reason)
    if(made is std::Result.Err) { return }
    var Ok(c) = made else unsafe { unreachable }
    env.quit_all_tests(unsafe(c.as_ptr()))
}

} // namespace safe
} // namespace test_env_safe
