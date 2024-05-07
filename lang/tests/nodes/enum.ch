import "../test.ch"

enum Thing {
    Fruit,
    Veg
}

func test_enum() {
    test("enum index works", () => {
        return Thing.Fruit == 0 && Thing.Veg == 1;
    })
    test("enum comparison works", () => {
        return Thing.Fruit == Thing.Fruit && Thing.Veg == Thing.Veg && Thing.Fruit != Thing.Veg;
    })
}