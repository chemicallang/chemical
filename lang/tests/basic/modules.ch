import "../test.ch"
import "./submod/check.ch"

func test_imported_modules() {
    test("function imported from other module works", () => {
        return extern_imported_sum(10, 10) == 40;
    })
    test("basic structs imported from other modules work", () => {
        var p = extern_imported_point { a : 20, b : 22 }
        const sum = p.a + p.b;
        return sum == 42;
    })
    test("functions of basic structs imported from other modules work", () => {
        var p = extern_imported_point { a : 10, b : 22 }
        return p.check_sum() == 32;
    })
    test("global variables imported from other modules work", () => {
        extern_globe_var_incr();
        return extern_globe_var == 769
    })
    test("enums imported from other modules work - 1", () => {
        return extern_enum_get_banana() == extern_enum_fruits.banana
    })
    test("enums imported from other modules work - 2", () => {
        return extern_enum_get_mango() == extern_enum_fruits.mango
    })
    test("enums imported from other modules work - 3", () => {
        return extern_enum_get_mango() != extern_enum_fruits.banana
    })
    test("variants imported from other modules work - 1", () => {
        return get_extern_imported_opt_value(get_extern_imported_opt_none()) == -1
    })
    test("variants imported from other modules work - 2", () => {
        return get_extern_imported_opt_value(get_extern_imported_opt_some()) == 50
    })
    test("variants imported from other modules work - 3", () => {
        return get_extern_imported_opt_value(extern_imported_opt.Some(102)) == 102
    })
    test("unused generic struct from other modules work - 1", () => {
        var p = extern_unused_gen_struct<int> {
            a : 10,
            b : 10
        }
        const sum = p.a + p.b
        return sum == 20
    })
    test("unused generic struct from other modules works with multiple types - 1", () => {
        var p = extern_unused_gen_struct2<short> {
            a : 12,
            b : 12
        }
        const sum = p.a + p.b
        return sum == 24
    })
    test("unused generic struct from other modules works with multiple types - 2", () => {
        var p = extern_unused_gen_struct2<long> {
            a : 454,
            b : 20
        }
        const sum = p.a + p.b
        return sum == 474
    })
}