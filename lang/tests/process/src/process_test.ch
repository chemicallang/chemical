using std::string;
using std::string_view;
using std::vector;
using std::Result;

@test
public func test_process_execute(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("echo"))
    cfg.args.push(string("hello world"))
    var res = process::execute(cfg)
    if(res is Result.Err) {
        env.error("process::execute returned Err")
        return
    }
    var Ok(r) = res else unreachable
    if(!r.success) {
        env.error("process::execute did not succeed")
        return
    }
    if(!stdout_contains(&raw r.output.stdout_data, "hello world")) {
        env.error("process stdout did not contain expected text")
        return
    }
}

@test
public func test_process_config_defaults(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    if(cfg.args.size() != 0) { env.error("default args should be empty"); return }
    if(!cfg.capture_stdout) { env.error("default capture_stdout should be true"); return }
    if(!cfg.capture_stderr) { env.error("default capture_stderr should be true"); return }
    if(cfg.merge_stdout_stderr) { env.error("default merge_stdout_stderr should be false"); return }
}

func stdout_contains(data : *vector<u8>, expected : string_view) : bool {
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
