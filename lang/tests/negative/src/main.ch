
// The work directory where negative-test modules are created.
// Resolved at runtime via fs::temp_dir so it is writable on every platform
// (POSIX: /tmp, Windows: %TEMP%). The old hardcoded "/tmp/chemical_neg_tests"
// resolves to "\tmp\..." on the current drive on Windows, which usually is
// not writable without admin rights.
internal unsafe var g_neg_work_dir_buf : [512]char
internal var NEG_WORK_DIR : *char = null

internal func neg_init_work_dir() {
    var r = fs::temp_dir(&raw mut g_neg_work_dir_buf[0], 512 as size_t)
    var i : size_t = 0
    if(r is std::Result.Ok) {
        var Ok(len) = r else unreachable
        i = len
    } else {
        // fallback: relative to the current working directory
        var fb = "chemical_neg_tests\0"
        while(fb[i] != 0 && i < 511) { g_neg_work_dir_buf[i] = fb[i]; i++ }
        g_neg_work_dir_buf[i] = 0
        NEG_WORK_DIR = &raw mut g_neg_work_dir_buf[0]
        return
    }
    // strip any trailing separator (POSIX returns "/tmp", Windows "%TEMP%\\")
    if(i > 0 && (g_neg_work_dir_buf[i-1] == '/' || g_neg_work_dir_buf[i-1] == '\\')) {
        i--
    }
    var suffix = "/chemical_neg_tests\0"
    var k = 0
    while(suffix[k] != 0 && i < 511) { g_neg_work_dir_buf[i] = suffix[k]; i++; k++ }
    g_neg_work_dir_buf[i] = 0
    NEG_WORK_DIR = &raw mut g_neg_work_dir_buf[0]
}

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

internal func run_compiler_capture(mod_path : *char, out_path : *char, output_buf : *mut char, buf_size : int) : int {
    unsafe var cmd : char[2048]
    // Write the (failed or successful) output executable into the test dir;
    // "/dev/null" is POSIX-only and on Windows would try to create "\dev\null"
    // on the current drive. Quoting every argument keeps paths with spaces working.
    comptime if(def.windows) {
        // _popen on Windows routes through cmd.exe, whose /S /C parsing breaks
        // when the command line starts with a quote. The doubled-quote pattern
        // is the canonical way to quote a command for cmd.exe and keeps paths
        // with spaces working.
        sprintf(&raw mut cmd[0], "cmd /S /C \"\"%s\" \"%s\" --no-cache -o \"%s\"\" 2>&1", intrinsics::get_compiler_path(), mod_path, out_path)
    } else {
        sprintf(&raw mut cmd[0], "\"%s\" \"%s\" --no-cache -o \"%s\" 2>&1", intrinsics::get_compiler_path(), mod_path, out_path)
    }
    var pipe = popen(&raw mut cmd[0], "r")
    if(pipe == null) {
        return -1
    }
    var total = 0
    unsafe var line_buf : char[4096]
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
    unsafe var test_dir : char[512]
    sprintf(&raw mut test_dir[0], "%s/%s", work_dir, name)
    fs::mkdir(&raw test_dir[0])

    unsafe var mod_path : char[512]
    sprintf(&raw mut mod_path[0], "%s/chemical.mod", &raw test_dir[0])
    write_file(&raw mod_path[0], mod_content)

    unsafe var ch_path : char[512]
    sprintf(&raw mut ch_path[0], "%s/test.ch", &raw test_dir[0])
    write_file(&raw ch_path[0], ch_content)

    return true
}

internal func cleanup_test_dir(work_dir : *char, name : *char) {
    unsafe var path : char[512]
    sprintf(&raw mut path[0], "%s/%s", work_dir, name)
    // cross-platform recursive delete (no shelling out to "rm -rf", which
    // does not exist on Windows cmd.exe)
    fs::remove_dir_all_recursive(&raw path[0])
}

internal func neg_debug_print(name : *char, output : *char) {
    var dbg = getenv("NEG_DEBUG")
    if(dbg != null && dbg[0] != 0) {
        printf("==== [NEG_DEBUG] %s ====\n", name)
        printf("%s", output)
        printf("==== [NEG_DEBUG end] ====\n")
    }
}

internal const NEG_MOD = "module neg_test\nsource \".\"\n"

// variant that imports core so the `Copy` marker interface is in scope
internal const NEG_MOD_CORE = "module neg_test\nsource \".\"\nimport core\n"

internal func expect_compile_error(env : &mut TestEnv, name : *char, ch_content : *char, expected_sub : *char) {
    expect_compile_error_with_mod(env, name, ch_content, expected_sub, NEG_MOD)
}

