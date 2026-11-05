
// The work directory where negative-test modules are created.
// Resolved at runtime via fs::temp_dir so it is writable on every platform
// (POSIX: /tmp, Windows: %TEMP%). The old hardcoded "/tmp/chemical_neg_tests"
// resolves to "\tmp\..." on the current drive on Windows, which usually is
// not writable without admin rights.
internal var g_neg_work_dir_buf : [512]char
internal var NEG_WORK_DIR : *char = null

internal func neg_init_work_dir() {
        unsafe {
        var r = fs::temp_dir(unsafe(&raw mut g_neg_work_dir_buf[0]), 512 as size_t)
        var i : size_t = 0
        if(r is std::Result.Ok) {
            var Ok(len) = r else unsafe { unreachable }
            i = len
        } else {
            // fallback: relative to the current working directory
            var fb = "chemical_neg_tests\0"
            while(fb[i] != 0 && i < 511) { g_neg_work_dir_buf[i] = fb[i]; i++ }
            g_neg_work_dir_buf[i] = 0
            NEG_WORK_DIR = unsafe(&raw mut g_neg_work_dir_buf[0])
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
        NEG_WORK_DIR = unsafe(&raw mut g_neg_work_dir_buf[0])

        }
    }

internal func write_file(path : *char, content : *char) {
    var f = unsafe(fopen(path, "w"))
    if(f != null) {
        unsafe { fwrite(content as *void, unsafe(strlen(content)), 1, f) }
        unsafe { fclose(f) }
    }
}

internal func string_contains(haystack : *char, needle : *char) : bool { unsafe {
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
 }}

internal func run_compiler_capture(mod_path : *char, out_path : *char, output_buf : *mut char, buf_size : int, mode : *char = "", extra_args : *char = "") : int { unsafe {
    var cmd : char[2048]
    var mode_flag : *char = ""
    if(mode[0] != 0) {
        mode_flag = "--mode "
    }
    // Write the (failed or successful) output executable into the test dir;
    // "/dev/null" is POSIX-only and on Windows would try to create "\dev\null"
    // on the current drive. Quoting every argument keeps paths with spaces working.
    comptime if(def.windows) {
        // _popen on Windows routes through cmd.exe, whose /S /C parsing breaks
        // when the command line starts with a quote. The doubled-quote pattern
        // is the canonical way to quote a command for cmd.exe and keeps paths
        // with spaces working.
        if(mode[0] != 0) {
            sprintf(unsafe(&raw mut cmd[0]), "cmd /S /C \"\"%s\" \"%s\" --no-cache %s%s%s -o \"%s\"\" 2>&1", intrinsics::get_compiler_path(), mod_path, mode_flag, mode, extra_args, out_path)
        } else {
            sprintf(unsafe(&raw mut cmd[0]), "cmd /S /C \"\"%s\" \"%s\" --no-cache%s -o \"%s\"\" 2>&1", intrinsics::get_compiler_path(), mod_path, extra_args, out_path)
        }
    } else {
        if(mode[0] != 0) {
            sprintf(unsafe(&raw mut cmd[0]), "\"%s\" \"%s\" --no-cache %s%s%s -o \"%s\" 2>&1", intrinsics::get_compiler_path(), mod_path, mode_flag, mode, extra_args, out_path)
        } else {
            sprintf(unsafe(&raw mut cmd[0]), "\"%s\" \"%s\" --no-cache%s -o \"%s\" 2>&1", intrinsics::get_compiler_path(), mod_path, extra_args, out_path)
        }
    }
    var pipe = popen(unsafe(&raw mut cmd[0]), "r")
    if(pipe == null) {
        return -1
    }
    // Keep only the LAST (buf_size - 1) output bytes. Compiling a module that
    // imports std/page can emit tens of KB of warnings before the actual
    // diagnostic; truncating the newest bytes would drop the error entirely.
    // A ring buffer is linearized (rotated) at the end so callers see the tail.
    var cap = buf_size - 1
    var head = 0
    var count = 0
    var line_buf : char[4096]
    while(fgets(unsafe(&raw mut line_buf[0]), 4096, pipe) != null) {
        var line_len = strlen(unsafe(&raw line_buf[0]))
        var i = 0u
        while(i < line_len) {
            *(output_buf + head) = line_buf[i]
            head++
            if(head >= cap) head = 0
            if(count < cap) count++
            i++
        }
    }
    if(count < cap || head == 0) {
        *(output_buf + count) = 0
    } else {
        // wrapped: rotate left by `head` to restore oldest-first order
        var swap1 : char = 0
        var lo = 0
        var hi = cap - 1
        while(lo < hi) {
            swap1 = *(output_buf + lo)
            *(output_buf + lo) = *(output_buf + hi)
            *(output_buf + hi) = swap1
            lo++
            hi--
        }
        lo = 0
        hi = cap - head - 1
        while(lo < hi) {
            swap1 = *(output_buf + lo)
            *(output_buf + lo) = *(output_buf + hi)
            *(output_buf + hi) = swap1
            lo++
            hi--
        }
        lo = cap - head
        hi = cap - 1
        while(lo < hi) {
            swap1 = *(output_buf + lo)
            *(output_buf + lo) = *(output_buf + hi)
            *(output_buf + hi) = swap1
            lo++
            hi--
        }
        *(output_buf + cap) = 0
    }
    var rc = pclose(pipe)
    return rc
 }}

internal func setup_test_files(work_dir : *char, name : *char, mod_content : *char, ch_content : *char) : bool {
    var test_dir : char[512]
    unsafe { sprintf(unsafe(&raw mut test_dir[0]), "%s/%s", work_dir, name) }
    fs::mkdir(unsafe(&raw test_dir[0]))

    var mod_path : char[512]
    unsafe { sprintf(unsafe(&raw mut mod_path[0]), "%s/chemical.mod", unsafe(&raw test_dir[0])) }
    write_file(unsafe(&raw mod_path[0]), mod_content)

    var ch_path : char[512]
    unsafe { sprintf(unsafe(&raw mut ch_path[0]), "%s/test.ch", unsafe(&raw test_dir[0])) }
    write_file(unsafe(&raw ch_path[0]), ch_content)

    return true
}

internal func cleanup_test_dir(work_dir : *char, name : *char) {
    var path : char[512]
    unsafe { sprintf(unsafe(&raw mut path[0]), "%s/%s", work_dir, name) }
    // cross-platform recursive delete (no shelling out to "rm -rf", which
    // does not exist on Windows cmd.exe)
    fs::remove_dir_all_recursive(unsafe(&raw path[0]))
}

internal func neg_debug_print(name : *char, output : *char) {
    var dbg = unsafe(getenv("NEG_DEBUG"))
    if(dbg != null && unsafe(dbg[0]) != 0) {
        unsafe { printf("==== [NEG_DEBUG] %s ====\n", name) }
        unsafe { printf("%s", output) }
        unsafe { printf("==== [NEG_DEBUG end] ====\n") }
    }
}

internal const NEG_MOD = "module neg_test\nsource \".\"\n"

// variant that imports cstd (malloc/free and the C runtime decls)
internal const NEG_MOD_CSTD = "module neg_test\nsource \".\"\nimport cstd\n"

// variant that imports core so the `Copy` marker interface is in scope
internal const NEG_MOD_CORE = "module neg_test\nsource \".\"\nimport core\n"
// Windows-target platform audit: these modules contain OS-gated FFI wrappers.
internal const NEG_MOD_WIN_LIBS = "application neg_test\nsource \".\"\nimport cstd\nimport std\nimport fs\nimport net\nimport test\nimport async\nimport tls\nimport http\nimport server\nimport window\nimport webview\n"

internal const NEG_MOD_POSIX_LIBS = "application neg_test\nsource \".\"\nimport cstd\nimport std\nimport fs\nimport net\nimport test\nimport async\nimport tls\nimport http\nimport server\n"
internal const NEG_MOD_STD_CORE = "module neg_test\nsource \".\"\nimport std\nimport core\n"

// an application importing the async protocol, so `async func main` gets its
// synchronous trampoline and becomes the program entry point
internal const NEG_MOD_ASYNC_APP = "application neg_test\nsource \".\"\nimport cstd\nimport core\nimport async\n"

// same, plus `std` for `std::string` (used by the coroutine regression)
internal const NEG_MOD_ASYNC_STD = "application neg_test\nsource \".\"\nimport cstd\nimport std\nimport core\nimport async\n"

// variant that imports page + universal_cbi so `#universal` components compile
internal const NEG_MOD_JSON = "module neg_test\nsource \".\"\nimport std\nimport json\nimport json_cbi\n"

internal const NEG_MOD_UNIVERSAL = "module neg_test\nsource \".\"\nimport std\nimport page\nimport universal_cbi\n"

// variant that imports page + html_cbi so `#html` macros compile
internal const NEG_MOD_HTML = "module neg_test\nsource \".\"\nimport std\nimport page\nimport html_cbi\n"

// variant that imports both, so a `#universal` component can be used in `#html`
internal const NEG_MOD_UNIVERSAL_HTML = "module neg_test\nsource \".\"\nimport std\nimport page\nimport universal_cbi\nimport html_cbi\n"

internal func expect_compile_error(env : &mut TestEnv, name : *char, ch_content : *char, expected_sub : *char) {
    expect_compile_error_with_mod(env, name, ch_content, expected_sub, NEG_MOD)
}

internal func expect_compile_error_with_mod(env : &mut TestEnv, name : *char, ch_content : *char, expected_sub : *char, mod_content : *char) {
        unsafe {
        setup_test_files(NEG_WORK_DIR, name, mod_content, ch_content)

        var mod_path : char[512]
        unsafe { sprintf(unsafe(&raw mut mod_path[0]), "%s/%s/chemical.mod", NEG_WORK_DIR, name) }
        var out_path : char[512]
        unsafe { sprintf(unsafe(&raw mut out_path[0]), "%s/%s/out.exe", NEG_WORK_DIR, name) }

        var output_buf : char[65536]
        var rc = run_compiler_capture(unsafe(&raw mod_path[0]), unsafe(&raw out_path[0]), unsafe(&raw mut output_buf[0]), 65536)

        var out_view = unsafe(&raw output_buf[0])
        // Coded diagnostics render as `error[SAFE-*]:`; uncoded ones as `error:`.
        var has_error = string_contains(out_view, "error:") || string_contains(out_view, "error[")
        var has_sub = if(unsafe(strlen(expected_sub)) == 0) true else string_contains(out_view, expected_sub)

        if(rc == 0) {
            env.error("expected compiler to fail but it succeeded")
            neg_debug_print(name, unsafe(&raw output_buf[0]))
        } else if(!has_error) {
            env.error("expected compiler error but did not find one")
            neg_debug_print(name, unsafe(&raw output_buf[0]))
        } else if(!has_sub) {
            env.error("expected error substring not found in output")
            neg_debug_print(name, unsafe(&raw output_buf[0]))
        }

        cleanup_test_dir(NEG_WORK_DIR, name)

        }
    }

// Compiles a program that must succeed, and asserts that the output contains
// the given warning code (for example SAFE-UNSAFE-001). Used while a safety
// rule is still in report-only mode.
internal func expect_compile_warning_id(env : &mut TestEnv, name : *char, ch_content : *char, expected_id : *char) {
        unsafe {
        setup_test_files(NEG_WORK_DIR, name, NEG_MOD, ch_content)

        var mod_path : char[512]
        unsafe { sprintf(unsafe(&raw mut mod_path[0]), "%s/%s/chemical.mod", NEG_WORK_DIR, name) }
        var out_path : char[512]
        unsafe { sprintf(unsafe(&raw mut out_path[0]), "%s/%s/out.exe", NEG_WORK_DIR, name) }

        var output_buf : char[65536]
        var rc = run_compiler_capture(unsafe(&raw mod_path[0]), unsafe(&raw out_path[0]), unsafe(&raw mut output_buf[0]), 65536)
        if(rc != 0) {
            env.error("expected compiler to succeed while emitting a warning")
            neg_debug_print(name, unsafe(&raw output_buf[0]))
        } else if(!string_contains(unsafe(&raw output_buf[0]), "warn[")) {
            env.error("expected a coded warning but found none")
            neg_debug_print(name, unsafe(&raw output_buf[0]))
        } else if(!string_contains(unsafe(&raw output_buf[0]), expected_id)) {
            env.error("expected warning code not found in output")
            neg_debug_print(name, unsafe(&raw output_buf[0]))
        }

        cleanup_test_dir(NEG_WORK_DIR, name)

        }
    }

// Like expect_compile_error_with_mod, but only asserts that the compiler
// *printed* the expected diagnostic. Used for the 2c (C translation) phase,
// whose diagnostics are printed and reset without failing the build.
internal func expect_compile_output_contains(env : &mut TestEnv, name : *char, ch_content : *char, expected_sub : *char, mod_content : *char) {
        unsafe {
        setup_test_files(NEG_WORK_DIR, name, mod_content, ch_content)

        var mod_path : char[512]
        unsafe { sprintf(unsafe(&raw mut mod_path[0]), "%s/%s/chemical.mod", NEG_WORK_DIR, name) }
        var out_path : char[512]
        unsafe { sprintf(unsafe(&raw mut out_path[0]), "%s/%s/out.exe", NEG_WORK_DIR, name) }

        var output_buf : char[65536]
        run_compiler_capture(unsafe(&raw mod_path[0]), unsafe(&raw out_path[0]), unsafe(&raw mut output_buf[0]), 65536)

        if(string_contains(unsafe(&raw output_buf[0]), expected_sub)) {
            env.success("compiler emitted the expected diagnostic")
        } else {
            env.error("expected diagnostic substring not found in output")
            neg_debug_print(name, unsafe(&raw output_buf[0]))
        }

        cleanup_test_dir(NEG_WORK_DIR, name)

        }
    }

// Asserts the compiler did NOT print `unexpected_sub` (e.g. a warning that must
// not fire for a valid case), while still compiling successfully.
internal func expect_compile_output_not_contains(env : &mut TestEnv, name : *char, ch_content : *char, unexpected_sub : *char, mod_content : *char) {
        unsafe {
        setup_test_files(NEG_WORK_DIR, name, mod_content, ch_content)

        var mod_path : char[512]
        unsafe { sprintf(unsafe(&raw mut mod_path[0]), "%s/%s/chemical.mod", NEG_WORK_DIR, name) }
        var out_path : char[512]
        unsafe { sprintf(unsafe(&raw mut out_path[0]), "%s/%s/out.exe", NEG_WORK_DIR, name) }

        var output_buf : char[65536]
        var rc = run_compiler_capture(unsafe(&raw mod_path[0]), unsafe(&raw out_path[0]), unsafe(&raw mut output_buf[0]), 65536)

        if(rc != 0) {
            env.error("expected compiler to succeed")
            neg_debug_print(name, unsafe(&raw output_buf[0]))
        } else if(string_contains(unsafe(&raw output_buf[0]), unexpected_sub)) {
            env.error("unexpected diagnostic substring found in output")
            neg_debug_print(name, unsafe(&raw output_buf[0]))
        }

        cleanup_test_dir(NEG_WORK_DIR, name)

        }
    }

// Returns true when `needle` occurs in the file at `path`.
internal func read_file_contains(path : *char, needle : *char) : bool {
    var buf : [131072]u8
    var r = fs::read_to_buffer(path, unsafe(&raw mut buf[0]), 131071)
    if(r is std::Result.Err) {
        return false
    }
    var Ok(n) = r else unsafe { unreachable }
    buf[n] = 0
    return string_contains(unsafe(&raw buf[0]) as *char, needle)
}

// Compiles the source with `-o out.c` (C translation, no link) and asserts the
// emitted translation unit does / does not contain `symbol`. Used to pin the
// "unused generic impls emit no code" invariant.
internal func expect_emitted_symbol(env : &mut TestEnv, name : *char, ch_content : *char, symbol : *char, should_exist : bool) {
        unsafe {
        setup_test_files(NEG_WORK_DIR, name, NEG_MOD, ch_content)

        // Compile the single source file directly: `-o <file>.c` selects the C
        // translation path, and a `.ch` input avoids the build.lab module pipeline.
        var ch_path : char[512]
        unsafe { sprintf(unsafe(&raw mut ch_path[0]), "%s/%s/test.ch", NEG_WORK_DIR, name) }
        var c_path : char[512]
        unsafe { sprintf(unsafe(&raw mut c_path[0]), "%s/%s/out.c", NEG_WORK_DIR, name) }

        var output_buf : char[65536]
        var rc = run_compiler_capture(unsafe(&raw ch_path[0]), unsafe(&raw c_path[0]), unsafe(&raw mut output_buf[0]), 65536)
        if(rc != 0) {
            env.error("expected compiler to succeed while emitting C")
            neg_debug_print(name, unsafe(&raw output_buf[0]))
            cleanup_test_dir(NEG_WORK_DIR, name)
            return
        }

        var found = read_file_contains(unsafe(&raw c_path[0]), symbol)
        if(found != should_exist) {
            if(should_exist) {
                env.error("expected the generic impl symbol in the emitted C, but it is missing")
            } else {
                env.error("unused generic impl emitted code: symbol found in the emitted C")
            }
            neg_debug_print(name, unsafe(&raw output_buf[0]))
        }

        cleanup_test_dir(NEG_WORK_DIR, name)

        }
    }

internal func expect_compile_success(env : &mut TestEnv, name : *char, ch_content : *char) {
        unsafe {
        setup_test_files(NEG_WORK_DIR, name, NEG_MOD, ch_content)

        var mod_path : char[512]
        unsafe { sprintf(unsafe(&raw mut mod_path[0]), "%s/%s/chemical.mod", NEG_WORK_DIR, name) }
        var out_path : char[512]
        unsafe { sprintf(unsafe(&raw mut out_path[0]), "%s/%s/out.exe", NEG_WORK_DIR, name) }

        var output_buf : char[65536]
        var rc = run_compiler_capture(unsafe(&raw mod_path[0]), unsafe(&raw out_path[0]), unsafe(&raw mut output_buf[0]), 65536)

        if(rc != 0) {
            env.error("expected compiler to succeed but it failed")
            neg_debug_print(name, unsafe(&raw output_buf[0]))
        }

        cleanup_test_dir(NEG_WORK_DIR, name)

        }
    }

internal func expect_compile_error_with_target(env : &mut TestEnv, name : *char, ch_content : *char, expected_sub : *char, mod_content : *char, target : *char) {
    unsafe {
        setup_test_files(NEG_WORK_DIR, name, mod_content, ch_content)
        var mod_path : char[512]
        unsafe { sprintf(unsafe(&raw mut mod_path[0]), "%s/%s/chemical.mod", NEG_WORK_DIR, name) }
        var out_path : char[512]
        unsafe { sprintf(unsafe(&raw mut out_path[0]), "%s/%s/out.exe", NEG_WORK_DIR, name) }
        var extra : char[256]
        unsafe { sprintf(unsafe(&raw mut extra[0]), " --target %s", target) }
        var output_buf : char[65536]
        var rc = run_compiler_capture(unsafe(&raw mod_path[0]), unsafe(&raw out_path[0]), unsafe(&raw mut output_buf[0]), 65536, "", unsafe(&raw extra[0]))
        var out_view = unsafe(&raw output_buf[0])
        var has_error = string_contains(out_view, "error:") || string_contains(out_view, "error[")
        var has_sub = if(unsafe(strlen(expected_sub)) == 0) true else string_contains(out_view, expected_sub)
        if(rc == 0) {
            env.error("expected compiler to fail but it succeeded")
            neg_debug_print(name, unsafe(&raw output_buf[0]))
        } else if(!has_error) {
            env.error("expected compiler error but did not find one")
            neg_debug_print(name, unsafe(&raw output_buf[0]))
        } else if(!has_sub) {
            env.error("expected error substring not found in output")
            neg_debug_print(name, unsafe(&raw output_buf[0]))
        }
    }
}

// Compiles for `target` and asserts the C ABI gate did not reject the
// by-value extern aggregate. The link step may still fail when the host lacks
// target startup files; that is unrelated to SAFE-FFI-003.
internal func expect_compile_target_no_diagnostic(env : &mut TestEnv, name : *char, ch_content : *char, unexpected_sub : *char, mod_content : *char, target : *char) {
    unsafe {
        setup_test_files(NEG_WORK_DIR, name, mod_content, ch_content)
        var mod_path : char[512]
        unsafe { sprintf(unsafe(&raw mut mod_path[0]), "%s/%s/chemical.mod", NEG_WORK_DIR, name) }
        var out_path : char[512]
        unsafe { sprintf(unsafe(&raw mut out_path[0]), "%s/%s/out.exe", NEG_WORK_DIR, name) }
        var extra : char[256]
        unsafe { sprintf(unsafe(&raw mut extra[0]), " --target %s", target) }
        var output_buf : char[65536]
        run_compiler_capture(unsafe(&raw mod_path[0]), unsafe(&raw out_path[0]), unsafe(&raw mut output_buf[0]), 65536, "", unsafe(&raw extra[0]))
        if(string_contains(unsafe(&raw output_buf[0]), unexpected_sub)) {
            env.error("compiler rejected a supported target ABI")
            neg_debug_print(name, unsafe(&raw output_buf[0]))
        } else {
            env.success("target ABI was accepted")
        }
        cleanup_test_dir(NEG_WORK_DIR, name)
    }
}

internal func expect_check_target_no_diagnostic(env : &mut TestEnv, name : *char, ch_content : *char, unexpected_sub : *char, mod_content : *char, target : *char) {
    unsafe {
        setup_test_files(NEG_WORK_DIR, name, mod_content, ch_content)
        var mod_path : char[512]
        unsafe { sprintf(unsafe(&raw mut mod_path[0]), "%s/%s/chemical.mod", NEG_WORK_DIR, name) }
        var out_path : char[512]
        unsafe { sprintf(unsafe(&raw mut out_path[0]), "%s/%s/out.exe", NEG_WORK_DIR, name) }
        var extra : char[256]
        unsafe { sprintf(unsafe(&raw mut extra[0]), " --target %s --check", target) }
        var output_buf : char[65536]
        var rc = run_compiler_capture(unsafe(&raw mod_path[0]), unsafe(&raw out_path[0]), unsafe(&raw mut output_buf[0]), 65536, "", unsafe(&raw extra[0]))
        if(rc != 0) {
            env.error("target safety check failed to compile")
            neg_debug_print(name, unsafe(&raw output_buf[0]))
        } else if(string_contains(unsafe(&raw output_buf[0]), unexpected_sub)) {
            env.error("compiler rejected a supported target ABI")
            neg_debug_print(name, unsafe(&raw output_buf[0]))
        } else {
            env.success("target safety check was accepted")
        }
        cleanup_test_dir(NEG_WORK_DIR, name)
    }
}

internal func expect_compile_success_with_mod(env : &mut TestEnv, name : *char, ch_content : *char, mod_content : *char) {
        unsafe {
        setup_test_files(NEG_WORK_DIR, name, mod_content, ch_content)

        var mod_path : char[512]
        unsafe { sprintf(unsafe(&raw mut mod_path[0]), "%s/%s/chemical.mod", NEG_WORK_DIR, name) }
        var out_path : char[512]
        unsafe { sprintf(unsafe(&raw mut out_path[0]), "%s/%s/out.exe", NEG_WORK_DIR, name) }

        var output_buf : char[65536]
        var rc = run_compiler_capture(unsafe(&raw mod_path[0]), unsafe(&raw out_path[0]), unsafe(&raw mut output_buf[0]), 65536)

        if(rc != 0) {
            env.error("expected compiler to succeed but it failed")
            neg_debug_print(name, unsafe(&raw output_buf[0]))
        }

        cleanup_test_dir(NEG_WORK_DIR, name)

        }
    }

// Compiles an application module and runs it, checking the process exit code.
// Used to exercise the `async func main` trampoline end-to-end.
internal func expect_compile_and_exit(env : &mut TestEnv, name : *char, ch_content : *char, mod_content : *char, expected_exit : int) {
    expect_compile_and_exit_mode(env, name, ch_content, mod_content, expected_exit, "")
}

// Like expect_compile_and_exit, but passes `--mode <mode>` to the compiler.
// Used to compile with debug info (`debug_complete`) so the backend verifier
// runs on the generated module.
internal func expect_compile_and_exit_mode(env : &mut TestEnv, name : *char, ch_content : *char, mod_content : *char, expected_exit : int, mode : *char) {
        unsafe {
        setup_test_files(NEG_WORK_DIR, name, mod_content, ch_content)

        var mod_path : char[512]
        unsafe { sprintf(unsafe(&raw mut mod_path[0]), "%s/%s/chemical.mod", NEG_WORK_DIR, name) }
        var out_path : char[512]
        unsafe { sprintf(unsafe(&raw mut out_path[0]), "%s/%s/out.exe", NEG_WORK_DIR, name) }

        var output_buf : char[65536]
        var rc = run_compiler_capture(unsafe(&raw mod_path[0]), unsafe(&raw out_path[0]), unsafe(&raw mut output_buf[0]), 16384, mode)
        if(rc != 0) {
            env.error("expected compiler to succeed but it failed")
            neg_debug_print(name, unsafe(&raw output_buf[0]))
            cleanup_test_dir(NEG_WORK_DIR, name)
            return
        }

        var status = unsafe(system(unsafe(&raw out_path[0])))
        var exit_code : int
        comptime if(def.windows) {
            exit_code = status
        } else {
            exit_code = (status >> 8) & 0xff
        }
        if(exit_code != expected_exit) {
            env.error("the program returned the wrong exit code")
        }
        cleanup_test_dir(NEG_WORK_DIR, name)

        }
    }

@test
func neg_temp_to_view_with_dtor_errors(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct MyView 'a {\n    var data : *char\n}\nstruct MyObj {\n    func get_view(&self) : 'self MyView {\n        return MyView { data : null }\n    }\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var v = MyObj().get_view()\n}\n"
    expect_compile_error(env, "temp_to_view_with_dtor_errors", ch, "lifetime dependency")
}

