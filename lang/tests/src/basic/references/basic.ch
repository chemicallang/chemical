struct ReferencableStruct {
    var i : int
}

func get_i_by_deref_ptr_to_ref_struct(r : *ReferencableStruct) : int {
    return r.i;
}

func create_ref_struct_with_i(i : int) : ReferencableStruct {
    return ReferencableStruct { i : i }
}

type ref_struct_type_def_hidden_ref_test = &ReferencableStruct

@direct_init
struct AssignableReferencableStruct {
    var i : int
    @make
    func make(i : int) {
        return AssignableReferencableStruct { i : i }
    }
}

func assign_to_ref_struct(r : &mut AssignableReferencableStruct) {
    *r = AssignableReferencableStruct(9873)
}

func get_int_of_ass_ref_struct_ref(a : &AssignableReferencableStruct) : int {
    return a.i
}

func take_ref(r : &ReferencableStruct) : int {
    return r.i;
}

func pass_deref_ref_to_ref(r : &ReferencableStruct) : int {
    return take_ref(&*r)
}

struct member_deref_ref {
    var i : &int
    func derer_ref(&self) : &int {
        return &*i;
    }
}

func take_direct_struct(r : ReferencableStruct) : int {
    return r.i
}

func pass_deref_ref_to_direct(r : &ReferencableStruct) : int {
    return take_direct_struct(*r)
}

func in_between_ref_pass(r : &ReferencableStruct) : int {
    return take_ref(r);
}

func assign_to_passed_ref(r : &mut int) {
    *r = 434
}

struct ReferencableStructRef {
    var r : &ReferencableStruct
}

variant ReferencableStructOpt {
    Some(r : &ReferencableStruct)
    None()
}

struct ReferencableInt {
    var i : &mut int
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
    if(*a + 1 > b) {
        return true;
    }
    return false;
}

variant OptRefInt {
    Some(i : &mut int)
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
        return &raw ref;
    }

}

struct RefGiveStruct {
    var i : int
    func get_ref(&self) : &mut int {
        return &mut i;
    }
}

func copying_trivially_copyable_from_ref_works(c : &RefGiveStruct) : int {
    var d = *c
    return d.i
}

