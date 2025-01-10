import "../../test.ch"

func take_int(i : int) : int {
    return i;
}

func give_ref(i : &int) : &int {
    return i;
}

func ret_auto_deref(i : &int) : int {
    return i
}

func test_auto_deref() {
    test("auto dereferences when value is being passed to function calls", () => {
        var i = 38
        return take_int(give_ref(i)) == 38
    })
    test("auto derefeences when a value is being returned", () => {
        var i = 323
        return ret_auto_deref(i) == 323
    })
}