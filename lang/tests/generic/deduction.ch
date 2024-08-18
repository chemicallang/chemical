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


func test_generic_type_deduction() {
    test("generic function call without types work", () => {
        return deduce_gen_sum(10, 10) == 20;
    })
    test("generic function call with types work", () => {
        return deduce_gen_sum<long>(10, 10) == 20;
    })
    test("nested generic type parameters can be deduced in function call", () => {
        const s = DeduceThing<int> {
            t : 20
        }
        return deduce_thing(s) == 20;
    })
    test("nested generic type parameters can be deduced in function call", () => {
        const s = DeduceThing<long> {
            t : 40
        }
        return deduce_thing(s) == 40;
    })
}