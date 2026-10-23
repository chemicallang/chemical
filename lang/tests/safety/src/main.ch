// Safety suite (S0.1 skeleton + runtime-failure harness).
//
// Compile-fail tests live in lang/tests/negative and run with `--negative`.
// This module hosts runtime-failure and parity tests, runs with `--safety`,
// and exercises the same compiler binary the negative suite uses.

using std::Result;
using std::string;
using std::vector;

internal var safety_work_dir_buf : [512]char
internal var SAFETY_WORK_DIR : *char = null

internal func safety_init_work_dir() {
    var r = fs::temp_dir(unsafe(&raw mut safety_work_dir_buf[0]), 512 as size_t)
    var i : size_t = 0
    if(r is Result.Ok) {
        var Ok(len) = r else unreachable
        i = len
    } else {
        var fallback = "chemical_safety_tests\0"
        while(fallback[i] != 0 && i < 511) {
            safety_work_dir_buf[i] = fallback[i]
            i++
        }
        safety_work_dir_buf[i] = 0
        SAFETY_WORK_DIR = unsafe(&raw mut safety_work_dir_buf[0])
        mkdir(SAFETY_WORK_DIR, 0o777 as uint)
        return
    }
    if(i > 0 && (safety_work_dir_buf[i - 1] == '/' || safety_work_dir_buf[i - 1] == '\\')) {
        i--
    }
    var suffix = "/chemical_safety_tests\0"
    var k = 0
    while(suffix[k] != 0 && i < 511) {
        safety_work_dir_buf[i] = suffix[k]
        i++
        k++
    }
    safety_work_dir_buf[i] = 0
    SAFETY_WORK_DIR = unsafe(&raw mut safety_work_dir_buf[0])
    mkdir(SAFETY_WORK_DIR, 0o777 as uint)
}

internal func safety_write_file(path : *char, content : *char) {
    var f = fopen(path, "w")
    if(f != null) {
        fwrite(content as *void, strlen(content), 1, f)
        fclose(f)
    }
}

internal func safety_run_command(args : vector<string>) : int {
    var cfg = process::ProcessConfig.default()
    cfg.args = args
    cfg.capture_stdout = true
    cfg.capture_stderr = true
    cfg.timeout_ms = 60000
    var r = process::execute(cfg)
    if(r is Result.Err) {
        return -1
    }
    var Ok(result) = r else unreachable
    if(result.status.signaled) {
        return 128 + result.status.signal
    }
    return result.status.code
}

internal func safety_run_command_capture(args : vector<string>, out : &mut string) : int {
    var cfg = process::ProcessConfig.default()
    cfg.args = args
    cfg.capture_stdout = true
    cfg.capture_stderr = true
    cfg.timeout_ms = 60000
    var r = process::execute(cfg)
    if(r is Result.Err) {
        return -1
    }
    var Ok(result) = r else unreachable
    var view = result.stdout_str()
    out.append_view(&view)
    if(result.status.signaled) {
        return 128 + result.status.signal
    }
    return result.status.code
}

// Compile `ch_content` into `name` and run it. The program is expected to
// terminate abnormally (panic/abort or a nonzero exit code).
internal func expect_runtime_panic(env : &mut TestEnv, name : *char, ch_content : *char) {
    var dir_path : [512]char
    var mod_path : [512]char
    var exe_path : [512]char
    var ch_path : [512]char
    sprintf(unsafe(&raw mut dir_path[0]), "%s/%s", SAFETY_WORK_DIR, name)
    mkdir(unsafe(&raw dir_path[0]), 0o777 as uint)
    sprintf(unsafe(&raw mut mod_path[0]), "%s/chemical.mod", unsafe(&raw dir_path[0]))
    sprintf(unsafe(&raw mut ch_path[0]), "%s/test.ch", unsafe(&raw dir_path[0]))
    sprintf(unsafe(&raw mut exe_path[0]), "%s/test.exe", unsafe(&raw dir_path[0]))
    safety_write_file(unsafe(&raw mut mod_path[0]), "module safety_test\nsource \".\"\nimport cstd\nimport std\nimport core\n")
    safety_write_file(unsafe(&raw mut ch_path[0]), ch_content)

    var compiler_path_buf : [1024]char
    sprintf(unsafe(&raw mut compiler_path_buf[0]), "%s", intrinsics::get_compiler_path())
    var compiler_path = string()
    var compiler_view = std::string_view(unsafe(&raw compiler_path_buf[0]), strlen(unsafe(&raw compiler_path_buf[0])))
    compiler_path.append_view(&compiler_view)
    var module_path = string()
    var module_view = std::string_view(unsafe(&raw mod_path[0]), strlen(unsafe(&raw mod_path[0])))
    module_path.append_view(&module_view)
    var executable_path = string()
    var executable_view = std::string_view(unsafe(&raw exe_path[0]), strlen(unsafe(&raw exe_path[0])))
    executable_path.append_view(&executable_view)

    var compile_args = vector<string>()
    compile_args.push(compiler_path)
    compile_args.push(module_path)
    compile_args.push(string("--no-cache"))
    compile_args.push(string("-o"))
    compile_args.push(executable_path)
    var compile_rc = safety_run_command(compile_args)
    if(compile_rc != 0) {
        env.error("expected runtime test to compile")
        return
    }

    var run_path = string()
    var run_view = std::string_view(unsafe(&raw exe_path[0]), strlen(unsafe(&raw exe_path[0])))
    run_path.append_view(&run_view)
    var run_args = vector<string>()
    run_args.push(run_path)
    var run_rc = safety_run_command(run_args)
    if(run_rc == 0) {
        env.error("expected the program to terminate abnormally")
    }
}

