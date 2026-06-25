// Native-only generic tests that can't run in interpret mode:
func test_native_generic_specifics() {
    test("monomorphization of a struct present in a module that is not directly inherited works", () => {
        var s = ExposedGenSecond<int> { value : 9473 }
        return s.give() == 9474
    })
}

