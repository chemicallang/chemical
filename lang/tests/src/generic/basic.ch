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

struct UnusedGeneric938374<T, U> {
    var a : T
    var b : U
}

struct UnusedGenericInherited234234<T, U> : UnusedGeneric938374<T, U> {
    var c : T
    var d : U
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

func <T, U, V> (pair : &PairGen<T, U, V>) ext_div() : V {
    return pair.a / pair.b;
}

@direct_init
struct ShortPairGen {
    var pair : PairGen<short, short, short>
    @constructor
    func make() {
        init {
            pair(PairGen<short, short, short> {
                a : 33,
                b : 10
            })
        }
    }
    func add(&self) : short {
        return pair.add();
    }
}

func get_other_gen_long(val : long) : OtherGen9<long> {
    return OtherGen9<long> { value : val }
}

func get_other_gen_long_value(t : OtherGen9<long>) : long {
    return t.value;
}

struct OV1Point {
    var a : int
    var b : int
    var c : int
}

func get_other_var1_point(value : int, some : bool) : OtherVar1<OV1Point> {
    if(some) {
        return OtherVar1.Some(OV1Point { a : value, b : value, c : value })
    } else {
        return OtherVar1.None();
    }
}

func get_other_var1_point_value(other : OtherVar1<OV1Point>) : int {
    switch(other) {
        Some(value) => {
            return value.a + value.b + value.c;
        }
        None => {
            return -1;
        }
    }
}

struct check_gen_multi_typed<T> {
    var a : T
    var b : T
}

func <T> gen_ret_func(value : T) : T {
    if(T is char || T is uchar) {
        return value + 1
    } else if(T is short || T is ushort) {
        return value + 2
    } else if(T is int || T is uint) {
        return value + 4
    } else if(T is bigint || T is ubigint) {
        return value + 8
    } else {
        return value + 0
    }
}

func <T> gen_ret_func2(value : T) : T {
    // TODO this parameter can't be inferred, please infer it
    return gen_ret_func<T>(value)
}

struct check_gen_right_impl<T> {