@test
func safety_suite_smoke(env : &mut TestEnv) {
    if(1 + 1 != 2) {
        env.error("safety suite sanity check failed")
    }
}

internal func safety_compile_with_report(env : &mut TestEnv, name : *char, ch_content : *char) : string {
    safety_init_work_dir()
    var dir_path : [512]char
    var mod_path : [512]char
    var exe_path : [512]char
    var ch_path : [512]char
    sprintf(unsafe(&raw mut dir_path[0]), "%s/%s", SAFETY_WORK_DIR, name)
    mkdir(unsafe(&raw dir_path[0]), 0o777 as uint)
    sprintf(unsafe(&raw mut mod_path[0]), "%s/chemical.mod", unsafe(&raw dir_path[0]))
    sprintf(unsafe(&raw mut ch_path[0]), "%s/test.ch", unsafe(&raw dir_path[0]))
    sprintf(unsafe(&raw mut exe_path[0]), "%s/test.exe", unsafe(&raw dir_path[0]))
    safety_write_file(unsafe(&raw mut mod_path[0]), "module safety_report_test\nsource \".\"\nimport cstd\nimport std\nimport core\n")
    safety_write_file(unsafe(&raw mut ch_path[0]), ch_content)

    var compiler_path_buf : [1024]char
    sprintf(unsafe(&raw mut compiler_path_buf[0]), "%s", intrinsics::get_compiler_path())
    var compiler_path = string()
    var compiler_view = std::string_view(unsafe(&raw compiler_path_buf[0]), strlen(unsafe(&raw compiler_path_buf[0])))
    compiler_path.append_view(&compiler_view)
    var module_path = string()
    var module_view = std::string_view(unsafe(&raw mod_path[0]), strlen(unsafe(&raw mod_path[0])))
    module_path.append_view(&module_view)
    var executable_path = string()
    var executable_view = std::string_view(unsafe(&raw exe_path[0]), strlen(unsafe(&raw exe_path[0])))
    executable_path.append_view(&executable_view)

    var args = vector<string>()
    args.push(compiler_path)
    args.push(module_path)
    args.push(string("--no-cache"))
    args.push(string("--safety-report"))
    args.push(string("-o"))
    args.push(executable_path)
    var out = string()
    var rc = safety_run_command_capture(args, &mut out)
    if(rc != 0) {
        env.error("safety report compilation failed")
    }
    return out
}

@test
func safety_report_emits_counts(env : &mut TestEnv) {
    var ch = "func read_ptr(p : *mut int) : int {\n    return *p\n}\npublic func main() : int {\n    return 0\n}\n"
    var out = safety_compile_with_report(env, "report_counts", ch)
    var view = out.to_view()
    var raw_needle = std::string_view("raw_pointer_types")
    var deref = std::string_view("raw_pointer_derefs")
    if(!view.contains(&raw_needle) || !view.contains(&deref)) {
        env.error("safety report did not contain raw pointer counts")
    }
}

@test
func safety_runtime_panic_harness(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    panic(\"expected safety test panic\")\n    return 0\n}\n"
    expect_runtime_panic(env, "runtime_panic_harness", ch)
}

public func main(argc : int, argv : **char) {
    test_runner(argc, argv)
}