func test_references() {
    test("copying trivially copyable from by dereference of reference works", () => {
        var c = RefGiveStruct { i : 2394 }
        var r = copying_trivially_copyable_from_ref_works(&c)
        return r == 2394;
    })
    test("assignment to passed mutable reference via constructor works", () => {
        var j = AssignableReferencableStruct(22)
        assign_to_ref_struct(&mut j)
        return j.i == 9873
    })
    test("taking reference of a constructor call works", () => {
        return get_int_of_ass_ref_struct_ref(&AssignableReferencableStruct(5239938)) == 5239938
    })
    test("integer references are passed as function arguments automatically", () => {
        var i = 3
        return take_int_ref(&i) == 3;
    })
    test("integer references can be stored in structs automatically", () => {
        var i = 45;
        var r = ReferencableInt { i : &mut i }
        return take_int_ref(r.i) == 45;
    })
    test("integer references can be stored in variants automatically", () => {
        var i = 32
        var o = OptRefInt.Some(&mut i)
        switch(o) {
            Some(i) => {
                return take_int_ref(i) == 32
            }
            None() => {
                return false;
            }
        }
    })
    test("sending reference of returned struct from function call works", () => {
        return take_ref(&create_ref_struct_with_i(383645)) == 383645
    })
    test("struct can be passed to functions as reference", () => {
        var r = ReferencableStruct { i : 99 }
        return take_ref(&r) == 99;
    })
    test("struct can be passed in between functions as reference", () => {
        var r = ReferencableStruct { i : 98 }
        return in_between_ref_pass(&r) == 98;
    })
    test("references can be stored in structs", () => {
        var r = ReferencableStruct { i : 97 }
        var rr = ReferencableStructRef { r : &r }
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
        var rr = ReferencableStructOpt.Some(&z)
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
        *give_int_ref_32(&mut i) = 43
        return i == 43;
    })
    test("can assign to a struct type reference", () => {
        var p = ReferencableStruct { i : 32 }
        *give_ref_struct_ref(&mut p) = ReferencableStruct { i : 98 }
        return p.i == 98
    })
    test("assignment to passed reference works", () => {
        var i = 0;
        assign_to_passed_ref(&mut i)
        return i == 434;
    })
    test("index operator automatically sends a reference", () => {
        var arr : [3]int = [14, 74, 92]
        assign_to_passed_ref(&mut arr[1])
        return arr[1] == 434
    })
    test("integer r value can be passed to constant reference function", () => {
        return take_int_ref(789) == 789
    })
    test("can call methods on casted references", () => {
        var r = ReferenceCastedMethod { a : 32, b : 5 }
        return call_on_casted_ref<ReferenceCastedMethod>(&r) == 37
    })
    test("assignment to stored reference inside struct works", () => {
        var i = 0
        var r = ReferencableInt { i : &mut i }
        *r.i = 458
        return i == 458;
    })
    test("assignment to stored reference inside variant works", () => {
        var j = 0
        var opt = OptRefInt.Some(&mut j)
        var Some(i) = opt else unreachable
        *i = 873
        return j == 873
    })
    test("assignment to stored reference inside array works", () => {
        var i = 0
        var arr = [ give_int_ref_32(&mut i) ]
        *arr[0] = 827
        return i == 827
    })
    test("stored reference inside struct is returned properly according to type - 1", () => {
        var i = 32;
        var p = RefPassRet { ref : &i }
        return p.give1() == 32
    })
    test("stored reference inside struct is returned properly according to type - 2", () => {
        var i = 34;
        var p = RefPassRet { ref : &i }
        return *p.give2() == 34
    })
    test("stored reference inside struct is returned properly according to type - 3", () => {
        var i = 98;
        var p = RefPassRet { ref : &i }
        return *p.give3() == 98
    })
    test("calling method on stored reference passes self pointer correctly", () => {
        var thing = ReferenceCastedMethod { a : 23, b : 6 }
        var r = ref_mem_call { thing : &thing }
        return r.get_sum() == 29
    })
    test("taking reference of struct member through method works", () => {
        var s = RefGiveStruct { i : 33 }
        assign_to_passed_ref(s.get_ref())
        return s.i == 434
    })
    test("references can be compared in expressions", () => {
        var x = 33
        return compare_refs(&x, 18)
    })
    test("de-referencing a pointer to reference type means no de-reference", () => {
        var u = 234
        var j = &mut u
        assign_to_passed_ref(&mut *j)
        return u == 434
    })
    test("passing dereference of struct to a reference parameter type works", () => {
        var r = ReferencableStruct { i : 94853 }
        var i = pass_deref_ref_to_ref(&r)
        return i == 94853
    })
    test("passing dereference of struct to a reference parameter type works", () => {
        var r = ReferencableStruct { i : 94853 }
        var i = pass_deref_ref_to_ref(&r)
        return i == 94853
    })
    test("passing dereference of struct to a direct parameter type works", () => {
        var r = ReferencableStruct { i : 34603288 }
        var i = pass_deref_ref_to_direct(&r)
        return i == 34603288;
    })
    test("struct member ref of dref works", () => {
        var i = 34
        var m = member_deref_ref { i : &i }
        return *m.derer_ref() == 34
    })
    test("passing a pointer via address of a reference variable works", () => {
        var p = ReferencableStruct { i : 9238455 }
        var ref = &p
        return get_i_by_deref_ptr_to_ref_struct(&raw ref) == 9238455
    })
    test("passing a pointer via address of a reference variable works - 2", () => {
        var p = ReferencableStruct { i : 9238455 }
        // here its testing that compiler doesn't simply assume its not a reference by checking type without canonicalization
        var ref : ref_struct_type_def_hidden_ref_test = &p
        return get_i_by_deref_ptr_to_ref_struct(&raw ref) == 9238455
    })
}

struct MultiField {
    var a : int
    var b : int
    var c : int
}

func read_multi(r : &MultiField) : int {
    return r.a + r.b + r.c
}

func pass_multi(r : &MultiField) : int {
    return read_multi(r)
}

func test_multi_field_ref() {
    test("multi-field struct passed as ref between functions retains all fields", () => {
        var s = MultiField { a : 10, b : 20, c : 30 }
        return pass_multi(&s) == 60
    })
    test("multi-field struct ref read directly retains all fields", () => {
        var s = MultiField { a : 100, b : 200, c : 300 }
        return read_multi(&s) == 600
    })
}