@test
func neg_named_to_view_succeeds(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct MyView 'a {\n    var data : *char\n}\nstruct MyObj {\n    func get_view(&self) : 'self MyView {\n        return MyView { data : null }\n    }\n    @delete\n    func delete(&mut self) { }\n}\npublic func main() : int {\n    var obj = MyObj()\n    var v = obj.get_view()\n    return 0\n}\n"
    expect_compile_success(env, "named_to_view_succeeds", ch)
}

@test
func neg_temp_no_lifetime_type_succeeds(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct MyObj {\n    func get_value(&self) : i32 {\n        return 42\n    }\n    @delete\n    func delete(&mut self) { }\n}\npublic func main() : int {\n    var v = MyObj().get_value()\n    return 0\n}\n"
    expect_compile_success(env, "temp_no_lifetime_type_succeeds", ch)
}

@test
func neg_temp_no_dtor_succeeds(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct MyView 'a {\n    var data : *char\n}\nstruct MyObj {\n    func get_view(&self) : 'self MyView {\n        return MyView { data : null }\n    }\n}\npublic func main() : int {\n    var v = MyObj().get_view()\n    return 0\n}\n"
    expect_compile_success(env, "temp_no_dtor_succeeds", ch)
}

@test
func neg_temp_no_return_lifetime_succeeds(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct MyView 'a {\n    var data : *char\n}\nfunc get_view(v : MyView) : MyView {\n    return v\n}\npublic func main() : int {\n    var v = get_view(MyView { data : null })\n    return 0\n}\n"
    expect_compile_success(env, "temp_no_return_lifetime_succeeds", ch)
}

