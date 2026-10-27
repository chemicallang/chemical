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

// Compile and run `ch_content`; returns the process exit code and captures
// stdout+stderr into `out`.
internal func safety_compile_and_run(env : &mut TestEnv, name : *char, ch_content : *char,
                                     out : &mut string) : int {
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
        return -1
    }

    var run_path = string()
    var run_view = std::string_view(unsafe(&raw exe_path[0]), strlen(unsafe(&raw exe_path[0])))
    run_path.append_view(&run_view)
    var run_args = vector<string>()
    run_args.push(run_path)
    return safety_run_command_capture(run_args, out)
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
    var ch = "func read_ptr(p : *mut int) : int {\n    return unsafe(*p)\n}\npublic func main() : int {\n    return 0\n}\n"
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

@test
func deque_bounds_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var d = std::deque<int>()\n    d.push(1)\n    var p = d.get_ptr(1)\n    return 0\n}\n"
    expect_runtime_panic(env, "deque_bounds_panics", ch)
}

@test
func span_bounds_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var a : [2]int = [1, 2]\n    var s = std::span<int>(&raw mut a[0], 2)\n    var p = s.get(2)\n    return 0\n}\n"
    expect_runtime_panic(env, "span_bounds_panics", ch)
}

@test
func string_view_subview_start_after_end_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var v = std::string_view(\"abc\")\n    var s = v.subview(2, 1)\n    return 0\n}\n"
    expect_runtime_panic(env, "string_view_subview_start_after_end_panics", ch)
}

@test
func string_view_subview_end_past_size_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var v = std::string_view(\"abc\")\n    var s = v.subview(0, 4)\n    return 0\n}\n"
    expect_runtime_panic(env, "string_view_subview_end_past_size_panics", ch)
}

@test
func string_view_skip_past_size_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var v = std::string_view(\"abc\")\n    var s = v.skip(4)\n    return 0\n}\n"
    expect_runtime_panic(env, "string_view_skip_past_size_panics", ch)
}

@test
func rc_use_after_try_unwrap_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var r = std::rc(1)\n    var first = r.try_unwrap()\n    var second = r.try_unwrap()\n    return 0\n}\n"
    expect_runtime_panic(env, "rc_use_after_try_unwrap_panics", ch)
}

@test
func arc_use_after_try_unwrap_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var a = std::arc(1)\n    var first = a.try_unwrap()\n    var second = a.try_unwrap()\n    return 0\n}\n"
    expect_runtime_panic(env, "arc_use_after_try_unwrap_panics", ch)
}

@test
func box_use_after_into_inner_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var b = std::box(5)\n    var first = b.into_inner()\n    var second = b.into_inner()\n    return first + second\n}\n"
    expect_runtime_panic(env, "box_use_after_into_inner_panics", ch)
}

@test
func vector_capacity_overflow_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var v = std::vector<int>()\n    v.reserve((0 as size_t) - 1)\n    return 0\n}\n"
    expect_runtime_panic(env, "vector_capacity_overflow_panics", ch)
}

@test
func string_capacity_overflow_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var s = std::string(\"x\")\n    s.reserve((0 as size_t) - 1)\n    return 0\n}\n"
    expect_runtime_panic(env, "string_capacity_overflow_panics", ch)
}

@test
func string_length_overflow_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var s = std::string(\"x\")\n    s.resize((0 as size_t) - 1)\n    return 0\n}\n"
    expect_runtime_panic(env, "string_length_overflow_panics", ch)
}

@test
func compound_division_by_zero_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var x = 10\n    x /= 0\n    return x\n}\n"
    expect_runtime_panic(env, "compound_division_by_zero_panics", ch)
}

@test
func compound_shift_out_of_range_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var x = 1\n    x <<= 64\n    return x\n}\n"
    expect_runtime_panic(env, "compound_shift_out_of_range_panics", ch)
}

@test
func compound_index_division_by_zero_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var a : [2]int = [10, 20]\n    var i = 0\n    a[i] /= 0\n    return a[i]\n}\n"
    expect_runtime_panic(env, "compound_index_division_by_zero_panics", ch)
}

@test
func compound_field_division_by_zero_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "struct Holder {\n    var value : int\n}\npublic func main() : int {\n    var h = Holder { value : 10 }\n    h.value /= 0\n    return h.value\n}\n"
    expect_runtime_panic(env, "compound_field_division_by_zero_panics", ch)
}

