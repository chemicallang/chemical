@direct_init
struct Pair66 {

    var a : int
    var b : int

    @implicit
    @constructor
    comptime func check(value : bool) {
        if(value){
            return intrinsics::wrap(first())
        } else {
            return intrinsics::wrap(second())
        }
    }

    @constructor
    comptime func pair2() {
        return Pair66 {
            a : 10,
            b : 10
        }
    }

    @constructor
    comptime func pair1(d : %literal<int>) {
        return Pair66 {
            a : d / 2
            b : d / 2
        }
    }

    @constructor
    func first() {
        init {
            a(15)
            b(15)
        }
    }

    @constructor
    func second() {
        init {
            a(20)
            b(20)
        }
    }

    func sum(&self) : int {
        return a + b;
    }

}

struct CTStructGetChild {

    func fake_sum(a : int, b : int) : int {
        return a + b + 60;
    }

}

comptime func give_me_some_sum() : (a : int, b : int) => int {
    return intrinsics::get_child_fn<CTStructGetChild>("fake_sum") as (a : int, b : int) => int;
}

comptime func <T> give_me_gen_sum() : (a : int, b : int) => int {
    return intrinsics::get_child_fn<T>("fake_sum") as (a : int, b : int) => int;
}

comptime func comptime_primitive() : int {
    return 10;
}

comptime func comptime_sum(a : int, b : int) : int {
    return a + b;
}

comptime func pair_66() : Pair66 {
    return Pair66 {
        a : 33,
        b : 11
    }
}

comptime func pair_66_ref() : Pair66 {
    var x = pair_66();
    return x;
}

comptime func call_struct_func() : int {
    var x = pair_66();
    return x.sum();
}

comptime func determine_str_len(str : %literal_string) : ubigint {
    return intrinsics::size(str);
}

func runtime_sum(a : int, b : int) : int {
    return a + b;
}

comptime func return_runtime_sum() : (a : int, b : int) => int {
    return runtime_sum;
}

comptime func sum_multiple(x : int) : int {
    return intrinsics::wrap(runtime_sum(x * 2, x * 2)) as int;
}

// TODO should this be allowed
func ret_struct_boi() : Pair66 {
    const p = intrinsics::return_struct() as *mut Pair66
    p.a = 343
    p.b = 979
    return;
}

func ret_struct_comptime() : Pair66 {
    return Pair66(true)
}

func ret_struct_comptime_b() : Pair66 {
    return Pair66(false)
}

func ret_struct_implicit() : Pair66 {
    return true;
}

func ret_struct_implicit_b() : Pair66 {
    return false;
}

struct CompTimeCounter {
    @constructor
    comptime func constructor(thing : *mut int) {
        return intrinsics::wrap(actual(thing, 1));
    }
    @constructor
    func actual(thing : *mut int, inc : int) {
        *thing = *thing + inc;
    }
}

comptime func get_line_no() : ubigint {

}

comptime func give_caller_line_no() : ubigint {
    return intrinsics::get_caller_line_no();
}

struct comptime_func_container {
    comptime func give_arg(&self, arg : int) : int {
        return arg;
    }
}

func (container : &comptime_func_container) give_arg2(arg : int) : int {
    return arg;
}

comptime func comptime_test_comparison_strings(value1 : *char, value2 : *char) : bool {
    return value1 == value2;
}

comptime func comptime_test_comparison_strings2() : bool {
    return "something34" == "something34";
}

comptime func comptime_test_comparison_strings3() : bool {
    return "sdfgsdfsd3234" == "fgdgdf234234234";
}

comptime func comptime_test_comparison_strings4() : bool {
    return "sdfgsdfsd3234" != "fgdgdf234234234";
}

comptime func comptime_test_comparison_strings5() : bool {
    return "abc123asdf" != "abc123asdf";
}

comptime func comptime_add_strings() : *char {
    return "abcdef" + "ghijkl"
}

comptime func check_addition_strings() : bool {
    return comptime_add_strings() == "abcdefghijkl"
}

comptime func test_comptime_default_val(value : int = 39238) : bool {
    return value == 39238
}

comptime func test_mm_comptime_def_val() : bool {
    return test_comptime_default_val()
}

