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
@expand
public comptime func (env : &mut TestEnv) info(msg : %expressive_string) {
    var text = std::string()
    intrinsics::expr_str_block_value(std::StringStream { str : &mut text }, msg)
    env.logIt(LogType.Information, text.data(), 0, 0)
}

@expand
public comptime func (env : &mut TestEnv) warning(msg : %expressive_string) {
    var text = std::string()
    intrinsics::expr_str_block_value(std::StringStream { str : &mut text }, msg)
    env.logIt(LogType.Warning, text.data(), 0, 0)
}

@expand
public comptime func (env : &mut TestEnv) warn(msg : %expressive_string) {
    var text = std::string()
    intrinsics::expr_str_block_value(std::StringStream { str : &mut text }, msg)
    env.logIt(LogType.Warning, text.data(), 0, 0)
}

@expand
public comptime func (env : &mut TestEnv) success(msg : %expressive_string) {
    var text = std::string()
    intrinsics::expr_str_block_value(std::StringStream { str : &mut text }, msg)
    env.logIt(LogType.Success, text.data(), 0, 0)
}

@expand
public comptime func (env : &mut TestEnv) error(msg : %expressive_string) {
    var text = std::string()
    intrinsics::expr_str_block_value(std::StringStream { str : &mut text }, msg)
    env.logIt(LogType.Error, text.data(), 0, 0)
}

@expand
public comptime func (env : &mut TestEnv) panic(msg : %expressive_string) {
    var text = std::string()
    intrinsics::expr_str_block_value(std::StringStream { str : &mut text }, msg)
    env.logIt(LogType.Panic, text.data(), 0, 0)
}
