func give_ref(i : &mut int) : &mut int {
    return i;
}

func test_deref_pass_func_call() {
    test("automatic dereferences when passing to var args parameter", () => {
        var i = 788
        var ref = give_ref(&mut i)
        var str : [20]char = []
        snprintf(&raw mut str[0], 19, "%d", *ref);
        return strncmp(&raw str[0], "788", 3) == 0
    })
}