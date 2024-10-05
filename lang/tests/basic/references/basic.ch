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

func test_references() {
    test("struct can be passed to functions as reference", () => {
        var r = ReferencableStruct { i : 99 }
        return take_ref(r) == 99;
    })
    test("struct can be passed in between functions as reference", () => {
        var r = ReferencableStruct { i : 98 }
        return in_between_ref_pass(r) == 98;
    })
}