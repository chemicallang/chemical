variant PMOpt1 {
    Some(value : int)
    None()
}

func get_pm_opt1_val(opt : PMOpt1) : int {
    var Some(value) = opt else -1
    return value;
}

func get_pm_opt1_val_with_ret(opt : PMOpt1) : int {
    var Some(value) = opt else return 0
    return value;
}

func get_pm_opt1_val_with_if(opt : PMOpt1) : int {
    if(var Some(value) = opt) {
        return value;
    }
    return -1;
}

variant PMMultiOpt {
    Some(value1 : int, value2 : int)
    None()
}

variant PMOptGen1<T> {
    Some(value : T)
    None()
}

func <T> get_pm_opt1_gen_val(opt : PMOptGen1<T>) : int {
    var Some(value) = opt else -1
    return value as int;
}

func <T> get_pm_opt1_gen_val_with_ret(opt : PMOptGen1<T>) : int {
    var Some(value) = opt else return 0
    return value as int;
}

func <T> get_pm_opt1_gen_val_with_if(opt : PMOptGen1<T>) : int {
    if(var Some(value) = opt) {
        return value as int;
    }
    return -1;
}

func test_variant_pattern_matching() {
    test("variant pattern matching with unreachable works", () => {
        var opt = PMOpt1.Some(10)
        var Some(value) = opt else unreachable;
        return value == 10
    })
    test("variant pattern matching with default value works - 1", () => {
        var opt2 = PMOpt1.Some(20)
        var Some(value) = opt2 else 10
        return value == 20;
    })
    test("variant pattern matching with default value works - 2", () => {
        var opt2 = PMOpt1.None()
        var Some(value) = opt2 else 10
        return value == 10;
    })
    test("variant pattern matching works in functions - 1", () => {
        return get_pm_opt1_val(PMOpt1.Some(32)) == 32
    })
    test("variant pattern matching works in functions - 2", () => {
        return get_pm_opt1_val(PMOpt1.None()) == -1
    })
    test("variant pattern matching works in functions with return - 1", () => {
        return get_pm_opt1_val_with_ret(PMOpt1.Some(75)) == 75
    })
    test("variant pattern matching works in functions with return - 2", () => {
        return get_pm_opt1_val_with_ret(PMOpt1.None()) == 0
    })
    test("variant pattern matching works in functions with if - 1", () => {
        return get_pm_opt1_val_with_if(PMOpt1.Some(26)) == 26
    })
    test("variant pattern matching works in functions with if - 2", () => {
        return get_pm_opt1_val_with_if(PMOpt1.None()) == -1
    })
    test("variant pattern matching with generic works in functions - 1", () => {
        return get_pm_opt1_gen_val(PMOptGen1.Some<short>(32)) == 32
    })
    test("variant pattern matching with generic  works in functions - 2", () => {
        return get_pm_opt1_gen_val(PMOptGen1.None<short>()) == -1
    })
    test("variant pattern matching with generic  works in functions with return - 1", () => {
        return get_pm_opt1_gen_val_with_ret(PMOptGen1.Some<short>(75)) == 75
    })
    test("variant pattern matching with generic  works in functions with return - 2", () => {
        return get_pm_opt1_gen_val_with_ret(PMOptGen1.None<short>()) == 0
    })
    test("variant pattern matching with generic  works in functions with if - 1", () => {
        return get_pm_opt1_gen_val_with_if(PMOptGen1.Some<short>(26)) == 26
    })
    test("variant pattern matching with generic  works in functions with if - 2", () => {
        return get_pm_opt1_gen_val_with_if(PMOptGen1.None<short>()) == -1
    })
    test("variant pattern matching with generic works in functions - 1", () => {
        return get_pm_opt1_gen_val(PMOptGen1.Some<long>(32)) == 32
    })
    test("variant pattern matching with generic  works in functions - 2", () => {
        return get_pm_opt1_gen_val(PMOptGen1.None<long>()) == -1
    })
    test("variant pattern matching with generic  works in functions with return - 1", () => {
        return get_pm_opt1_gen_val_with_ret(PMOptGen1.Some<long>(75)) == 75
    })
    test("variant pattern matching with generic  works in functions with return - 2", () => {
        return get_pm_opt1_gen_val_with_ret(PMOptGen1.None<long>()) == 0
    })
    test("variant pattern matching with generic  works in functions with if - 1", () => {
        return get_pm_opt1_gen_val_with_if(PMOptGen1.Some<long>(26)) == 26
    })
    test("variant pattern matching with generic  works in functions with if - 2", () => {
        return get_pm_opt1_gen_val_with_if(PMOptGen1.None<long>()) == -1
    })
    test("variant pattern matching with return works - 1", () => {
        var opt3 = PMOpt1.Some(15)
        var Some(value) = opt3 else return 10;
        return value == 15;
    })
    test("variant pattern matching with return works - 2", () => {
        var opt3 = PMOpt1.None()
        var Some(value) = opt3 else return true;
        return false;
    })
    test("variant pattern matching with multiple values work - 1", () => {
        var opt = PMMultiOpt.Some(10, 20)
        var Some(value1, value2) = opt else unreachable
        return value1 == 10 && value2 == 20
    })
    test("variant pattern matching with multiple values work - 2", () => {
        var opt = PMMultiOpt.Some(10, 20)
        var Some(value1, value2) = opt else return false;
        return value1 == 10 && value2 == 20
    })
    test("variant pattern matching with multiple values work - 3", () => {
        var opt = PMMultiOpt.None()
        var Some(value1, value2) = opt else return true;
        return false
    })
    test("variant pattern matching with multiple values work - 4", () => {
        var opt = PMMultiOpt.Some(10, 20)
        var Some{value2} = opt else unreachable
        return value2 == 20
    })
    test("variant pattern matching with multiple values work - 5", () => {
        var opt = PMMultiOpt.Some(10, 20)
        var Some{value2} = opt else return false;
        return value2 == 20
    })
    test("variant pattern matching with multiple values work - 6", () => {
        var opt = PMMultiOpt.Some(10, 20)
        var Some(value) = opt else unreachable
        return value == 10
    })
    test("variant pattern matching with multiple values work - 7", () => {
        var opt = PMMultiOpt.Some(10, 20)
        var Some(value) = opt else return false;
        return value == 10
    })
    test("variant pattern matching with if work - 1", () => {
        var s = PMOpt1.Some(78)
        if(var Some(value) = s) {
            return value == 78
        } else {
            return false;
        }
    })
    test("variant pattern matching with if work - 2", () => {
        var s = PMOpt1.None()
        if(var Some(value) = s) {
            return false
        } else {
            return true;
        }
    })
    test("pointer to variant dereferenced works in pattern match expression - 1", () => {
        var o = PMOpt1.Some(23)
        var ptr = &mut o
        var Some(value) = *ptr else -1
        return value == 23
    })
    test("pointer to variant dereferenced works in pattern match expression - 2", () => {
        var o = PMOpt1.Some(23)
        var ptr = &mut o
        var Some(value) = *ptr else return false
        return value == 23
    })
    test("pointer to variant dereferenced works in pattern match expression - 3", () => {
        var o = PMOpt1.Some(23)
        var ptr = &mut o
        var Some(value) = *ptr else unreachable
        return value == 23
    })
}

