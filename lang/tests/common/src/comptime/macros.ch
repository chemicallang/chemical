func test_macros() {
    test("evaluation macro works", () => {
        var evaluated = comptime { 2 + 2 };
        return evaluated == 4;
    });
}