@test
func neg_lambda_param_unresolved_child_prints_error(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
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
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func get(&self, i : int) : T where T : Copy {\n        return unsafe(*data)\n    }\n}\nstruct Row {\n    var p : *char\n    @delete\n    func delete(&mut self) { }\n}\npublic func main() : int {\n    var h = Holder<Row> { data = null }\n    var r = h.get(0)\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "get_on_destructible_struct_direct", ch, "does not satisfy where clause constraint", NEG_MOD_CORE)
}

// A user-defined type without a destructor is still not Copy unless it opts in
// with `impl Copy`. The old behavior treated every destructor-less type as
// implicitly Copy.
@test
func neg_copy_requires_explicit_impl(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func get(&self, i : int) : T where T : Copy {\n        return unsafe(*data)\n    }\n}\nstruct Row {\n    var p : *char\n}\npublic func main() : int {\n    var h = Holder<Row> { data = null }\n    var r = h.get(0)\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "copy_requires_explicit_impl", ch, "SAFE-COPY-001", NEG_MOD_CORE)
}

@test
func copy_explicit_impl_satisfies_bound(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func get(&self, i : int) : T where T : Copy {\n        return unsafe(*data)\n    }\n}\nstruct Row {\n    var p : *char\n}\nimpl Copy for Row {}\npublic func main() : int {\n    var h = Holder<Row> { data = null }\n    var r = h.get(0)\n    return 0\n}\n"
    expect_compile_output_not_contains(env, "copy_explicit_impl", ch, "SAFE-COPY-001", NEG_MOD_CORE)
}

