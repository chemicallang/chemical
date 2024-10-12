import "../../test.ch"

struct ReferencableStruct {
    var i : int
}

func take_ref(r : &ReferencableStruct) : int {
    return r.i;
}

func in_between_ref_pass(r : &ReferencableStruct) : int {
    return take_ref(r);
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

variant OptRefInt {
    Some(i : &int)
    None()
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
            OptRefInt.Some(i) => {
                return take_int_ref(i) == 32
            }
            OptRefInt.None() => {
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
    test("references can be stored in variants", () => {
        var z = ReferencableStruct { i : 97 }
        var rr = ReferencableStructOpt.Some(z)
        switch(rr) {
            ReferencableStructOpt.Some(r) => {
                return r.i == 97;
            }
            ReferencableStructOpt.None => {
                return false;
            }
        }
    })
    test("can assign to a int n reference type", () => {
        var i = 0;
        give_int_ref_32(i) = 43
        return i == 43;
    })
}