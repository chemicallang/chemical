import "../../test.ch"

struct ReferencableStruct {
    var i : int
}

func take_ref(r : ReferencableStruct&) : int {
    return r.i;
}

func in_between_ref_pass(r : ReferencableStruct&) : int {
    return take_ref(r);
}

struct ReferencableStructRef {
    var r : ReferencableStruct&
}

variant ReferencableStructOpt {
    Some(r : ReferencableStruct&)
    None()
}

func test_references() {
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
}