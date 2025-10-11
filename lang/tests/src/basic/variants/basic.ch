variant OptVariant {
    Some(a : int)
    None()
}

variant GenVarIsValTest<T> {
    Some(value : T)
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

struct VariantInheritedPoint {
    var a : int = 10
    var b : int = 20

    func multiply(&self) : int {
        return a * b;
    }

}

func variant_inh_point_sum(p : *VariantInheritedPoint) : int {
    return p.a + p.b;
}

variant InheriterOfPoint : VariantInheritedPoint {
    First(x : int, y : int)
    Second(x : int, y : int, z : int)
}

struct ConstructedVertex {
    var x : int
    var y : int
    var z : int
    @make
    func make() {
        x = 98
        y = 97
        z = 96
    }
}

variant VertexInheriter : ConstructedVertex {
    First(i : int)
    Second(j : int)
}

// EXISTENCE TEST BEGIN
// This tests whether an error of type satisfaction is caused, when we use
// ubigint_type_alias, the calls to variant members cause type satisfaction bug
// with the return type (even though they are same types) but if one is canonical
// it rejects the typealias type (ubigint doesn't get satisfied from ubigint_type_alias)
public variant CustomResult<T, E> {
    Ok(value : T)
    Err(error : E)
}

public type ubigint_type_alias = ubigint

public variant CustomFsError {
    NotFound()
    AlreadyExists()
}

func first_variant_result_giver() : CustomResult<ubigint_type_alias, CustomFsError> {
    return CustomResult.Ok<ubigint_type_alias, CustomFsError>(1)
}

func second_variant_result_giver() : CustomResult<ubigint_type_alias, CustomFsError> {
    return CustomResult.Err<ubigint_type_alias, CustomFsError>(CustomFsError.NotFound())
}

// EXISTENCE TEST END

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
        var c : []OptVariant = [ OptVariant.None(), OptVariant.Some(43) ]
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
    test("is value work with variants - 1", () => {
        var v1 = OptVariant.Some(10)
        return v1 is OptVariant.Some
    })
    test("is value work with variants - 2", () => {
        var v1 = OptVariant.Some(10)
        return v1 !is OptVariant.None
    })
    test("is value work with variants - 3", () => {
        var v1 = OptVariant.None()
        return v1 is OptVariant.None
    })
    test("is value work with variants - 4", () => {
        var v1 = OptVariant.None()
        return v1 !is OptVariant.Some
    })
    test("is value work with variants - 5", () => {
        var v1 = OptVariant.Some(10)
        return v1 is OptVariant.Some && v1 !is OptVariant.None
    })
    test("is value work with variants - 6", () => {
        var v1 = OptVariant.None()
        return v1 is OptVariant.None && v1 !is OptVariant.Some
    })
    test("is value work with variants - 7", () => {
        return check_variant_is_some(OptVariant.Some(10))
    })
    test("is value work with variants - 8", () => {
        return !check_variant_is_none(OptVariant.Some(10))
    })
    test("is value work with variants - 9", () => {
        return check_variant_is_none(OptVariant.None())
    })
    test("is value work with variants - 10", () => {
        return !check_variant_is_some(OptVariant.None())
    })
    test("is value works with generic variant - 1", () => {
        var v = GenVarIsValTest.Some<short>(9201)
        return v is GenVarIsValTest.Some && v !is GenVarIsValTest.None
    })
    test("is value works with generic variant - 2", () => {
        var v = GenVarIsValTest.None<short>()
        return v is GenVarIsValTest.None && v !is GenVarIsValTest.Some
    })
    test("is value works with generic variant - 3", () => {
        var v = GenVarIsValTest.Some<int>(9201)
        return v is GenVarIsValTest.Some && v !is GenVarIsValTest.None
    })
    test("is value works with generic variant - 4", () => {
        var v = GenVarIsValTest.None<int>()
        return v is GenVarIsValTest.None && v !is GenVarIsValTest.Some
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
    test("inherited struct in variant is default initialized properly - 1", () => {
        var i = InheriterOfPoint.First(48, 58)
        return i.a == 10 && i.b == 20
    })
    test("inherited struct in variant is default initialized properly - 2", () => {
        var i = InheriterOfPoint.First(48, 58)
        var First(x, y) = i else unreachable
        return x == 48 && y == 58 && i.a == 10 && i.b == 20
    })
    test("inherited struct in variant is default initialized properly - 3", () => {
        var i = InheriterOfPoint.Second(65, 87, 45)
        return i.a == 10 && i.b == 20
    })
    test("inherited struct in variant is default initialized properly - 4", () => {
        var i = InheriterOfPoint.Second(65, 87, 45)
        var Second(x, y, z) = i else unreachable
        return x == 65 && y == 87 && z == 45 && i.a == 10 && i.b == 20
    })
    test("inherited struct in variant is default initialized with constructor - 1", () => {
        var i = VertexInheriter.First(92)
        return i.x == 98 && i.y == 97 && i.z == 96
    })
    test("inherited struct in variant is default initialized with constructor - 2", () => {
        var x = VertexInheriter.First(92)
        var First(i) = x else unreachable
        return i == 92 && x.x == 98 && x.y == 97 && x.z == 96
    })
    test("inherited struct in variant is default initialized with constructor - 3", () => {
        var i = VertexInheriter.Second(83)
        return i.x == 98 && i.y == 97 && i.z == 96
    })
    test("inherited struct in variant is default initialized with constructor - 4", () => {
        var i = VertexInheriter.Second(83)
        var Second(j) = i else unreachable
        return j == 83 && i.x == 98 && i.y == 97 && i.z == 96
    })
    test("inherited struct in variant can be passed to functions as pointers - 1", () => {
        // lets test passing the struct only first
        var p = VariantInheritedPoint { a : 34, b : 21 }
        return variant_inh_point_sum(&p) == 55
    })
    test("inherited struct in variant can be passed to functions as pointers - 2", () => {
        var p = InheriterOfPoint.First(65, 54)
        p.a = 43
        p.b = 13
        return variant_inh_point_sum(&p) == 56
    })
    test("methods on inherited struct in variant can be called - 1", () => {
        // lets test passing the struct only first
        var p = VariantInheritedPoint { a : 15, b : 3 }
        return p.multiply() == 45
    })
    test("methods on inherited struct in variant can be called - 2", () => {
        var p = InheriterOfPoint.First(65, 54)
        p.a = 8
        p.b = 4
        return p.multiply() == 32
    })
    test("pointer to variant works in switch pattern match", () => {
        var p = new OptVariant
        new (p) OptVariant.Some(765)
        var result : int = 0
        switch(p) {
            Some(a) => {
                result = a;
            }
            None() => {

            }
        }
        dealloc p
        return result == 765
    })
    test("different sized objects in variants have same size upon allocation", () => {
        var first = OptVariant.Some(32)
        var second = OptVariant.None()
        var expected_size = sizeof(OptVariant)
        var first_size_of = sizeof(first)
        var second_size_of = sizeof(second)
        return first_size_of == second_size_of && first_size_of == expected_size
    })
    test("assignment works with variant calls", () => {
        var x = ImpVar22.None()
        x = ImpVar22.Some(CheckImpParam22(99))
        return x is ImpVar22.Some
    })
    /** TODO:
    test("pointer to variant works in switch pattern match", () => {
        var p = new OptVariant
        new (p) OptVariant.Some(262)
        var result : int = 0
        switch(*p) {
            Some(a) => {
                result = a;
            }
            None() => {

            }
        }
        dealloc p
        return result == 262
    })
    **/
}