func test_comptime() {
    test("comptime sum works", () => {
        return comptime_sum(3, 6) == 9;
    })
    test("comptime function can return struct", () => {
        var pair = pair_66();
        return pair.a == 33 && pair.b == 11;
    })
    test("comptime function can return struct from a reference", () => {
        var pair = pair_66_ref();
        return pair.a == 33 && pair.b == 11;
    })
    test("comptime function can call struct functions", () => {
        return call_struct_func() == 44;
    })
    test("comptime constructor function can be called", () => {
        var p = Pair66();
        return p.a == 10 && p.b == 10;
    })
    test("appropriate comptime constructor function is selected", () => {
        var p = Pair66(10);
        return p.a == 5 && p.b == 5;
    })
    test("comptime constructor can delegate to actual constructor - 1", () => {
        var p = Pair66(true)
        return p.a == 15 && p.b == 15;
    })
    test("comptime constructor can delegate to actual constructor - 2", () => {
        var p = Pair66(false)
        return p.a == 20 && p.b == 20;
    })
    test("determine string length, using comptime", () => {
        return determine_str_len("hello") == 5;
    })
    test("compiler wrap functionally works", () => {
        return sum_multiple(20) == 80;
    })
    test("can gain access to implicitly passed struct", ()=> {
        var p = ret_struct_boi()
        return p.a == 343 && p.b == 979;
    })
    test("comptime functions returning primitive work", () => {
        return comptime_primitive() == 10;
    })
    test("comptime functions returning primitive can be stored", () => {
        const prim = comptime_primitive();
        return prim == 10;
    })
    test("comptime delegated constructor get's called once", () => {
        var i = 0;
        var c = CompTimeCounter(&mut i);
        return i == 1;
    })
    test("comptime constructor initialized struct can be returned - 1", () => {
        const d = ret_struct_comptime();
        return d.a == 15 && d.b == 15;
    })
    test("comptime constructor initialized struct can be returned - 2", () => {
        const d = ret_struct_comptime_b();
        return d.a == 20 && d.b == 20;
    })
    test("comptime constructor can be implicit - 1", () => {
        const d = ret_struct_implicit();
        return d.a == 15 && d.b == 15;
    })
    test("comptime constructor can be implicit - 2", () => {
        const d = ret_struct_implicit_b();
        return d.a == 20 && d.b == 20;
    })
    test("compile time defined works - 1", () => {
        return defined("CHECK_DEF");
    })
    test("compile time defined works - 2", () => {
        return !defined("CHECK_DEF2");
    })
    test("compiler get target function works", () => {
        var t = intrinsics::get_target();
        return true;
    })
    test("get compiler line number", () => {
        var current = intrinsics::get_line_no();
        var lo = give_caller_line_no();
        return lo == current + 1;
    })
    test("comptime functions can return function pointers", () => {
        return return_runtime_sum()(10, 33) == 43;
    })
    test("comptime functions can return function pointers", () => {
        var sum_fn = return_runtime_sum();
        return sum_fn(66, 2) == 68;
    })
    test("comptime function get_child_fn can return child of a struct - 1", () => {
        var sum_fn = give_me_some_sum();
        return sum_fn(9, 4) == 73;
    })
    test("comptime function get_child_fn can return child of a struct - 2", () => {
        return give_me_some_sum()(9, 3) == 72;
    })
    test("comptime function get_child_fn can return child of a struct - 3", () => {
        var sum_fn = give_me_gen_sum<CTStructGetChild>();
        return sum_fn(9, 4) == 73;
    })
    test("comptime function get_child_fn can return child of a struct - 4", () => {
        return give_me_gen_sum<CTStructGetChild>()(9, 3) == 72;
    })
    test("module scope is as expected", () => {
        var scope_name = std::string_view(intrinsics::get_module_scope())
        var exp_scope_name = std::string_view("")
        return scope_name.equals(exp_scope_name)
    })
    test("module name is as expected", () => {
        var module_name = std::string_view(intrinsics::get_module_name())
        var exp_module_name = std::string_view("main")
        return module_name.equals(exp_module_name)
    })
    test("comptime member functions can return arguments", () => {
        var c = comptime_func_container {}
        return c.give_arg(33) == 33
    })
    test("comptime extension functions can return arguments", () => {
        var c = comptime_func_container {}
        return c.give_arg2(67) == 67
    })
    test("comptime can compare two strings for equality - 1", () => {
        return comptime_test_comparison_strings("something123", "something123")
    })
    test("comptime can compare two strings for equality - 2", () => {
        return comptime_test_comparison_strings2()
    })
    test("comptime can compare two strings for equality - 3", () => {
        return comptime_test_comparison_strings3() == false
    })
    test("comptime can compare two strings for equality - 4", () => {
        return comptime_test_comparison_strings4()
    })
    test("comptime can compare two strings for equality - 5", () => {
        return comptime_test_comparison_strings5() == false
    })
    test("comptime can add two strings together using plus", () => {
        return check_addition_strings()
    })
    test("default values in comptime functions work when called from runtime function", () => {
        return test_comptime_default_val()
    })
    test("default values in comptime functions work when called from comptime function", () => {
        return test_mm_comptime_def_val()
    })
}