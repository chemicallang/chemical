struct ReferencableStruct {
    var i : int
}

func take_ref(r : &ReferencableStruct) : int {
    return r.i;
}

func in_between_ref_pass(r : &ReferencableStruct) : int {
    return take_ref(r);
}

func assign_to_passed_ref(r : &mut int) {
    r = 434
}

struct ReferencableStructRef {
    var r : &ReferencableStruct
}

variant ReferencableStructOpt {
    Some(r : &ReferencableStruct)
    None()
}

struct ReferencableInt {
    var i : &int
}

func take_int_ref(a : &int) : int {
    return a;
}

func give_int_ref_32(a : &mut int) : &mut int {
    return a;
}

func give_ref_struct_ref(r : &mut ReferencableStruct) : &mut ReferencableStruct {
    return r;
}

variant OptRefInt {
    Some(i : &int)
    None()
}

struct ReferenceCastedMethod {
    var a : int
    var b : int
    func sum(&self) : int {
        return a + b;
    }
}

func <T> call_on_casted_ref(ref : &T) : int {
    if(T is ReferenceCastedMethod) {
        const ref2 = ref as &ReferenceCastedMethod
        return ref2.sum()
    } else {
        return 0;
    }
}

func test_references() {
    test("integer references are passed as function arguments automatically", () => {
        var i = 3
        return take_int_ref(i) == 3;
    })
    test("integer references can be stored in structs automatically", () => {
        var i = 45;
        var r = ReferencableInt { i : i }
        return take_int_ref(r.i) == 45;
    })
    test("integer references can be stored in variants automatically", () => {
        var i = 32
        var o = OptRefInt.Some(i)
        switch(o) {
            Some(i) => {
                return take_int_ref(i) == 32
            }
            None() => {
                return false;
            }
        }
    })
    test("struct can be passed to functions as reference", () => {
        var r = ReferencableStruct { i : 99 }
        return take_ref(r) == 99;
    })
    test("struct can be passed in between functions as reference", () => {
        var r = ReferencableStruct { i : 98 }
        return in_between_ref_pass(r) == 98;
    })
    test("references can be stored in structs", () => {
        var r = ReferencableStruct { i : 97 }
        var rr = ReferencableStructRef { r : r }
        return rr.r.i == 97
    })
    test("accessing members through deref works", () => {
        var r = ReferencableStruct { i : 32 }
        var j = &r;
        var k = *j;
        return k.i == 32;
    })
    test("accessing members through double deref works", () => {
        var r = ReferencableStruct { i : 32 }
        var j = &r;
        var k = &j
        var i = **k
        return i.i == 32;
    })
    test("references can be stored in variants", () => {
        var z = ReferencableStruct { i : 97 }
        var rr = ReferencableStructOpt.Some(z)
        switch(rr) {
            Some(r) => {
                return r.i == 97;
            }
            None => {
                return false;
            }
        }
    })
    test("can assign to a int n reference", () => {
        var i = 0;
        give_int_ref_32(i) = 43
        return i == 43;
    })
    test("can assign to a struct type reference", () => {
        var p = ReferencableStruct { i : 32 }
        give_ref_struct_ref(p) = ReferencableStruct { i : 98 }
        return p.i == 98
    })
    test("assignment to passed reference works", () => {
        var i = 0;
        assign_to_passed_ref(i)
        return i == 434;
    })
    test("integer r value can be passed to constant reference function", () => {
        return take_int_ref(789) == 789
    })
    test("can call methods on casted references", () => {
        var r = ReferenceCastedMethod { a : 32, b : 5 }
        return call_on_casted_ref<ReferenceCastedMethod>(r) == 37
    })
    test("assignment to stored reference inside struct works", () => {
        var i = 0
        var r = ReferencableInt { i : i }
        r.i = 458
        return i == 458;
    })
    test("assignment to stored reference inside variant works", () => {
        var j = 0
        var opt = OptRefInt.Some(j)
        var Some(i) = opt else unreachable
        i = 873
        return j == 873
    })
    test("assignment to stored reference inside array works", () => {
        var i = 0
        var arr = [ give_int_ref_32(i) ]
        arr[0] = 827
        return i == 827
    })
}