@test
func copy_impl_with_destructor_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Bad {\n    var s : *char\n    @delete\n    func delete(&mut self) { }\n}\nimpl Copy for Bad {}\npublic func main() : int { return 0 }\n"
    expect_compile_error_with_mod(env, "copy_impl_destructor", ch, "SAFE-COPY-002", NEG_MOD_CORE)
}

@test
func copy_impl_with_non_copy_field_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Row {\n    var x : int\n}\nstruct Outer {\n    var row : Row\n}\nimpl Copy for Outer {}\npublic func main() : int { return 0 }\n"
    expect_compile_error_with_mod(env, "copy_impl_non_copy_field", ch, "SAFE-COPY-003", NEG_MOD_CORE)
}

@test
func copy_impl_with_copy_fields_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Row {\n    var x : int\n}\nimpl Copy for Row {}\nstruct Outer {\n    var row : Row\n    var values : [4]int\n}\nimpl Copy for Outer {}\npublic func main() : int { return 0 }\n"
    expect_compile_output_not_contains(env, "copy_impl_valid", ch, "SAFE-COPY", NEG_MOD_CORE)
}

// Same as above but the receiver of .get is reached through a member access
// (outer.h.get(0)), which is exactly how `qr.rows.get(0)` triggered the bug.
@test
func neg_get_on_destructible_struct_nested_receiver_errors(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func get(&self, i : int) : T where T : Copy {\n        return unsafe(*data)\n    }\n}\nstruct Row {\n    var p : *char\n    @delete\n    func delete(&mut self) { }\n}\nstruct Outer {\n    var h : Holder<Row>\n}\npublic func main() : int {\n    var outer = Outer { h = Holder<Row> { data = null } }\n    var r = outer.h.get(0)\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "get_on_destructible_struct_nested", ch, "does not satisfy where clause constraint", NEG_MOD_CORE)
}

