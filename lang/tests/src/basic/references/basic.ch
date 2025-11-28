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
    return *a;
}

func give_int_ref_32(a : &mut int) : &mut int {
    return a;
}

func give_ref_struct_ref(r : &mut ReferencableStruct) : &mut ReferencableStruct {
    return r;
}

func compare_refs(a : &int, b : int) : bool {
    // a > b is an expression, it returns the type reference type
    // which further is dereferenced, so output in C would be *(*(a + 1) > b) which is wrong
    // it should understand that comparison with reference means unwrap the reference type
    // when determining type of the expression
    if(a + 1 > b) {
        return true;
    }
    return false;
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

struct ref_mem_call {
    var thing : &ReferenceCastedMethod
    func get_sum(&self) : int {
        return thing.sum()
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

struct RefPassRet {

    var ref : &int

    func give1(&self) : int {
        return *ref;
    }

    func give2(&self) : &int {
        return ref;
    }

    func give3(&self) : *int {
        return &ref;
    }

}

struct RefGiveStruct {
    var i : int
    func get_ref(&self) : &mut int {
        return i;
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
    test("index operator automatically sends a reference", () => {
        var arr : [3]int = [14, 74, 92]
        assign_to_passed_ref(arr[1])
        return arr[1] == 434
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
    test("stored reference inside struct is returned properly according to type - 1", () => {
        var i = 32;
        var p = RefPassRet { ref : i }
        return p.give1() == 32
    })
    test("stored reference inside struct is returned properly according to type - 2", () => {
        var i = 34;
        var p = RefPassRet { ref : i }
        return *p.give2() == 34
    })
    test("stored reference inside struct is returned properly according to type - 3", () => {
        var i = 98;
        var p = RefPassRet { ref : i }
        return *p.give3() == 98
    })
    test("calling method on stored reference passes self pointer correctly", () => {
        var thing = ReferenceCastedMethod { a : 23, b : 6 }
        var r = ref_mem_call { thing : thing }
        return r.get_sum() == 29
    })
    test("taking reference of struct member through method works", () => {
        var s = RefGiveStruct { i : 33 }
        assign_to_passed_ref(s.get_ref())
        return s.i == 434
    })
    test("references can be compared in expressions", () => {
        var x = 33
        return compare_refs(x, 18)
    })
    test("de-referencing a pointer to reference type means no de-reference", () => {
        var u = 234
        var j = &mut u
        assign_to_passed_ref(*j)
        return u == 434
    })
}