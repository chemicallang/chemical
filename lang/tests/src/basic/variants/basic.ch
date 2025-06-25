variant OptVariant {
    Some(a : int)
    None()
}

func check_variant_is_some(opt : OptVariant) : bool {
    return opt is OptVariant.Some
}

func check_variant_is_none(opt : OptVariant) : bool {
    return opt is OptVariant.None
}

struct CheckImpParam22 {
    var a : int
    @implicit
    @constructor
    func make(x : int) {
        init {
            a(x)
        }
    }
}

variant ImpVar22 {
    Some(a : CheckImpParam22)
    None()
}

func get_value_iv22(variant : ImpVar22) : int {
    switch(variant) {
        Some(a) => {
            return a.a;
        }
        None => {
            return -1;
        }
    }
}

func get_value(variant : OptVariant) : int {
    switch(variant) {
        Some(a) => {
            return a;
        }
        None => {
            return -1;
        }
    }
}

func give_variant(some : bool) : OptVariant {
    if(some) {
        return OptVariant.Some(30)
    } else {
        return OptVariant.None();
    }
}

struct ContOptVariant {
    var v : OptVariant
}

variant GenVar<T> {
    First(a : T, b: T)
    Second(a : T, b : T, c : T)
}

variant GenVarDef<T = ulong> {
    Some(value : T)
    None()
}

variant FuncInVariant {

    First(value : int)
    Second(value : int)

    func get(&self) : int {
        switch(self) {
            First(value) => return value;
            Second(value) => return value;
        }
    }

}

func <T> test_gen_var_def(value : T) : bool {
    return T is ulong
}

func get_sum(v : GenVar<int>) : int {
    switch(v) {
        First(a, b) => {
            return a + b;
        }
        Second(a, b, c) => {
            return a + b + c;
        }
    }
}

func <T> get_sum_gen(v : GenVar<T>) : T {
    switch(v) {
        First(a, b) => return a + b;
        Second(a, b, c) => return a + b + c;
    }
}

func test_variants() {
    test("variants can be passed to functions - 1", () => {
        return get_value(OptVariant.Some(10)) == 10;
    })
    test("variants can be passed to functions - 2", () => {
        return get_value(OptVariant.None()) == -1;
    })
    test("variants can be passed to functions - 2", () => {
        return get_value(OptVariant.None()) == -1;
    })
    test("allocated variant is in unspecified state", () => {
        var v : OptVariant
        switch(v) {
            Some() => {
                return false;
            }
            None() => {
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
        var c = ContOptVariant { v : OptVariant.None() }
        return get_value(c.v) == -1;
    })
    test("variants can be stored in arrays - 1", () => {
        var c = [ OptVariant.Some(76) ]
        return get_value(c[0]) == 76;
    })
    test("variants can be stored in arrays - 2", () => {
        var c = [ OptVariant.None() ]
        return get_value(c[0]) == -1;
    })
    test("variants can be stored in arrays - 3", () => {
        var c : OptVariant[] = [ OptVariant.None(), OptVariant.Some(43) ]
        return get_value(c[0]) == -1 && get_value(c[1]) == 43;
    })
    test("variants can be stored in variables", () => {
        var v = OptVariant.Some(12)
        return get_value(v) == 12;
    })
    test("variants can be stored in variables", () => {
        var v = OptVariant.None();
        return get_value(v) == -1
    })
    test("generic variants work - 1", () => {
        return get_sum(GenVar.First(20, 30)) == 50;
    })
    test("generic variants work - 2", () => {
        return get_sum(GenVar.Second(20, 30, 2)) == 52;
    })
    test("generic variants work with generic switch - 1", () => {
        return get_sum_gen(GenVar.First<short>(20, 30)) == 50;
    })
    test("generic variants work with generic switch - 2", () => {
        return get_sum_gen(GenVar.Second<short>(20, 30, 2)) == 52;
    })
    test("generic variants work with generic switch - 3", () => {
        return get_sum_gen(GenVar.First<long>(20, 30)) == 50;
    })
    test("generic variants work with generic switch - 4", () => {
        return get_sum_gen(GenVar.Second<long>(20, 30, 2)) == 52;
    })
    test("implicit args in variant calls", () => {
        var v = ImpVar22.Some(99)
        return get_value_iv22(v) == 99;
    })
    test("is values work with variants - 1", () => {
        var v1 = OptVariant.Some(10)
        return v1 is OptVariant.Some
    })
    test("is values work with variants - 2", () => {
        var v1 = OptVariant.Some(10)
        return v1 !is OptVariant.None
    })
    test("is values work with variants - 3", () => {
        var v1 = OptVariant.None()
        return v1 is OptVariant.None
    })
    test("is values work with variants - 4", () => {
        var v1 = OptVariant.None()
        return v1 !is OptVariant.Some
    })
    test("is values work with variants - 5", () => {
        var v1 = OptVariant.Some(10)
        return v1 is OptVariant.Some && v1 !is OptVariant.None
    })
    test("is values work with variants - 6", () => {
        var v1 = OptVariant.None()
        return v1 is OptVariant.None && v1 !is OptVariant.Some
    })
    test("is values work with variants - 7", () => {
        return check_variant_is_some(OptVariant.Some(10))
    })
    test("is values work with variants - 8", () => {
        return !check_variant_is_none(OptVariant.Some(10))
    })
    test("is values work with variants - 9", () => {
        return check_variant_is_none(OptVariant.None())
    })
    test("is values work with variants - 10", () => {
        return !check_variant_is_some(OptVariant.None())
    })
    test("default generic parameter types work in variants", () => {
        var s = GenVarDef.Some<>(10)
        switch(s) {
            Some(value) => {
               return test_gen_var_def(value)
            }
            None => {
                return false;
            }
        }
    })
    test("functions in variants work - 1", () => {
        var v = FuncInVariant.First(232)
        return v.get() == 232
    })
    test("functions in variants work - 2", () => {
        var v = FuncInVariant.Second(987)
        return v.get() == 987
    })
    test("storing in variants via pattern matched unreachable works", () => {
        var x = OptVariant.Some(0)
        var Some(a) = x else unreachable
        a = 87
        switch(x) {
            Some(a) => return a == 87;
            None => return false;
        }
    })
    test("storing in variants via pattern matched return works", () => {
        var x = OptVariant.Some(0)
        var Some(a) = x else return false
        a = 15
        switch(x) {
            Some(a) => return a == 15;
            None => return false;
        }
    })
    test("storing in variants via switch var case works", () => {
        var x = OptVariant.Some(0)
        var Some(a) = x else return false
        switch(x) {
            Some(a) => { a = 234 }
            None => return false;
        }
        switch(x) {
            Some(a) => return a == 234;
            None => return false;
        }
    })
}