@test
public func neg_universal_missing_prop_names_the_component(env : &mut TestEnv) {
    var ch = "#universal Good(props : title) {\n    return <span>{props.title}</span>\n}\n#universal Host(props) {\n    return <Good />\n}\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "universal_missing_prop", ch, "missing required prop 'title' on <Good>", NEG_MOD_UNIVERSAL)
}

@test
public func neg_html_unsupported_value_type_is_diagnosed(env : &mut TestEnv) {
    // A struct with no `writeToPageHtml` cannot be embedded in `#html`; it used
    // to write an error string into the page silently. Replacement hooks now
    // report it through the live ASTDiagnoser.
    var ch = "struct Blob {\n    var x : int\n}\npublic func main() : int {\n    var page = HtmlPage()\n    var blob = Blob { x = 1 }\n    #html { <span>{blob}</span> }\n    return 0\n}\n"
    expect_compile_output_contains(env, "html_unsupported_value", ch, "has no HTML serialization", NEG_MOD_HTML)
}

@test
public func neg_universal_unsupported_attribute_value_is_diagnosed(env : &mut TestEnv) {
    // A pointer (non-char) attribute value has no SsrAttributeValue
    // representation; it used to be silently emitted as a number. The converter
    // now reports it through the live ASTDiagnoser.
    var ch = "func unsupported_ptr(page : &mut HtmlPage) : *mut int {\n    return null\n}\n#universal PtrAttr(props) {\n    return <div data-p={${unsupported_ptr(page)}}>x</div>\n}\npublic func main() : int {\n    var page = HtmlPage()\n    #html { <PtrAttr /> }\n    return 0\n}\n"
    expect_compile_output_contains(env, "universal_unsupported_attr", ch, "only char pointers have an SSR representation", NEG_MOD_UNIVERSAL_HTML)
}

