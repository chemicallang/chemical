import "../test.ch"

struct Pair66 {

    var a : int
    var b : int

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
}