@test
func string_substring_start_after_end_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var s = std::string(\"abc\")\n    var out = s.substring(2, 1)\n    return 0\n}\n"
    expect_runtime_panic(env, "string_substring_start_after_end_panics", ch)
}

@test
func string_substring_end_past_size_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var s = std::string(\"abc\")\n    var out = s.substring(0, 4)\n    return 0\n}\n"
    expect_runtime_panic(env, "string_substring_end_past_size_panics", ch)
}

@test
func string_bounds_read_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var s = std::string(\"abc\")\n    return s.get(3) as int\n}\n"
    expect_runtime_panic(env, "string_bounds_read_panics", ch)
}

@test
func string_bounds_write_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var s = std::string(\"abc\")\n    s.set(3, 'x')\n    return 0\n}\n"
    expect_runtime_panic(env, "string_bounds_write_panics", ch)
}

@test
func string_view_bounds_read_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var v = std::string_view(\"abc\")\n    return v.get(3) as int\n}\n"
    expect_runtime_panic(env, "string_view_bounds_read_panics", ch)
}

@test
func division_by_zero_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var z = 0\n    return 10 / z\n}\n"
    expect_runtime_panic(env, "division_by_zero_panics", ch)
}

@test
func remainder_by_zero_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var z = 0\n    return 10 % z\n}\n"
    expect_runtime_panic(env, "remainder_by_zero_panics", ch)
}

@test
func shift_out_of_range_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var s = 64\n    return 1 << s\n}\n"
    expect_runtime_panic(env, "shift_out_of_range_panics", ch)
}

@test
func negative_shift_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "func get_shift() : int { return -1 }\npublic func main() : int {\n    var s = get_shift()\n    return 1 << s\n}\n"
    expect_runtime_panic(env, "negative_shift_panics", ch)
}

@test
func signed_division_overflow_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var a : i64 = -9223372036854775807 - 1\n    var b : i64 = -1\n    return (a / b) as int\n}\n"
    expect_runtime_panic(env, "signed_division_overflow_panics", ch)
}

@test
func array_bounds_dynamic_read_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var a : [3]int = [1, 2, 3]\n    var i = 3\n    return a[i]\n}\n"
    expect_runtime_panic(env, "array_bounds_dynamic_read_panics", ch)
}

@test
func array_bounds_dynamic_store_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var a : [3]int = [1, 2, 3]\n    var i = 4\n    a[i] = 7\n    return 0\n}\n"
    expect_runtime_panic(env, "array_bounds_dynamic_store_panics", ch)
}

@test
func vector_bounds_index_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var v = std::vector<int>()\n    v.push(1)\n    var p = v.get_ptr(1)\n    return 0\n}\n"
    expect_runtime_panic(env, "vector_bounds_index_panics", ch)
}

@test
func vector_bounds_empty_pop_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var v = std::vector<int>()\n    v.pop_back()\n    return 0\n}\n"
    expect_runtime_panic(env, "vector_bounds_empty_pop_panics", ch)
}

@test
func vector_bounds_empty_last_ptr_panics(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "public func main() : int {\n    var v = std::vector<int>()\n    var p = v.last_ptr()\n    return 0\n}\n"
    expect_runtime_panic(env, "vector_bounds_empty_last_ptr_panics", ch)
}

@test
func partial_init_destroys_only_initialized_fields(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "struct Tracker {\n    var id : int\n    @delete func delete(&mut self) { printf(\"drop %d\\n\", self.id) }\n}\nstruct S {\n    var a : Tracker\n    var b : Tracker\n}\npublic func main() : int {\n    unsafe {\n        var s : S\n        s.a = Tracker { id : 1 }\n    }\n    printf(\"end\\n\")\n    return 0\n}\n"
    var out = string()
    var rc = safety_compile_and_run(env, "partial_init_drop", ch, &mut out)
    if(rc != 0) {
        env.error("expected the partial-init program to exit successfully")
        return
    }
    var view = out.to_view()
    var drop_a = std::string_view("drop 1")
    var drop_b = std::string_view("drop 2")
    var end = std::string_view("end")
    if(!view.contains(&drop_a)) {
        env.error("the initialized field was not destroyed")
    }
    if(view.contains(&drop_b)) {
        env.error("an uninitialized field was destroyed")
    }
    if(!view.contains(&end)) {
        env.error("partial-init program did not finish")
    }
}

