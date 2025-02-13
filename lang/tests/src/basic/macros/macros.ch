func test_macros() {
    test_sizeof_alignof();
    test("evaluation macro works", () => {
        var evaluated = comptime { 2 + 2 };
        return evaluated == 4;
    });
}