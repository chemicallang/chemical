// process — cross-platform process management.

public namespace process {

using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;

public struct ProcessOutput {
    var stdout_data : vector<u8>;
    var stderr_data : vector<u8>;
}

public struct ExitStatus {
    var code : int;
    var signaled : bool;
    var signal : int;
}

public struct ProcessResult {
    var output : ProcessOutput;
    var status : ExitStatus;
    var success : bool;
}

public struct ProcessConfig {
    var args : vector<string>;
    var env : vector<string>;
    var working_dir : string;
    var capture_stdout : bool;
    var capture_stderr : bool;
    var merge_stdout_stderr : bool;
    var stdin_data : vector<u8>;

    func default() : ProcessConfig {
        return ProcessConfig {
            args = vector<string>(),
            env = vector<string>(),
            working_dir = string(),
            capture_stdout = true,
            capture_stderr = true,
            merge_stdout_stderr = false,
            stdin_data = vector<u8>()
        }
    }
}

public struct ChildProcess {
    if(def.windows) {
        struct {
            var h_process : *mut void;
            var h_thread : *mut void;
            var h_stdout_read : *mut void;
            var h_stderr_read : *mut void;
            var h_stdin_write : *mut void;
        } win;
    } else {
        struct {
            var pid : int;
            var stdout_fd : int;
            var stderr_fd : int;
            var stdin_fd : int;
        } _unix;
    }
    var is_running : bool;
    var captured_stdout : vector<u8>;
    var captured_stderr : vector<u8>;
}

public type PR_Result = Result<ProcessResult, ProcessError>
public type CP_Result = Result<ChildProcess, ProcessError>
public type UT_Result = Result<UnitTy, ProcessError>



func pr_ok(v : &mut ProcessResult, ret : &mut PR_Result) {
    std::replace<PR_Result>(ret, Result.Ok<ProcessResult, ProcessError>(std::replace<ProcessResult>(v, zeroed:unsafe<ProcessResult>())))
}
func pr_err(e : ProcessError, ret : &mut PR_Result) {
    std::replace<PR_Result>(ret, Result.Err<ProcessResult, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
}

public func execute(cfg : ProcessConfig) : PR_Result {
    var ret = zeroed:unsafe<PR_Result>()
    if(cfg.args.size() == 0) {
        var e = ProcessError.InvalidArgs(string("no arguments provided"))
        pr_err(e, &mut ret)
        return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
    } else {}
    comptime if(def.windows) {
        var e = ProcessError.OperationFailed(string("not implemented"))
        pr_err(e, &mut ret)
        return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
    } else {
        var result = zeroed:unsafe<ProcessResult>()
        if(posix_execute(&raw mut cfg, &raw mut result)) {
            pr_ok(&mut result, &mut ret)
            return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
        } else {
            var e = ProcessError.OperationFailed(string("exec failed"))
            pr_err(e, &mut ret)
            return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
        }
    }
}

public func spawn(cfg : ProcessConfig) : CP_Result {
    var ret = zeroed:unsafe<CP_Result>()
    if(cfg.args.size() == 0) {
        var e = ProcessError.InvalidArgs(string("no arguments provided"))
        std::replace<CP_Result>(&mut ret, Result.Err<ChildProcess, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
        return std::replace<CP_Result>(&mut ret, zeroed:unsafe<CP_Result>())
    } else {}
    comptime if(def.windows) {
        var e = ProcessError.OperationFailed(string("not implemented"))
        std::replace<CP_Result>(&mut ret, Result.Err<ChildProcess, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
        return std::replace<CP_Result>(&mut ret, zeroed:unsafe<CP_Result>())
    } else {
        var child : ChildProcess
        if(posix_spawn(&raw mut cfg, &raw mut child)) {
            std::replace(&mut ret, Result.Ok<ChildProcess, ProcessError>(std::replace(&mut child, zeroed:unsafe<ChildProcess>())))
            return std::replace<CP_Result>(&mut ret, zeroed:unsafe<CP_Result>())
        } else {
            var e = ProcessError.OperationFailed(string("spawn failed"))
            std::replace<CP_Result>(&mut ret, Result.Err<ChildProcess, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
            return std::replace<CP_Result>(&mut ret, zeroed:unsafe<CP_Result>())
        }
    }
}

public func wait(child : *mut ChildProcess) : PR_Result {
    var ret = zeroed:unsafe<PR_Result>()
    if(!child.is_running) {
        var e = ProcessError.NotRunning()
        pr_err(e, &mut ret)
        return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
    } else {}
    comptime if(def.windows) {
        var e = ProcessError.OperationFailed(string("not implemented"))
        pr_err(e, &mut ret)
        return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
    } else {
        var result = zeroed:unsafe<ProcessResult>()
        if(posix_wait(child, &raw mut result)) {
            pr_ok(&mut result, &mut ret)
            return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
        } else {
            var e = ProcessError.OperationFailed(string("wait failed"))
            pr_err(e, &mut ret)
            return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
        }
    }
}

public func kill(child : *mut ChildProcess, signal : int) : UT_Result {
    var ret = zeroed:unsafe<UT_Result>()
    if(!child.is_running) {
        var e = ProcessError.NotRunning()
        std::replace(&mut ret, Result.Err<UnitTy, ProcessError>(std::replace(&mut e, ProcessError.NotRunning())))
        return std::replace<UT_Result>(&mut ret, zeroed:unsafe<UT_Result>())
    } else {}
    comptime if(def.windows) {
        var r = TerminateProcess(child.win.h_process, 1u32);
        if(r == 0) {
            var e = ProcessError.OperationFailed(string("TerminateProcess failed"))
            std::replace(&mut ret, Result.Err<UnitTy, ProcessError>(std::replace(&mut e, ProcessError.NotRunning())))
            return std::replace<UT_Result>(&mut ret, zeroed:unsafe<UT_Result>())
        } else {}
        child.is_running = false;
        std::replace(&mut ret, Result.Ok<UnitTy, ProcessError>(UnitTy{}))
        return std::replace<UT_Result>(&mut ret, zeroed:unsafe<UT_Result>())
    } else {
        child.is_running = false;
        std::replace(&mut ret, Result.Ok<UnitTy, ProcessError>(UnitTy{}))
        return std::replace<UT_Result>(&mut ret, zeroed:unsafe<UT_Result>())
    }
}

} // end namespace process
