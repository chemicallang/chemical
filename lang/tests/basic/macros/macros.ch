import "./sizeof_alignof.ch"

func test_macros() {
    test_sizeof_alignof();
    test("test evaluation macro works", () => {
        var evaluated = #eval { 2 + 2 };
        return evaluated == 4;
    });
}