@test
func full_field_init_destroys_every_field(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "struct Tracker {\n    var id : int\n    @delete func delete(&mut self) { printf(\"drop %d\\n\", self.id) }\n}\nstruct S {\n    var a : Tracker\n    var b : Tracker\n}\npublic func main() : int {\n    unsafe {\n        var s : S\n        s.a = Tracker { id : 1 }\n        s.b = Tracker { id : 2 }\n    }\n    printf(\"end\\n\")\n    return 0\n}\n"
    var out = string()
    var rc = safety_compile_and_run(env, "full_field_init_drop", ch, &mut out)
    if(rc != 0) {
        env.error("expected the full-init program to exit successfully")
        return
    }
    var view = out.to_view()
    var drop_a = std::string_view("drop 1")
    var drop_b = std::string_view("drop 2")
    if(!view.contains(&drop_a) || !view.contains(&drop_b)) {
        env.error("a fully initialized aggregate did not destroy every field")
    }
}

@test
func partial_init_reassignment_destroys_old_value(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "struct Tracker {\n    var id : int\n    @delete func delete(&mut self) { printf(\"drop %d\\n\", self.id) }\n}\nstruct S {\n    var a : Tracker\n    var b : Tracker\n}\npublic func main() : int {\n    unsafe {\n        var s : S\n        s.a = Tracker { id : 1 }\n        s = S { a : Tracker { id : 10 }, b : Tracker { id : 20 } }\n    }\n    printf(\"end\\n\")\n    return 0\n}\n"
    var out = string()
    var rc = safety_compile_and_run(env, "partial_init_reassign", ch, &mut out)
    if(rc != 0) {
        env.error("expected the reassignment program to exit successfully")
        return
    }
    var view = out.to_view()
    var old = std::string_view("drop 1")
    var new_a = std::string_view("drop 10")
    var new_b = std::string_view("drop 20")
    if(!view.contains(&old)) {
        env.error("the old initialized field was not destroyed on reassignment")
    }
    if(!view.contains(&new_a) || !view.contains(&new_b)) {
        env.error("the replacement value was not destroyed")
    }
}

@test
func partial_array_init_destroys_only_initialized_elements(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "struct Tracker {\n    var id : int\n    @delete func delete(&mut self) { printf(\"drop %d\\n\", self.id) }\n}\npublic func main() : int {\n    unsafe {\n        var arr : [3]Tracker\n        arr[0] = Tracker { id : 1 }\n        arr[2] = Tracker { id : 3 }\n    }\n    printf(\"end\\n\")\n    return 0\n}\n"
    var out = string()
    var rc = safety_compile_and_run(env, "partial_array_init_drop", ch, &mut out)
    if(rc != 0) {
        env.error("expected the partial-array program to exit successfully")
        return
    }
    var view = out.to_view()
    var one = std::string_view("drop 1")
    var three = std::string_view("drop 3")
    var two = std::string_view("drop 2")
    if(!view.contains(&one) || !view.contains(&three)) {
        env.error("initialized array elements were not destroyed")
    }
    if(view.contains(&two)) {
        env.error("an uninitialized array element was destroyed")
    }
}

@test
func partial_array_reassignment_destroys_old_element(env : &mut TestEnv) {
    safety_init_work_dir()
    var ch = "struct Tracker {\n    var id : int\n    @delete func delete(&mut self) { printf(\"drop %d\\n\", self.id) }\n}\npublic func main() : int {\n    unsafe {\n        var arr : [2]Tracker\n        arr[0] = Tracker { id : 1 }\n        arr[0] = Tracker { id : 9 }\n    }\n    printf(\"end\\n\")\n    return 0\n}\n"
    var out = string()
    var rc = safety_compile_and_run(env, "partial_array_reassign_drop", ch, &mut out)
    if(rc != 0) {
        env.error("expected the array reassignment program to exit successfully")
        return
    }
    var view = out.to_view()
    var old = std::string_view("drop 1")
    var replacement = std::string_view("drop 9")
    if(!view.contains(&old)) {
        env.error("the old array element was not destroyed on reassignment")
    }
    if(!view.contains(&replacement)) {
        env.error("the replacement array element was not destroyed")
    }
}

public func main(argc : int, argv : **char) {
    test_runner(argc, argv)
}
