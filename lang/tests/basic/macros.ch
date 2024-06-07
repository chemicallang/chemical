import "../test.ch"

func test_macros() {
    test("test evaluation macro works", () => {
        var evaluated = #eval { 2 + 2 };
        return evaluated == 4;
    });
}