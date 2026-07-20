
internal func write_file(path : *char, content : *char) {
    var f = fopen(path, "w")
    if(f != null) {
        fwrite(content as *void, strlen(content), 1, f)
        fclose(f)
    }
}

internal func string_contains(haystack : *char, needle : *char) : bool {
    var nlen = strlen(needle)
    if(nlen == 0) return true
    while(*haystack != 0) {
        var i = 0u
        var match = true
        while(i < nlen) {
            if(*(haystack + i) != *(needle + i)) {
                match = false
                break
            }
            i++
        }
        if(match) return true
        haystack++
    }
    return false
}

internal func run_compiler_capture(mod_path : *char, output_buf : *mut char, buf_size : int) : int {
    var cmd : char[2048]
    sprintf(&raw mut cmd[0], "%s \"%s\" --no-cache -o /dev/null 2>&1", intrinsics::get_compiler_path(), mod_path)
    var pipe = popen(&raw mut cmd[0], "r")
    if(pipe == null) {
        return -1
    }
    var total = 0
    var line_buf : char[4096]
    while(fgets(&raw mut line_buf[0], 4096, pipe) != null) {
        var line_len = strlen(&raw line_buf[0])
        var i = 0u
        while(i < line_len && total < buf_size - 1) {
            *(output_buf + total) = line_buf[i]
            total++
            i++
        }
    }
    *(output_buf + total) = 0
    var rc = pclose(pipe)
    return rc
}

internal func setup_test_files(work_dir : *char, name : *char, mod_content : *char, ch_content : *char) : bool {
    var test_dir : char[512]
    sprintf(&raw mut test_dir[0], "%s/%s", work_dir, name)
    mkdir(&raw test_dir[0], 0o777 as uint)

    var mod_path : char[512]
    sprintf(&raw mut mod_path[0], "%s/chemical.mod", &raw test_dir[0])
    write_file(&raw mod_path[0], mod_content)

    var ch_path : char[512]
    sprintf(&raw mut ch_path[0], "%s/test.ch", &raw test_dir[0])
    write_file(&raw ch_path[0], ch_content)

    return true
}

internal func cleanup_test_dir(work_dir : *char, name : *char) {
    var rm_cmd : char[512]
    sprintf(&raw mut rm_cmd[0], "rm -rf \"%s/%s\"", work_dir, name)
    system(&raw rm_cmd[0])
}

internal const NEG_WORK_DIR = "/tmp/chemical_neg_tests"
internal const NEG_MOD = "module neg_test\nsource \".\"\n"

internal func expect_compile_error(env : &mut TestEnv, name : *char, ch_content : *char, expected_sub : *char) {
    setup_test_files(NEG_WORK_DIR, name, NEG_MOD, ch_content)

    var mod_path : char[512]
    sprintf(&raw mut mod_path[0], "%s/%s/chemical.mod", NEG_WORK_DIR, name)

    var output_buf : char[16384]
    var rc = run_compiler_capture(&raw mod_path[0], &raw mut output_buf[0], 16384)

    var has_error = string_contains(&raw output_buf[0], "error:")
    var has_sub = if(strlen(expected_sub) == 0) true else string_contains(&raw output_buf[0], expected_sub)

    if(rc == 0) {
        env.error("expected compiler to fail but it succeeded")
    } else if(!has_error) {
        env.error("expected compiler error but did not find one")
    } else if(!has_sub) {
        env.error("expected error substring not found in output")
    }

    cleanup_test_dir(NEG_WORK_DIR, name)
}

internal func expect_compile_success(env : &mut TestEnv, name : *char, ch_content : *char) {
    setup_test_files(NEG_WORK_DIR, name, NEG_MOD, ch_content)

    var mod_path : char[512]
    sprintf(&raw mut mod_path[0], "%s/%s/chemical.mod", NEG_WORK_DIR, name)

    var output_buf : char[16384]
    var rc = run_compiler_capture(&raw mod_path[0], &raw mut output_buf[0], 16384)

    if(rc != 0) {
        env.error("expected compiler to succeed but it failed")
        var msg : char[512]
        sprintf(&raw mut msg[0], "output: %s", &raw output_buf[0])
        env.info(&raw msg[0])
    }

    cleanup_test_dir(NEG_WORK_DIR, name)
}

@test
func neg_temp_to_view_with_dtor_errors(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct MyView 'a {\n    var data : *char\n}\nstruct MyObj {\n    func get_view(&self) : 'self MyView {\n        return MyView { data : null }\n    }\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var v = MyObj().get_view()\n}\n"
    expect_compile_error(env, "temp_to_view_with_dtor_errors", ch, "lifetime dependency")
}

@test
func neg_named_to_view_succeeds(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct MyView 'a {\n    var data : *char\n}\nstruct MyObj {\n    func get_view(&self) : 'self MyView {\n        return MyView { data : null }\n    }\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var obj = MyObj()\n    var v = obj.get_view()\n}\n"
    expect_compile_success(env, "named_to_view_succeeds", ch)
}

@test
func neg_temp_no_lifetime_type_succeeds(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct MyObj {\n    func get_value(&self) : i32 {\n        return 42\n    }\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var v = MyObj().get_value()\n}\n"
    expect_compile_success(env, "temp_no_lifetime_type_succeeds", ch)
}

@test
func neg_temp_no_dtor_succeeds(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct MyView 'a {\n    var data : *char\n}\nstruct MyObj {\n    func get_view(&self) : 'self MyView {\n        return MyView { data : null }\n    }\n}\nfunc main() {\n    var v = MyObj().get_view()\n}\n"
    expect_compile_success(env, "temp_no_dtor_succeeds", ch)
}

@test
func neg_temp_no_return_lifetime_succeeds(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct MyView 'a {\n    var data : *char\n}\nfunc get_view(v : MyView) : MyView {\n    return v\n}\nfunc main() {\n    var v = get_view(MyView { data : null })\n}\n"
    expect_compile_success(env, "temp_no_return_lifetime_succeeds", ch)
}

@test
func neg_lambda_param_unresolved_child_prints_error(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // Lambda parameter type can't be inferred and accessing a child on it
    // should print a proper error, not crash with SIGSEGV.
    var ch = "func main() {\n    var cb = ||(x) => {\n        x.some_nonexistent_field\n    }\n}\n"
    expect_compile_error(env, "lambda_param_unresolved_child", ch, "unresolved")
}

public func main(argc : int, argv : **char) {
    test_runner(argc, argv)
}