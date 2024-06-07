import "../test.ch"

@deprecated
func add(a : int, b : int) {
    return a + b;
}

func test_annotations() {
    test("test annotations work", () => {
        return add(1 + 2) == 3;
    })
}