@test
public func neg_universal_unknown_component_names_the_symbol(env : &mut TestEnv) {
    var ch = "#universal Host(props) {\n    return <NoSuchComponent />\n}\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "universal_unknown_component", ch, "is not a known component", NEG_MOD_UNIVERSAL)
}

public func main(argc : int, argv : **char) {
    neg_init_work_dir()
    test_runner(argc, argv)
}
@test
func neg_tuple_element_type_mismatch(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var t : (int, bool) = (1, 2)\n    return 0\n}\n"
    expect_compile_error(env, "tuple_element_type_mismatch", ch, "does not satisfy type")
}

@test
func neg_tuple_pattern_arity_mismatch(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var (a, b) = (1, 2, 3)\n    return a\n}\n"
    expect_compile_error(env, "tuple_pattern_arity_mismatch", ch,
                         "tuple pattern has 2 names but the value has 3 elements")
}

@test
func neg_nested_array_pattern_arity(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var values : [2][2]int = []\n    var [[a, b], [c]] = values\n    return 0\n}\n"
    expect_compile_error(env, "nested_array_pattern_arity", ch,
                         "array pattern arity does not match at a nested level")
}

@test
func neg_nested_array_pattern_requires_array(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var values : [2]int = []\n    var [[a, b], c] = values\n    return 0\n}\n"
    expect_compile_error(env, "nested_array_pattern_requires_array", ch,
                         "nested array pattern element is not a fixed-size array")
}