internal func expect_compile_error_with_mod(env : &mut TestEnv, name : *char, ch_content : *char, expected_sub : *char, mod_content : *char) {
    setup_test_files(NEG_WORK_DIR, name, mod_content, ch_content)

    unsafe var mod_path : char[512]
    sprintf(&raw mut mod_path[0], "%s/%s/chemical.mod", NEG_WORK_DIR, name)
    unsafe var out_path : char[512]
    sprintf(&raw mut out_path[0], "%s/%s/out.exe", NEG_WORK_DIR, name)

    unsafe var output_buf : char[16384]
    var rc = run_compiler_capture(&raw mod_path[0], &raw out_path[0], &raw mut output_buf[0], 16384)

    var has_error = string_contains(&raw output_buf[0], "error:")
    var has_sub = if(strlen(expected_sub) == 0) true else string_contains(&raw output_buf[0], expected_sub)

    if(rc == 0) {
        env.error("expected compiler to fail but it succeeded")
        neg_debug_print(name, &raw output_buf[0])
    } else if(!has_error) {
        env.error("expected compiler error but did not find one")
        neg_debug_print(name, &raw output_buf[0])
    } else if(!has_sub) {
        env.error("expected error substring not found in output")
        neg_debug_print(name, &raw output_buf[0])
    }

    cleanup_test_dir(NEG_WORK_DIR, name)
}

internal func expect_compile_success(env : &mut TestEnv, name : *char, ch_content : *char) {
    setup_test_files(NEG_WORK_DIR, name, NEG_MOD, ch_content)

    unsafe var mod_path : char[512]
    sprintf(&raw mut mod_path[0], "%s/%s/chemical.mod", NEG_WORK_DIR, name)
    unsafe var out_path : char[512]
    sprintf(&raw mut out_path[0], "%s/%s/out.exe", NEG_WORK_DIR, name)

    unsafe var output_buf : char[16384]
    var rc = run_compiler_capture(&raw mod_path[0], &raw out_path[0], &raw mut output_buf[0], 16384)

    if(rc != 0) {
        env.error("expected compiler to succeed but it failed")
        neg_debug_print(name, &raw output_buf[0])
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
    var ch = "struct MyView 'a {\n    var data : *char\n}\nstruct MyObj {\n    func get_view(&self) : 'self MyView {\n        return MyView { data : null }\n    }\n    @delete\n    func delete(&mut self) { }\n}\npublic func main() : int {\n    var obj = MyObj()\n    var v = obj.get_view()\n    return 0\n}\n"
    expect_compile_success(env, "named_to_view_succeeds", ch)
}

@test
func neg_temp_no_lifetime_type_succeeds(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct MyObj {\n    func get_value(&self) : i32 {\n        return 42\n    }\n    @delete\n    func delete(&mut self) { }\n}\npublic func main() : int {\n    var v = MyObj().get_value()\n    return 0\n}\n"
    expect_compile_success(env, "temp_no_lifetime_type_succeeds", ch)
}

@test
func neg_temp_no_dtor_succeeds(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct MyView 'a {\n    var data : *char\n}\nstruct MyObj {\n    func get_view(&self) : 'self MyView {\n        return MyView { data : null }\n    }\n}\npublic func main() : int {\n    var v = MyObj().get_view()\n    return 0\n}\n"
    expect_compile_success(env, "temp_no_dtor_succeeds", ch)
}

@test
func neg_temp_no_return_lifetime_succeeds(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct MyView 'a {\n    var data : *char\n}\nfunc get_view(v : MyView) : MyView {\n    return v\n}\npublic func main() : int {\n    var v = get_view(MyView { data : null })\n    return 0\n}\n"
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

// vector<T>::get has `where T : Copy`. Calling .get on a vector whose element type
// carries a destructor (and is therefore not Copy) must be rejected at compile time.
// If it were allowed, the returned temporary would be destroyed and would free the
// element's buffers, corrupting the original in the vector.
@test
func neg_get_on_destructible_struct_direct_errors(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func get(&self, i : int) : T where T : Copy {\n        return *data\n    }\n}\nstruct Row {\n    var p : *char\n    @delete\n    func delete(&mut self) { }\n}\npublic func main() : int {\n    var h = Holder<Row> { data = null }\n    var r = h.get(0)\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "get_on_destructible_struct_direct", ch, "does not satisfy where clause constraint", NEG_MOD_CORE)
}

// Same as above but the receiver of .get is reached through a member access
// (outer.h.get(0)), which is exactly how `qr.rows.get(0)` triggered the bug.
@test
func neg_get_on_destructible_struct_nested_receiver_errors(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func get(&self, i : int) : T where T : Copy {\n        return *data\n    }\n}\nstruct Row {\n    var p : *char\n    @delete\n    func delete(&mut self) { }\n}\nstruct Outer {\n    var h : Holder<Row>\n}\npublic func main() : int {\n    var outer = Outer { h = Holder<Row> { data = null } }\n    var r = outer.h.get(0)\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "get_on_destructible_struct_nested", ch, "does not satisfy where clause constraint", NEG_MOD_CORE)
}

public func main(argc : int, argv : **char) {
    neg_init_work_dir()
    test_runner(argc, argv)
}