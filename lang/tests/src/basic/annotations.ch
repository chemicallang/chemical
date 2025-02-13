@deprecated
func add(a : int, b : int) : int {
    return a + b;
}

func test_annotations() {
    test("annotations work", () => {
        return add(1, 2) == 3;
    })
}