// ═══════════════════════════════════════════════════════════════════════
// Pattern matching through a reference member of a struct.
//
// Bug history: the LLVM backend used the address of the reference *slot*
// (a pointer to a pointer) as the variant pointer, instead of loading the
// reference. Extracted values were garbage (e.g. 0 instead of 42).
// ═══════════════════════════════════════════════════════════════════════

struct PMRefHolder {
    var value : &PMOpt1
}

func get_pm_via_ref_member(h : &PMRefHolder) : int {
    var Some(value) = h.value else unreachable
    return value
}

func get_pm_via_ref_member_if(h : &PMRefHolder) : int {
    if(h.value is PMOpt1.Some) {
        var Some(value) = h.value else unreachable
        return value
    }
    return -1
}

func get_pm_via_ref_member_switch(h : &PMRefHolder) : int {
    switch(h.value) {
        Some(value) => {
            return value
        }
        None() => {
            return -1
        }
    }
}

func test_pattern_match_reference_member() {
    test("pattern match through a reference member works - 1", () => {
        var o = PMOpt1.Some(10)
        var h = PMRefHolder { value : &o }
        return get_pm_via_ref_member(&h) == 10
    })
    test("pattern match through a reference member works - 2", () => {
        var o = PMOpt1.Some(20)
        var h = PMRefHolder { value : &o }
        return get_pm_via_ref_member_if(&h) == 20
    })
    test("pattern match through a reference member works - 3", () => {
        var o = PMOpt1.None()
        var h = PMRefHolder { value : &o }
        return get_pm_via_ref_member_if(&h) == -1
    })
    test("pattern match through a reference member works - 4", () => {
        var o = PMOpt1.Some(30)
        var h = PMRefHolder { value : &o }
        return get_pm_via_ref_member_switch(&h) == 30
    })
    test("pattern match through a reference member works - 5", () => {
        var o = PMOpt1.None()
        var h = PMRefHolder { value : &o }
        return get_pm_via_ref_member_switch(&h) == -1
    })
    test("pattern match in if body directly through reference member works", () => {
        var o = PMOpt1.Some(77)
        var h = PMRefHolder { value : &o }
        var Some(value) = h.value else unreachable
        return value == 77
    })
}

struct PMRefHolder2 {
    var holder : &PMRefHolder
}

func get_pm_via_ref_member_chain(h : &PMRefHolder2) : int {
    var Some(value) = h.holder.value else unreachable
    return value
}

func test_pattern_match_reference_member_chain() {
    test("pattern match through nested reference members in a chain works", () => {
        var o = PMOpt1.Some(55)
        var h = PMRefHolder { value : &o }
        var h2 = PMRefHolder2 { holder : &h }
        return get_pm_via_ref_member_chain(&h2) == 55
    })
}