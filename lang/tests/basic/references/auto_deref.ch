import "../../test.ch"

func take_int(i : int) : int {
    return i;
}

func give_ref(i : &int) : &int {
    return i;
}

func give_ref_bool(i : &bool) : &bool {
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

variant OptIntStorage8373 {
    Some(value : int)
    None
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
    test("auto dereferences when calling variants", () => {
        var i = 34343;
        var thing : OptIntStorage8373 = OptIntStorage8373.Some(give_ref(i))
        switch(thing) {
            Some(value) => {
                return value == 34343;
            }
            None => {
                return false;
            }
        }
    })
    test("referenced intN types are automatically de-referenced in switch statements", () => {
        var i = 33;
        switch(give_ref(i)) {
            33 => {
                return true;
            }
            default => {
                return false;
            }
        }
    })
    test("referenced boolean types are automatically de-referenced in switch statements", () => {
        var i = true;
        switch(give_ref_bool(i)) {
            true => {
                return true;
            }
            false => {
                return false;
            }
            default => {
                return false;
            }
        }
    })
    test("increment decrement can handle references - 1", () => {
        var i = 26
        var ref = give_ref(i)
        ref++
        return i == 27
    })
    test("increment decrement can handle references - 2", () => {
        var i = 23
        var ref = give_ref(i)
        ++ref
        return i == 24
    })
    test("increment decrement can handle references - 3", () => {
        var i = 26
        var ref = give_ref(i)
        ref--
        return i == 25
    })
    test("increment decrement can handle references - 4", () => {
        var i = 23
        var ref = give_ref(i)
        --ref
        return i == 22
    })
    test("increment decrement can handle references - 5", () => {
        var i = 26
        var ref = give_ref(i)
        return ref++ == 26
    })
    test("increment decrement can handle references - 6", () => {
        var i = 23
        var ref = give_ref(i)
        return ++ref == 24
    })
    test("increment decrement can handle references - 7", () => {
        var i = 26
        var ref = give_ref(i)
        return i-- == 26
    })
    test("increment decrement can handle references - 8", () => {
        var i = 23
        var ref = give_ref(i)
        return --i == 22
    })
}