func assign_to_addr(value : *mut int) {
    *value = 25
}

func take_addr_of(value : int) : int {
    assign_to_addr(&mut value)
    return value;
}

func <T> gen_assign_to_addr(value : *mut T) {
    if(T is short) {
        *value = 2
    } else if(T is int) {
        *value = 4
    } else if(T is bigint) {
        *value = 8
    } else {
        *value = 99
    }
}

func <T> gen_take_addr_of(value : T) : T {
    gen_assign_to_addr(&value)
    return value
}

func take_ref_to_r_val(value : &int) : bool {
    return value == 812
}

func pass_r_val_to_ref(value : int) : bool {
    return take_ref_to_r_val(value)
}

func test_parameters() {

    test("taking address of parameters works", () => {
        var result = take_addr_of(0)
        return result == 25
    })
    test("taking address of generic parameters works - 1", () => {
        var result = gen_take_addr_of<short>(22)
        return result == 2
    })
    test("taking address of generic parameters works - 2", () => {
        var result = gen_take_addr_of<int>(22)
        return result == 4
    })
    test("taking address of generic parameters works - 3", () => {
        var result = gen_take_addr_of<bigint>(22)
        return result == 8
    })
    test("passing r value function params to references works", () => {
        return pass_r_val_to_ref(812)
    })

}