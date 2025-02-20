enum Thing {
    Fruit,
    Veg
}

struct EnumThing {
    var value : Thing
}

namespace enum_parent_ns {
    enum favorite {
        fruit,
        veges,
        coke
    }
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

enum Thing22 : uchar {
    Fruit,
    Veg,
    OtherStuff,
    MoreOtherStuff
}

enum Anything : Thing {
    Space,
    Universe,
    Garbage
}

enum Thing33 : ushort {
    Fruit,
    Veg,
    OtherStuff,
    MoreOtherStuff
}

enum MultiNum {
    First,
    Second,
    AnotherFirst = First,
    AnotherSecond = Second
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
    test("enums can be returned from functions", () => {
        return take_my_thing() == Thing.Veg;
    })
    test("enums with underlying type work with uchar", () => {
        const one = 0 as uchar
        const two = 1 as uchar
        const three = 2 as uchar
        const four = 3 as uchar
        return Thing22.Fruit == one && Thing22.Veg == two && Thing22.OtherStuff == three && Thing22.MoreOtherStuff == four
    })
    test("enums with underlying type work with ushort", () => {
        const one = 0 as ushort
        const two = 1 as ushort
        const three = 2 as ushort
        const four = 3 as ushort
        return Thing33.Fruit == one && Thing33.Veg == two && Thing33.OtherStuff == three && Thing33.MoreOtherStuff == four
    })
    test("enums with underlying type work with number values", () => {
        return Thing33.Fruit == 0 && Thing33.Veg == 1 && Thing33.OtherStuff == 2 && Thing33.MoreOtherStuff == 3
    })
    test("enums automatically cast according to underlying type", () => {
        const one = 0
        const two = 1
        const three = 2
        const four = 3
        return Thing33.Fruit == one && Thing33.Veg == two && Thing33.OtherStuff == three && Thing33.MoreOtherStuff == four
    })
    test("enum inheritance works - 1", () => {
        return Anything.Fruit == 0 && Anything.Veg == 1 && Anything.Space == 2 && Anything.Universe == 3 && Anything.Garbage == 4
    })
    test("enum members of enum can reference other members as their values", () => {
        return MultiNum.First == MultiNum.AnotherFirst && MultiNum.Second == MultiNum.AnotherSecond
    })
    test("enums inside namespace work too", () => {
        return enum_parent_ns::favorite.fruit == 0 && enum_parent_ns::favorite.veges == 1 && enum_parent_ns::favorite.coke == 2
    })
}