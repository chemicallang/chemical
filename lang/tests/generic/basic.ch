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

func create_pair_gen() : PairGen<int, int, int> {
    return PairGen <int,int,int> { a : 12, b : 13 }
}

func create_pair_gen_long() : PairGen<long, long, long> {
    return PairGen <long, long, long> { a : 12, b : 14 }
}

func mul_int_pair(pair_gen : PairGen<int, int, int>) : int {
    return pair_gen.a * pair_gen.b;
}

struct ShortPairGen {
    var pair : PairGen<short, short, short>
    @constructor
    func make() {
        pair.a = 33;
        pair.b = 10;
    }
    func add(&self) : short {
        return pair.add();
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
    test("generic struct can be stored in another struct - 1", () => {
        var p = ShortPairGen()
        return p.add() == 43;
    })
    test("generic struct can be stored in another struct - 2", () => {
        var p = ShortPairGen {
            pair : PairGen<short, short, short> {
                a : 20,
                b : 41
            }
        }
        return p.add() == 61 && p.pair.add() == 61;
    })
    test("generic struct can be passed as function arg", () => {
        return mul_int_pair(PairGen <int, int, int> {
            a : 2,
            b : 9
        }) == 18
    })
    test("generic structs can be returned - 1", () => {
        const p = create_pair_gen();
        return p.add() == 25;
    })
    test("generic structs can be returned - 2", () => {
        const p = create_pair_gen_long();
        return p.add() == 26;
    })
}