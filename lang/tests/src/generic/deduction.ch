import "../test.ch"

func <T> deduce_gen_sum(a : T, b : T) : T {
    return a + b;
}

struct DeduceThing<T> {
    var t : T
}

func <T> deduce_thing(thing : DeduceThing<T>) : T {
    return thing.t;
}

func <T> give_gen_ret(a : int, b : int) : T {
    return a + b;
}

func take_gen_ret(sum : int) : int {
    return sum + 10;
}

func take_gen_ret_long(sum : long) : long {
    return sum + 20;
}

func test_generic_type_deduction() {
    test("generic function call without types work", () => {
        return deduce_gen_sum(10, 10) == 20;
    })
    test("generic function call with types work", () => {
        return deduce_gen_sum<long>(10, 10) == 20;
    })
    test("nested generic type parameters can be deduced in function call - 1", () => {
        const s = DeduceThing<int> {
            t : 20
        }
        return deduce_thing(s) == 20;
    })
    test("nested generic type parameters can be deduced in function call - 2", () => {
        const s = DeduceThing<long> {
            t : 40
        }
        return deduce_thing(s) == 40;
    })
    test("generic return type can be deduced when in function call - 1", () => {
        return take_gen_ret(give_gen_ret(16, 16)) == 42;
    })
    test("generic return type can be deduced when in function call - 2", () => {
        return take_gen_ret_long(give_gen_ret(15, 15)) == 50;
    })
}