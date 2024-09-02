import "../test.ch"

variant OptVariant {
    Some(a : int)
    None()
}

struct CheckImpParam22 {
    var a : int
    @implicit
    @constructor
    func make(x : int) {
        a = x;
    }
}

variant ImpVar22 {
    Some(a : CheckImpParam22)
    None()
}

func get_value_iv22(variant : ImpVar22) : int {
    switch(variant) {
        case ImpVar22.Some(a) => {
            return a.a;
        }
        case ImpVar22.None => {
            return -1;
        }
    }
}

func get_value(variant : OptVariant) : int {
    switch(variant) {
        case OptVariant.Some(a) => {
            return a;
        }
        case OptVariant.None => {
            return -1;
        }
    }
}

func give_variant(some : bool) : OptVariant {
    if(some) {
        return OptVariant.Some(30)
    } else {
        return OptVariant.None;
    }
}

struct ContOptVariant {
    var v : OptVariant
}

variant GenVar<T> {
    First(a : T, b: T)
    Second(a : T, b : T, c : T)
}

func get_sum(v : GenVar<int>) : int {
    switch(v) {
        case GenVar.First(a, b) => {
            return a + b;
        }
        case GenVar.Second(a, b, c) => {
            return a + b + c;
        }
    }
}

func test_variants() {
    test("variants can be passed to functions - 1", () => {
        return get_value(OptVariant.Some(10)) == 10;
    })
    test("variants can be passed to functions - 2", () => {
        return get_value(OptVariant.None) == -1;
    })
    test("variants can be passed to functions - 2", () => {
        return get_value(OptVariant.None) == -1;
    })
    test("allocated variant is in unspecified state", () => {
        var v : OptVariant
        switch(v) {
            case OptVariant.Some() => {
                return false;
            }
            case OptVariant.None() => {
                return false;
            }
        }
        return true;
    })
    test("variants can be returned from functions - 1", () => {
        const v = give_variant(true);
        return get_value(v) == 30
    })
    test("variants can be returned from functions - 2", () => {
        const v = give_variant(false);
        return get_value(v) == -1
    })
    test("variants can be stored in structs - 1", () => {
        var c = ContOptVariant { v : OptVariant.Some(55) }
        return get_value(c.v) == 55;
    })
    test("variants can be stored in structs - 2", () => {
        var c = ContOptVariant { v : OptVariant.None }
        return get_value(c.v) == -1;
    })
    test("variants can be stored in arrays - 1", () => {
        var c = { OptVariant.Some(76) }
        return get_value(c[0]) == 76;
    })
    test("variants can be stored in arrays - 2", () => {
        var c = { OptVariant.None }
        return get_value(c[0]) == -1;
    })
    test("variants can be stored in arrays - 3", () => {
        var c : OptVariant[] = { OptVariant.None, OptVariant.Some(43) }
        return get_value(c[0]) == -1 && get_value(c[1]) == 43;
    })
    test("variants can be stored in variables", () => {
        var v = OptVariant.Some(12)
        return get_value(v) == 12;
    })
    test("variants can be stored in variables", () => {
        var v = OptVariant.None;
        return get_value(v) == -1
    })
    test("generic variants work - 1", () => {
        return get_sum(GenVar.First(20, 30)) == 50;
    })
    test("generic variants work - 2", () => {
        return get_sum(GenVar.Second(20, 30, 2)) == 52;
    })
    test("test implicit args in variant calls", () => {
        var v = ImpVar22.Some(99)
        return get_value_iv22(v) == 99;
    })
}