@direct_init
struct Pair66 {

    var a : int
    var b : int

    @implicit
    @comptime
    @constructor
    func check(value : bool) {
        if(value){
            return intrinsics::wrap(first())
        } else {
            return intrinsics::wrap(second())
        }
    }

    @comptime
    @constructor
    func pair2() {
        return Pair66 {
            a : 10,
            b : 10
        }
    }

    @comptime
    @constructor
    func pair1(d : literal<int>) {
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

@comptime
func give_me_some_sum() : (a : int, b : int) => int {
    return intrinsics::get_child_fn(CTStructGetChild, "fake_sum") as (a : int, b : int) => int;
}

@comptime
func comptime_primitive() : int {
    return 10;
}

@comptime
func comptime_sum(a : int, b : int) : int {
    return a + b;
}

@comptime
func pair_66() : Pair66 {
    return Pair66 {
        a : 33,
        b : 11
    }
}

@comptime
func pair_66_ref() : Pair66 {
    var x = pair_66();
    return x;
}

@comptime
func call_struct_func() : int {
    var x = pair_66();
    return x.sum();
}

@comptime
func determine_str_len(str : literal<string>) : ubigint {
    return intrinsics::size(str);
}

func runtime_sum(a : int, b : int) : int {
    return a + b;
}

@comptime
func return_runtime_sum() : (a : int, b : int) => int {
    return runtime_sum;
}

@comptime
func sum_multiple(x : int) : int {
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
    @comptime
    @constructor
    func constructor(thing : *mut int) {
        return intrinsics::wrap(actual(thing, 1));
    }
    @constructor
    func actual(thing : *mut int, inc : int) {
        *thing = *thing + inc;
    }
}

@comptime
func get_line_no() : ubigint {

}

@comptime
func give_caller_line_no() : ubigint {
    return intrinsics::get_caller_line_no();
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
    test("comptime functions can return child of a struct", () => {
        var sum_fn = give_me_some_sum();
        return sum_fn(9, 4) == 73;
    })
    test("comptime functions can return child of a struct", () => {
        return give_me_some_sum()(9, 3) == 72;
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
}