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
}