    func get_integer() : T {
        if(T is char || T is uchar) {
            return 1;
        } else if(T is short || T is ushort) {
            return 2;
        } else if(T is int || T is uint) {
            return 4;
        } else if(T is bigint || T is ubigint) {
            return 8;
        } else {
            return 0;
        }
    }

}

// struct won't be used, but calls a generic function
// tests: doesn't cause a compiler error just by being there
struct unused_gen_calls_gen<T> {
    func call_gen_func(value : T) {
        var x = gen_ret_func(value)
    }
}

func <T> gen_func_called_by_gen_struct(value : T) : T {
    if(T is char || T is uchar) {
        return value + 1;
    } else if(T is short || T is ushort) {
        return value + 2
    } else if(T is int || T is uint) {
        return value + 4;
    } else if (T is bigint || T is ubigint) {
        return value + 8;
    } else {
        return 0
    }
}

struct gen_struct_calls_gen<T> {
    func call_it(value : T) : T {
        // TODO couldn't deduce this generic argument
        return gen_func_called_by_gen_struct<T>(value);
    }
}

@comptime
func <T> comptime_func_call_gen1() : int {
    if(T is short) {
        return 2
    } else if(T is int) {
        return 4
    } else if(T is bigint) {
        return 8
    } else {
        return 0;
    }
}

func <T> gen_wrap_comptime_func_call() : int {
    return comptime_func_call_gen1<T>();
}

struct wrap_gen_comptime_func_call<T> {
    func do_call() : int {
        return comptime_func_call_gen1<T>();
    }
}

func test_basic_generics() {
    test("basic generic function with no generic args works", () => {
        return gen_sum(10, 20) == 30;
    })
    test("basic generic function with generic args works", () => {
        return gen_sum<long, long, long>(20, 20) == 40;
    })
    test("generic functions can be called inside other calls", () => {
        return is_this_60(gen_sum<long, long, long>(30, 30));
    })
    test("generic functions result can be saved into variables", () => {
        var i = gen_sum<long, long, long>(30, 40);
        return i == 70;
    })
    test("generic struct works - 1", () => {
        var p = PairGen <int, int, int> { a : 10, b : 12 }
        return p.add() == 22;
    })
    test("generic struct works - 2", () => {
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
    test("extension functions work on generic nodes", () => {
        var p = PairGen<short, short, short> {
            a : 56,
            b : 7
        }
        return p.ext_div<short, short, short>() == 8;
    })
    test("generic structs declared and used from other files work - 1", () => {
        const g = get_other_gen(20);
        return get_other_gen_value(g) == 20;
    })
    test("generic structs declared and used from other files work - 2", () => {
        const g = get_other_gen_long(22);
        return get_other_gen_long_value(g) == 22;
    })
    test("generic variants declared and used from other files work - 1", () => {
        const g = get_other_var1(10, true);
        return get_other_var1_value(g) == 10;
    })
    test("generic variants declared and used from other files work - 2", () => {
        const g = get_other_var1(10, false);
        return get_other_var1_value(g) == -1;
    })
    test("generic variants declared and used from other files work - 3", () => {
        const g = get_other_var1_point(10, true);
        return get_other_var1_point_value(g) == 30;
    })
    test("generic variants declared and used from other files work - 4", () => {
        const g = get_other_var1_point(10, false);
        return get_other_var1_point_value(g) == -1;
    })
    test("unused generic struct works with multiple types - 1", () => {
        var p = check_gen_multi_typed<short> {
            a : 13,
            b : 13
        }
        const sum = p.a + p.b
        return sum == 26
    })
    test("unused generic struct works with multiple types - 2", () => {
        var p = check_gen_multi_typed<long> {
            a : 452,
            b : 20
        }
        const sum = p.a + p.b
        return sum == 472
    })
    test("struct type can be stored in generic struct", () => {
        var g = check_gen_multi_typed<OV1Point> {
            a : OV1Point {
                a : 10, b : 20, c : 30
            },
            b : OV1Point {
                a : 40, b : 50, c : 60
            }
        }
        return g.a.a == 10 && g.a.b == 20 && g.a.c == 30 && g.b.a == 40 && g.b.b == 50 && g.b.c == 60;
    })
    test("generic functions call the right instantiation - 1", () => {
        return gen_ret_func(0i8) == 1 && gen_ret_func(0ui8) == 1
    })
    test("generic functions call the right instantiation - 2", () => {
        return gen_ret_func(0i16) == 2 && gen_ret_func(0ui16) == 2
    })
    test("generic functions call the right instantiation - 3", () => {
        return gen_ret_func(0i32) == 4 && gen_ret_func(0ui32) == 4
    })
    test("generic functions call the right instantiation - 4", () => {
        return gen_ret_func(0i64) == 8 && gen_ret_func(0ui64) == 8
    })
    // duplicating tests so when generic implementations are reused based on types
    // we test that correct implementations are called still.
    test("generic functions call the right instantiation - 5", () => {
        return gen_ret_func(0i8) == 1 && gen_ret_func(0ui8) == 1
    })
    test("generic functions call the right instantiation - 6", () => {
        return gen_ret_func(0i16) == 2 && gen_ret_func(0ui16) == 2
    })
    test("generic functions call the right instantiation - 7", () => {
        return gen_ret_func(0i32) == 4 && gen_ret_func(0ui32) == 4
    })
    test("generic functions call the right instantiation - 8", () => {
        return gen_ret_func(0i64) == 8 && gen_ret_func(0ui64) == 8
    })
    test("generic functions calling other generic functions call the right instantiation  - 1", () => {
        return gen_ret_func2(0i8) == 1 && gen_ret_func2(0ui8) == 1
    })
    test("generic functions calling other generic functions call the right instantiation  - 2", () => {
        return gen_ret_func2(0i16) == 2 && gen_ret_func2(0ui16) == 2
    })
    test("generic functions calling other generic functions call the right instantiation  - 3", () => {
        return gen_ret_func2(0i32) == 4 && gen_ret_func2(0ui32) == 4
    })
    test("generic functions calling other generic functions call the right instantiation  - 4", () => {
        return gen_ret_func2(0i64) == 8 && gen_ret_func2(0ui64) == 8
    })
    // duplicating tests so when generic implementations are reused based on types
    // we test that correct implementations are called still.
    test("generic functions calling other generic functions call the right instantiation  - 5", () => {
        return gen_ret_func2(0i8) == 1 && gen_ret_func2(0ui8) == 1
    })
    test("generic functions calling other generic functions call the right instantiation  - 6", () => {
        return gen_ret_func2(0i16) == 2 && gen_ret_func2(0ui16) == 2
    })
    test("generic functions calling other generic functions call the right instantiation  - 7", () => {
        return gen_ret_func2(0i32) == 4 && gen_ret_func2(0ui32) == 4
    })
    test("generic functions calling other generic functions call the right instantiation  - 8", () => {
        return gen_ret_func2(0i64) == 8 && gen_ret_func2(0ui64) == 8
    })
    test("functions inside generic struct call correct implementation based on type - 1", () => {
        var i = check_gen_right_impl<char> {}
        var u = check_gen_right_impl<uchar> {}
        return i.get_integer() == 1 && u.get_integer() == 1
    })
    test("functions inside generic struct call correct implementation based on type - 2", () => {
        var i = check_gen_right_impl<short> {}
        var u = check_gen_right_impl<ushort> {}
        return i.get_integer() == 2 && u.get_integer() == 2
    })
    test("functions inside generic struct call correct implementation based on type - 3", () => {
        var i = check_gen_right_impl<int> {}
        var u = check_gen_right_impl<uint> {}
        return i.get_integer() == 4 && u.get_integer() == 4
    })
    test("functions inside generic struct call correct implementation based on type - 4", () => {
        var i = check_gen_right_impl<bigint> {}
        var u = check_gen_right_impl<ubigint> {}
        return i.get_integer() == 8 && u.get_integer() == 8
    })
    // duplicating tests so when generic implementations are reused based on types
    // we test that correct implementations are called still.
    test("functions inside generic struct call correct implementation based on type - 5", () => {
        var i = check_gen_right_impl<char> {}
        var u = check_gen_right_impl<uchar> {}
        return i.get_integer() == 1 && u.get_integer() == 1
    })
    test("functions inside generic struct call correct implementation based on type - 6", () => {
        var i = check_gen_right_impl<short> {}
        var u = check_gen_right_impl<ushort> {}
        return i.get_integer() == 2 && u.get_integer() == 2
    })
    test("functions inside generic struct call correct implementation based on type - 7", () => {
        var i = check_gen_right_impl<int> {}
        var u = check_gen_right_impl<uint> {}
        return i.get_integer() == 4 && u.get_integer() == 4
    })
    test("functions inside generic struct call correct implementation based on type - 8", () => {
        var i = check_gen_right_impl<bigint> {}
        var u = check_gen_right_impl<ubigint> {}
        return i.get_integer() == 8 && u.get_integer() == 8
    })
    test("generic function call inside a function of a generic struct calls correct implementation - 1", () => {
        var i = gen_struct_calls_gen<char> {};
        var u = gen_struct_calls_gen<uchar> {};
        return i.call_it(60) == 61 && u.call_it(60) == 61
    })
    test("generic function call inside a function of a generic struct calls correct implementation - 2", () => {
        var i = gen_struct_calls_gen<short> {};
        var u = gen_struct_calls_gen<ushort> {};
        return i.call_it(60) == 62 && u.call_it(60) == 62
    })
    test("generic function call inside a function of a generic struct calls correct implementation - 3", () => {
        var i = gen_struct_calls_gen<int> {};
        var u = gen_struct_calls_gen<uint> {};
        return i.call_it(60) == 64 && u.call_it(60) == 64
    })
    test("generic function call inside a function of a generic struct calls correct implementation - 4", () => {
        var i = gen_struct_calls_gen<bigint> {};
        var u = gen_struct_calls_gen<ubigint> {};
        return i.call_it(60) == 68 && u.call_it(60) == 68
    })
    // duplicating tests so when generic implementations are reused based on types
    // we test that correct implementations are called still.
    test("generic function call inside a function of a generic struct calls correct implementation - 5", () => {
        var i = gen_struct_calls_gen<char> {};
        var u = gen_struct_calls_gen<uchar> {};
        return i.call_it(60) == 61 && u.call_it(60) == 61
    })
    test("generic function call inside a function of a generic struct calls correct implementation - 6", () => {
        var i = gen_struct_calls_gen<short> {};
        var u = gen_struct_calls_gen<ushort> {};
        return i.call_it(60) == 62 && u.call_it(60) == 62
    })
    test("generic function call inside a function of a generic struct calls correct implementation - 7", () => {
        var i = gen_struct_calls_gen<int> {};
        var u = gen_struct_calls_gen<uint> {};
        return i.call_it(60) == 64 && u.call_it(60) == 64
    })
    test("generic function call inside a function of a generic struct calls correct implementation - 8", () => {
        var i = gen_struct_calls_gen<bigint> {};
        var u = gen_struct_calls_gen<ubigint> {};
        return i.call_it(60) == 68 && u.call_it(60) == 68
    })
    test("comptime function calls in generic functions are reevaluated between different types - 1", () => {
        return gen_wrap_comptime_func_call<short>() == 2;
    })
    test("comptime function calls in generic functions are reevaluated between different types - 2", () => {
        return gen_wrap_comptime_func_call<int>() == 4;
    })
    test("comptime function calls in generic functions are reevaluated between different types - 3", () => {
        return gen_wrap_comptime_func_call<bigint>() == 8;
    })
    test("comptime function calls in generic functions are reevaluated between different types - 4", () => {
        return gen_wrap_comptime_func_call<ushort>() == 0;
    })
    test("comptime function calls in generic structs are reevaluated between different types - 1", () => {
        var p1 = wrap_gen_comptime_func_call<short> {}
        return p1.do_call() == 2;
    })
    test("comptime function calls in generic structs are reevaluated between different types - 2", () => {
        var p1 = wrap_gen_comptime_func_call<int> {}
        return p1.do_call() == 4;
    })
    test("comptime function calls in generic structs are reevaluated between different types - 3", () => {
        var p1 = wrap_gen_comptime_func_call<bigint> {}
        return p1.do_call() == 8;
    })
    test("comptime function calls in generic structs are reevaluated between different types - 4", () => {
        var p1 = wrap_gen_comptime_func_call<ushort> {}
        return p1.do_call() == 0;
    })
}