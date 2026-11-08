using std::string;
using std::string_view;
using std::vector;
using std::Result;

@test
public func test_process_execute(env : &mut TestEnv) {
    var cfg = process::Command.new("echo")
    cfg.arg("hello world")
    var res = cfg.output()
    if(res is Result.Err) {
        env.error("process::execute returned Err")
        return
    }
    var Ok(r) = res else unsafe { unreachable }
    if(!r.success()) {
        env.error("process::execute did not succeed")
        return
    }
    if(!stdout_contains(r.stdout_bytes(), "hello world")) {
        env.error("process stdout did not contain expected text")
        return
    }
}

@test
public func test_process_command_defaults(env : &mut TestEnv) {
    var cfg = process::Command.new("true")
    var res = cfg.status()
    if(res is Result.Err) { env.error("Command defaults should run"); return }
}

func stdout_contains(data : [u8], expected : string_view) : bool {
        unsafe {
        if(data.size() < expected.size()) { return false }
        var d = data.data()
        var e = expected.data()
        var i = 0
        while(i < expected.size()) {
            if(d[i] != e[i]) { return false }
            i += 1
        }
        return true

        }
    }
