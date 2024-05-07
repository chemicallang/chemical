import "../test.ch"

enum Thing {
    Fruit,
    Veg
}

func test_enum() {
    test("enum index works", () => {
        return Thing.Fruit == 0 && Thing.Veg == 1;
    })
}