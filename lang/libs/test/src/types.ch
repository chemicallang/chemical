public struct TestFunction {
    var id : int
    var name : std::string_view
    var group : std::string_view
    var ptr : (env : &mut TestEnv) => void
    var file : std::string_view
    var timeout : uint
    var retry : uint
    var pass_on_crash : bool
    var returns_bool : bool
    var benchmark : bool
    var lineNum : uint
    var charNum : uint
}

struct TestEnvImpl : TestEnv {

    var fn : *mut TestFunction

    if(def.windows) {
        var pipeHandle : HANDLE
    } else {
        var fd : int
    }

    @override
    func get_test_id(&self) : int {
        return fn.id;
    }

    @override
    func get_group_name(&self) : std::string_view {
        return fn.group
    }

    @override
    func logIt(&mut self, type : LogType, msgData : *char, lineNum : uint, charNum : uint) {
        var msg = std::string();
        msg.append_char_ptr("$log,")
        var buff : [2048]char
        snprintf(&mut buff[0], sizeof(buff), "%d,%d,%d,%s", type as int, lineNum, charNum, msgData);
        msg.append_char_ptr(&buff[0])
        self.send_message(msg.data(), msg.size())
    }

    @override
    func quit_current_group(&mut self, reason : *char) {
        var msg = std::string();
        msg.append_char_ptr("$quit_group:");
        msg.append_char_ptr(reason)
        self.send_message(msg.data(), msg.size())
    }

    @override
    func quit_all_tests(&mut self, reason : *char) {
        var msg = std::string();
        msg.append_char_ptr("$quit_all:");
        msg.append_char_ptr(reason)
        self.send_message(msg.data(), msg.size())
    }

}

struct TestDisplayConfig {
    /**
     * display successful tests only
     */
    var successful_only : bool = false;
    /**
     * display failed tests only
     */
    var failure_only : bool = false;
    /**
     * when false, no logs will be displayed
     */
    var display_logs : bool = true;
}

struct TestRunnerConfig {
    /**
     * display configuration
     */
    var display : TestDisplayConfig
    /**
     * should benchmark all the tests
     */
    var benchmark : bool = false
    /**
     * when -1, means user didn't give --test-id command line argument
     * this contains the parsed id
     */
    var single_test_id : int = -1
    /**
     * the communication id is used for ipc communication between processes
     * it may or may not be required
     */
    var comm_id : int = -1;
    /**
     * limit total processes to it
     */
    var process_limit : int = 6;
    /**
     * when user asks to launch only specific groups
     */
    var groups_to_launch : std::vector<*char>
    /**
     * the testing function that should be executed before each test
     */
    var before_each : (env : &mut TestEnv) => void = null
    /**
     * the testing function that should be executed after each test
     */
    var after_each : (env : &mut TestEnv) => void = null
}

struct TestLog {

    var type : LogType = LogType.Success

    var message : std::string

    var line : ubigint = 0

    var character : ubigint = 0

}

struct TestFunctionState {

    var fn : *mut TestFunction

    if(def.windows) {
        var exitCode : DWORD = 0
    } else {
        var exitCode : int = 0
    }

    var has_error_log : bool = false

    var has_failed : bool = false

    var logs : std::vector<TestLog>

    @make
    func make(t_fn : *mut TestFunction) {
        return TestFunctionState {
            fn : t_fn
        }
    }

}

struct TestRunnerState {

    var tests : std::vector<TestFunctionState>

}
