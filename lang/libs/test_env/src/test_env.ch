public enum LogType {
    Information,
    Warning,
    Success,
    Error,
    Panic,
    UnknownFailure,
    ConfigFailure,
    IOFailure,
    NetworkFailure,
    MemoryFailure,
    RuntimeFailure,
    OutOfBoundFailure,
    ResourceFailure,
    TodoFailure,
    SecurityFailure,
    WTFFailure
}

/**
 * this interface is exposed to client testing environment
 * tests use this interface to send messages to the parent executable
 */
@static
public interface TestEnv {

    /**
     * get the current test id
     * less than zero when not known due to a failure
     */
    func get_test_id(&self) : int

    /**
     * get the group name this test belongs to
     * empty when belongs to no group
     */
    func get_group_name(&self) : std::string_view;

    /**
     * this function logs it
     */
    func logIt(&self, type : LogType, msg : *char, lineNum : uint, charNum : uint)

    /**
     * quits the current group of tests
     */
    func quit_current_group(&self, reason : *char);

    /**
     * quit all the tests running
     */
    func quit_all_tests(&self, reason : *char)

}

// Canonical message methods take `&str`; the raw `*char` CBI entry point is
// `test_env::ffi::log_cstr` / `quit_*_cstr` below.
public func (env : &mut TestEnv) info(msg : &str) {
    test_env::log_message(env, LogType.Information, msg, 0, 0)
}

public func (env : &mut TestEnv) warning(msg : &str) {
    test_env::log_message(env, LogType.Warning, msg, 0, 0)
}

public func (env : &mut TestEnv) warn(msg : &str) {
    test_env::log_message(env, LogType.Warning, msg, 0, 0)
}

public func (env : &mut TestEnv) success(msg : &str) {
    test_env::log_message(env, LogType.Success, msg, 0, 0)
}

public func (env : &mut TestEnv) error(msg : &str) {
    test_env::log_message(env, LogType.Error, msg, 0, 0)
}

public func (env : &mut TestEnv) panic(msg : &str) {
    test_env::log_message(env, LogType.Panic, msg, 0, 0)
    // TODO destruct everything
}