@test
func neg_nested_tuple_pattern_arity(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var ((a, b), c) = ((1, 2, 3), 4)\n    return 0\n}\n"
    expect_compile_error(env, "nested_tuple_pattern_arity", ch,
                         "tuple pattern arity does not match at a nested level")
}

@test
func neg_nested_tuple_pattern_requires_tuple(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var (a, (b, c)) = (1, 2)\n    return 0\n}\n"
    expect_compile_error(env, "nested_tuple_pattern_requires_tuple", ch,
                         "nested tuple pattern element is not a tuple")
}

@test
func neg_tuple_pattern_requires_tuple(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var (a, b) = 1\n    return a + b\n}\n"
    expect_compile_error(env, "tuple_pattern_requires_tuple", ch,
                         "tuple destructuring requires a tuple value")
}

@test
func neg_tuple_pattern_const_binding_assign(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var pair = (1, 2)\n    const (a, b) = pair\n    a = 5\n    return a + b\n}\n"
    expect_compile_error(env, "tuple_pattern_const_binding_assign", ch,
                         "cannot assign to a non mutable value")
}

@test
func neg_tuple_struct_constructor_arity(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Point(int, int)\npublic func main() : int {\n    var point = Point(1)\n    return point._0\n}\n"
    expect_compile_error(env, "tuple_struct_constructor_arity", ch,
                         "doesn't have a constructor")
}

@test
func neg_array_pattern_arity_mismatch(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var values : [3]int = [1, 2, 3]\n    var [a, b] = values\n    return a + b\n}\n"
    expect_compile_error(env, "array_pattern_arity_mismatch", ch,
                         "array pattern has 2 names but the value has 3 elements")
}

@test
func array_pattern_non_copy_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var values : [2]std::string = [std::string(\"a\"), std::string(\"b\")]\n    var [a, b] = values\n    return (a.size() + b.size()) as int\n}\n"
    expect_compile_success_with_mod(env, "array_pattern_non_copy", ch, NEG_MOD_STD_CORE)
}

@test
func neg_array_pattern_non_copy_const(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct DT {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\npublic func main() : int {\n    const values : [2]DT = [DT { x : 1 }, DT { x : 2 }]\n    var [a, b] = values\n    return a.x + b.x\n}\n"
    expect_compile_error(env, "array_pattern_non_copy_const", ch, "SAFE-MOVE-004")
}

@test
func neg_array_pattern_non_copy_parameter(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct DT {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\nfunc take(values : [2]DT) : int {\n    var [a, b] = values\n    return a.x + b.x\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "array_pattern_non_copy_parameter", ch, "SAFE-MOVE-004")
}

@test
func neg_array_pattern_non_copy_nested(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct DT {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\npublic func main() : int {\n    var values : [2][2]DT = []\n    values[0][0] = DT { x : 1 }\n    values[0][1] = DT { x : 2 }\n    values[1][0] = DT { x : 3 }\n    values[1][1] = DT { x : 4 }\n    var [[a, b], [c, d]] = values\n    return a.x + b.x + c.x + d.x\n}\n"
    expect_compile_error(env, "array_pattern_non_copy_nested", ch, "SAFE-MOVE-004")
}

@test
func slice_rest_pattern_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var values : [3]int = [1, 2, 3]\n    var view : [int] = values\n    if(var [a, .., z] = view) { return a + z }\n    return 1\n}\n"
    expect_compile_success_with_mod(env, "slice_rest_pattern_compiles", ch, NEG_MOD_STD_CORE)
}

@test
func associated_type_binding_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "interface HasItem {\n    type Item\n    func value(&self) : int\n}\nstruct HasItemImpl {\n    var n : int\n}\nimpl HasItem for HasItemImpl {\n    type Item = int\n    func value(&self) : int { return self.n }\n}\npublic func main() : int { return 0 }\n"
    expect_compile_success(env, "associated_type_binding_compiles", ch)
}

@test
func neg_associated_type_missing_binding(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "interface HasItem {\n    type Item\n}\nstruct HasItemImpl { var n : int }\nimpl HasItem for HasItemImpl { }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "associated_type_missing_binding", ch,
                         "type does not implement associated type 'Item'")
}

@test
func associated_type_projection_return_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "interface HasItem {\n    type Item\n    func value(&self) : Self::Item\n}\nstruct Counter { var n : int }\nimpl HasItem for Counter {\n    type Item = int\n    func value(&self) : int { return self.n }\n}\npublic func main() : int { return 0 }\n"
    expect_compile_success(env, "associated_type_projection_return_compiles", ch)
}

