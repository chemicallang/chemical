import "../test.ch"

enum Thing {
    Fruit,
    Veg
}

struct EnumThing {
    var value : Thing
}

func take_my_enum_dawg(numnum : Thing) : bool {
    return numnum == Thing.Fruit
}

func take_my_enum_again_dawg(numnum : Thing) : bool {
    return numnum == Thing.Veg
}

func take_my_thing() : Thing {
    return Thing.Veg
}

func test_enum() {
    test("enum index works", () => {
        return Thing.Fruit == 0 && Thing.Veg == 1;
    })
    test("enum comparison works", () => {
        return Thing.Fruit == Thing.Fruit && Thing.Veg == Thing.Veg && Thing.Fruit != Thing.Veg;
    })
    test("enums can be initialize variables", () => {
        var x = Thing.Fruit
        return x == Thing.Fruit && x != Thing.Veg
    })
    test("enums can be stored in variables", () => {
        var x : Thing
        x = Thing.Veg
        return x == Thing.Veg && x != Thing.Fruit
    })
    test("check enums can be stored in structs", () => {
        var p = EnumThing { value : Thing.Fruit }
        return p.value == Thing.Fruit && p.value != Thing.Veg
    })
    test("check enums can be stored in structs - 2", () => {
        var p = EnumThing { value : Thing.Veg }
        return p.value == Thing.Veg && p.value != Thing.Fruit
    })
    test("check enums can be passed to functions", () => {
        return take_my_enum_dawg(Thing.Fruit)
    })
    test("check enums can be passed to functions - 2", () => {
        return take_my_enum_again_dawg(Thing.Veg)
    })
    test("check enums can be passed to functions - 3", () => {
        var a = Thing.Fruit
        return take_my_enum_dawg(a)
    })
    test("test enums can be returned from functions", () => {
        return take_my_thing() == Thing.Veg;
    })
}