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

enum StartingValueInEnum {
    First = 50,
    Second,
    Third,
    Fourth = 10,
    Fifth,
    Sixth,
    NegFirst = -10,
    NegSecond,
    NegThird
}

func take_addr_of_enum_param(check : MultiNum) : MultiNum {
    var addr = &check
    return *addr;
}

enum UIntEnumDecl : uint {
    First = 264,
    Second = 832
}

func i_take_uint_enum(f : uint, s : uint) : bool {
    return f == 264 && s == 832
}

func test_native_enum() {
    test("address of enum works in function", () => {
        return take_addr_of_enum_param(MultiNum.Second) == MultiNum.Second
    })
}