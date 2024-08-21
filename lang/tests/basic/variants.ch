import "../test.ch"

variant OptVariant {
    Some(a : int)
    None()
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
}