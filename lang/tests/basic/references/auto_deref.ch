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

struct IntStorageT3232 {
    var i : int
}

func create_IntStorageT3232(i : &int) : IntStorageT3232 {
    return IntStorageT3232 { i : i }
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
    test("auto dereferences when storing into a struct", () => {
        var i = 234
        var s = create_IntStorageT3232(i);
        return s.i == 234
    })
    test("auto dereferences when storing into an array", () => {
        var i = 356
        var s : int[] = { give_ref(i) }
        return s[0] == 356
    })
}