type SimpleFunc = () => int;

func take_simple_func(simple : SimpleFunc) : int {
    return simple();
}

type SimpleFunc2 = () => void;

func take_simple_func2(simple : SimpleFunc2) {
    simple();
}

var my_secret_typealias_number = 0;

comptime const if_type_as_alias_is_32 = true
type if_type_as_alias = if(if_type_as_alias_is_32) u32 else u64
type if_type_as_alias_2 = if(!if_type_as_alias_is_32) u32 else u64

comptime const if_type_as_alias_is_32_not = false
type if_type_as_alias3 = if(if_type_as_alias_is_32_not) u64 else u32

func test_typealias() {
    test("typealias to a function works - 1", () => {
        return take_simple_func(() => 674) == 674;
    })
    test("typealias to a function works - 2", () => {
        return take_simple_func(() => 837) == 837;
    })
    test("typealias to a function works - 3", () => {
        take_simple_func2(() => {
            my_secret_typealias_number = 2346
        })
        return my_secret_typealias_number == 2346
    })
    test("global typealias that uses if type works - 1", () => {
        return sizeof(if_type_as_alias) == 4
    })
    test("global typealias that uses if type works - 2", () => {
        return sizeof(if_type_as_alias_2) == 8
    })
    test("global typealias that uses if type works - 3", () => {
        return sizeof(if_type_as_alias3) == 4
    })
    test("local typealias that uses if type works - 1", () => {
        const local_if_type_alias_32 = true
        type local_if_type_alias = if(local_if_type_alias_32) u32 else u64
        return sizeof(local_if_type_alias) == 4
    })
    test("local typealias that uses if type works - 2", () => {
        const local_if_type_alias_32 = false
        type local_if_type_alias = if(local_if_type_alias_32) u32 else u64
        return sizeof(local_if_type_alias) == 8
    })
    test("local typealias that uses if type works - 3", () => {
        const local_if_type_alias_32 = true
        type local_if_type_alias = if(!local_if_type_alias_32) u32 else u64
        return sizeof(local_if_type_alias) == 8
    })
}