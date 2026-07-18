const ANSI_COLOR_RESET = "\x1b[0m"
const ANSI_COLOR_RED = "\x1b[31m"
const ANSI_COLOR_GREEN = "\x1b[32m"

var total_tests = 0;
var tests_passed = 0;
var tests_failed = 0;

func test(name : *char, passed : bool) {
    if(passed) {
        tests_passed++;
        printf("%sTest %d [%s] succeeded%s\n", ANSI_COLOR_GREEN, total_tests + 1, name, ANSI_COLOR_RESET);
    } else {
        tests_failed++;
        printf("%sTest %d [%s] failed%s\n", ANSI_COLOR_RED, total_tests + 1, name, ANSI_COLOR_RESET);
    }
    total_tests++;
}

func print_test_stats() {
    var failed_color = if(tests_failed == 0) ANSI_COLOR_GREEN else ANSI_COLOR_RED
    printf("Total %d %sPassed %d%s %sFailed %d%s\n", total_tests, ANSI_COLOR_GREEN, tests_passed, ANSI_COLOR_RESET, failed_color, tests_failed, ANSI_COLOR_RESET);
}

func write_file(path : *char, content : *char) {
    var f = fopen(path, "w")
    if(f != null) {
        fwrite(content as *void, strlen(content), 1, f)
        fclose(f)
    }
}

func string_contains(haystack : *char, needle : *char) : bool {
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

func run_compiler_on(mod_path : *char, output_buf : *mut char, buf_size : int) : int {
    var cmd : char[2048]
    sprintf(&raw mut cmd[0], "%s \"%s\" --no-cache 2>&1", intrinsics::get_compiler_path(), mod_path)
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

func run_single_negative_test(work_dir : *char, name : *char, mod_content : *char, ch_content : *char, expect_error : bool, expected_sub : *char) : bool {
    var test_dir : char[512]
    sprintf(&raw mut test_dir[0], "%s/%s", work_dir, name)
    mkdir(&raw test_dir[0], 0o777 as uint)

    var mod_path : char[512]
    sprintf(&raw mut mod_path[0], "%s/chemical.mod", &raw test_dir[0])
    write_file(&raw mod_path[0], mod_content)

    var ch_path : char[512]
    sprintf(&raw mut ch_path[0], "%s/test.ch", &raw test_dir[0])
    write_file(&raw ch_path[0], ch_content)

    var output_buf : char[16384]
    var rc = run_compiler_on(&raw mod_path[0], &raw mut output_buf[0], 16384)

    var test_pass : bool
    if(expect_error) {
        var has_error = string_contains(&raw output_buf[0], "[TypeCheck] error")
        var has_sub = if(strlen(expected_sub) == 0) true else string_contains(&raw output_buf[0], expected_sub)
        test_pass = (rc != 0) && has_error && has_sub
    } else {
        test_pass = (rc == 0)
    }

    if(!test_pass) {
        printf("  Output was:\n%s\n", &raw output_buf[0])
    }

    var rm_cmd : char[512]
    sprintf(&raw mut rm_cmd[0], "rm -rf \"%s\"", &raw test_dir[0])
    system(&raw rm_cmd[0])

    return test_pass
}

public func main() {
    printf("Negative Lifetime Tests\n")
    printf("=======================\n\n")

    mkdir("/tmp/chemical_neg_tests", 0o777 as uint)

    var mod = "module neg_test\nsource \".\"\n"

    var ch1 = "struct MyView 'a {\n    var data : *char\n}\nstruct MyObj {\n    func get_view(&self) : 'self MyView {\n        return MyView { data : null }\n    }\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var v = MyObj().get_view()\n}\n"
    test("temp_to_view_with_dtor_errors", run_single_negative_test("/tmp/chemical_neg_tests", "temp_to_view_with_dtor_errors", mod, ch1, true, "lifetime dependency"))

    var ch2 = "struct MyView 'a {\n    var data : *char\n}\nstruct MyObj {\n    func get_view(&self) : 'self MyView {\n        return MyView { data : null }\n    }\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var obj = MyObj()\n    var v = obj.get_view()\n}\n"
    test("named_to_view_succeeds", run_single_negative_test("/tmp/chemical_neg_tests", "named_to_view_succeeds", mod, ch2, false, ""))

    var ch3 = "struct MyObj {\n    func get_value(&self) : i32 {\n        return 42\n    }\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var v = MyObj().get_value()\n}\n"
    test("temp_no_lifetime_type_succeeds", run_single_negative_test("/tmp/chemical_neg_tests", "temp_no_lifetime_type_succeeds", mod, ch3, false, ""))

    var ch4 = "struct MyView 'a {\n    var data : *char\n}\nstruct MyObj {\n    func get_view(&self) : 'self MyView {\n        return MyView { data : null }\n    }\n}\nfunc main() {\n    var v = MyObj().get_view()\n}\n"
    test("temp_no_dtor_succeeds", run_single_negative_test("/tmp/chemical_neg_tests", "temp_no_dtor_succeeds", mod, ch4, false, ""))

    var ch5 = "struct MyView 'a {\n    var data : *char\n}\nfunc get_view(v : MyView) : MyView {\n    return v\n}\nfunc main() {\n    var v = get_view(MyView { data : null })\n}\n"
    test("temp_no_return_lifetime_succeeds", run_single_negative_test("/tmp/chemical_neg_tests", "temp_no_return_lifetime_succeeds", mod, ch5, false, ""))

    print_test_stats()

    system("rm -rf /tmp/chemical_neg_tests")
}
