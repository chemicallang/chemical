import "../test.ch"

struct Pair66 {

    var a : int
    var b : int

    @comptime
    @constructor
    func check(value : bool) {
        if(value){
            return compiler::wrap(first())
        } else {
            return compiler::wrap(second())
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
    func pair1(d : literal::int) {
        return Pair66 {
            a : d / 2
            b : d / 2
        }
    }

    @constructor
    func first() {
        return Pair {
            a : 15,
            b : 15
        }
    }

    @constructor
    func second() {
        return Pair {
            a : 20,
            b : 20
        }
    }

    func sum(&self) : int {
        return a + b;
    }

}

@comptime
func comptime_sum(a : int, b : int) {
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
func determine_str_len(str : literal::string) {
    return compiler::strlen(str);
}

func runtime_sum(a : int, b : int) : int {
    return a + b;
}

@comptime
func sum_multiple(x : int) : int {
    return compiler::wrap(runtime_sum(x * 2, x * 2));
}

func ret_struct_boi() : Pair66 {
    var p = compiler::return_struct() as Pair66*
    p.a = 343
    p.b = 979
    return;
}

func test_comptime() {
    test("test comptime sum works", () => {
        return comptime_sum(3, 6) == 9;
    })
    test("test comptime function can return struct", () => {
        var pair = pair_66();
        return pair.a == 33 && pair.b == 11;
    })
    test("test comptime function can return struct from a reference", () => {
        var pair = pair_66_ref();
        return pair.a == 33 && pair.b == 11;
    })
    test("test comptime function can call struct functions", () => {
        return call_struct_func() == 44;
    })
    test("test comptime constructor function can be called", () => {
        var p = Pair66();
        return p.a == 10 && p.b == 10;
    })
    test("test that appropriate comptime constructor function is selected", () => {
        var p = Pair66(10);
        return p.a == 5 && p.b == 5;
    })
    test("test comptime constructor can delegate to actual constructor - 1", () => {
        var p = Pair66(true)
        return p.a == 15 && p.b == 15;
    })
    test("test comptime constructor can delegate to actual constructor - 2", () => {
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
}