@test
func associated_type_projection_parameter_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "interface HasItem {\n    type Item\n    func set(&mut self, value : Self::Item)\n}\nstruct Counter { var n : int }\nimpl HasItem for Counter {\n    type Item = int\n    func set(&mut self, value : int) { self.n = value }\n}\npublic func main() : int { return 0 }\n"
    expect_compile_success(env, "associated_type_projection_parameter_compiles", ch)
}

@test
func associated_type_bound_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "interface HasCopyItem {\n    type Item: Copy\n    func get(&self) : Self::Item\n}\nstruct Good { var n : int }\nimpl HasCopyItem for Good {\n    type Item = int\n    func get(&self) : int { return self.n }\n}\npublic func main() : int { return 0 }\n"
    expect_compile_success_with_mod(env, "associated_type_bound_compiles", ch, NEG_MOD_STD_CORE)
}

@test
func neg_associated_type_bound_rejects_noncopy(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "interface HasCopyItem {\n    type Item: Copy\n    func get(&self) : Self::Item\n}\nstruct Bad { var s : std::string }\nimpl HasCopyItem for Bad {\n    type Item = std::string\n    func get(&self) : std::string { return self.s }\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error_with_mod(env, "associated_type_bound_rejects_noncopy", ch,
                                  "associated type 'Item' binding does not satisfy bound 'Copy'",
                                  NEG_MOD_STD_CORE)
}

@test
func neg_associated_equality_inline_mismatch(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "interface HasItem {\n    type Item\n    func get(&self) : Self::Item\n}\nstruct Counter { var n : int }\nimpl HasItem for Counter {\n    type Item = int\n    func get(&self) : int { return self.n }\n}\nfunc <T : HasItem<Item = long>> read_item(t : &T) : T::Item { return t.get() }\npublic func main() : int {\n    var c = Counter { n : 7 }\n    return read_item(&c) as int\n}\n"
    expect_compile_error(env, "associated_equality_inline_mismatch", ch,
                         "associated type 'Item' of 'Counter' is 'int', but the bound requires 'long'")
}

@test
func neg_associated_equality_where_mismatch(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "interface HasItem {\n    type Item\n    func get(&self) : Self::Item\n}\nstruct Counter { var n : int }\nimpl HasItem for Counter {\n    type Item = int\n    func get(&self) : int { return self.n }\n}\nfunc <T> read_item(t : &T) : T::Item where T : HasItem<Item = long> { return t.get() }\npublic func main() : int {\n    var c = Counter { n : 7 }\n    return read_item(&c) as int\n}\n"
    expect_compile_error(env, "associated_equality_where_mismatch", ch,
                         "associated type 'Item' of 'Counter' is 'int', but the bound requires 'long'")
}

@test
func neg_associated_equality_unknown_name(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "interface HasItem {\n    type Item\n    func get(&self) : Self::Item\n}\nfunc <T : HasItem<Nope = int>> bad(t : &T) { }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "associated_equality_unknown_name", ch,
                         "interface 'HasItem' has no associated type 'Nope'")
}

@test
func neg_associated_equality_requires_interface(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Box<T> { var value : T }\nfunc <T : Box<Item = int>> bad(t : &T) { }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "associated_equality_requires_interface", ch,
                         "associated-type equality constraints are only valid on interface bounds")
}

@test
func neg_associated_projection_without_bound(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func <T> bad(t : &T) : T::Item { return t.n }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "associated_projection_without_bound", ch,
                         "unresolved identifier, 'Item' not found")
}

@test
func neg_associated_projection_unknown_member(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "interface HasItem {\n    type Item\n    func get(&self) : Self::Item\n}\nfunc <T : HasItem> bad(t : &T) : T::Nope { return t.get() }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "associated_projection_unknown_member", ch,
                         "unresolved identifier, 'Nope' not found")
}

@test
func neg_associated_type_projection_default_parameter(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "interface HasItem {\n    type Item\n    func set(&mut self, value : Self::Item) { }\n}\nstruct Counter { var n : int }\nimpl HasItem for Counter {\n    type Item = int\n    func set(&mut self, value : int) { self.n = value }\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "associated_type_projection_default_parameter", ch,
                         "associated type projections in parameter position are not implemented yet in default interface methods")
}

@test
func neg_dyn_associated_type_interface_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "interface HasItem {\n    type Item\n    func get(&self) : Self::Item\n}\nstruct Counter { var n : int }\nimpl HasItem for Counter {\n    type Item = int\n    func get(&self) : int { return self.n }\n}\npublic func main() : int {\n    var c = Counter { n : 7 }\n    var d : dyn HasItem = dyn<HasItem>(c)\n    return 0\n}\n"
    expect_compile_error(env, "dyn_associated_type_interface", ch,
                         "is not object safe")
}

@test
func neg_dyn_inherited_associated_type_interface_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "interface HasItem {\n    type Item\n    func get(&self) : Self::Item\n}\ninterface Child : HasItem {\n    func tag(&self) : int\n}\nstruct Counter { var n : int }\nimpl HasItem for Counter {\n    type Item = int\n    func get(&self) : int { return self.n }\n}\nimpl Child for Counter {\n    func tag(&self) : int { return 1 }\n}\npublic func main() : int {\n    var c = Counter { n : 7 }\n    var d : dyn Child = dyn<Child>(c)\n    return 0\n}\n"
    expect_compile_error(env, "dyn_inherited_associated_type_interface", ch,
                         "is not object safe")
}
