import "../test.ch"

func <T = int, K = int, R = int> gen_sum(a : T, b : K) : R {
    return a + b;
}

func is_this_60(thing : long) : bool {
    return thing == 60;
}

struct PairGen <T, U, V> {
    var a : T
    var b : U
    func add(&self) : V {
        return a + b
    }
}

func test_basic_generics() {
    test("test that basic generic function with no generic args works", () => {
        return gen_sum(10, 20) == 30;
    })
    test("test that basic generic function with generic args works", () => {
        return gen_sum<long, long, long>(20, 20) == 40;
    })
    test("test that generic functions can be called inside other calls", () => {
        return is_this_60(gen_sum<long, long, long>(30, 30));
    })
    test("test that generic functions result can be saved into variables", () => {
        var i = gen_sum<long, long, long>(30, 40);
        return i == 70;
    })
    test("test that generic struct works - 1", () => {
        var p = PairGen <int, int, int> { a : 10, b : 12 }
        return p.add() == 22;
    })
    test("test that generic struct works - 2", () => {
        var p = PairGen <long, long, long> { a : 20, b : 15 }
        return p.add() == 35
    })
}