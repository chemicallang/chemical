import "../test.ch"
import "./submod/check.ch"

func test_imported_modules() {
    test("function imported from other module works", () => {
        return extern_imported_sum(10, 10) == 40;
    })
}