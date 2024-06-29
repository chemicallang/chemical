import "../test.ch"

struct Pair66 {

    var a : int
    var b : int

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
    test("determine string length, using comptime", () => {
        return determine_str_len("hello") == 5;
    })
}