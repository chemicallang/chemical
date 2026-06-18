comptime func comptime_give_char_by_offset(str : *char, offset : int) : char {
    return *(str + offset);
}

comptime func comptime_give_char_by_ptr_offset(str : *char, offset : int) : char {
    var x = str as *char
    x += offset
    return *x;
}

comptime func comptime_ptr_add_zero(str : *char) : char {
    return *(str + 0);
}

comptime func comptime_ptr_add_stride(str : *char) : char {
    return *(str + 2);
}

comptime func comptime_ptr_arith_if(str : *char, flag : bool) : char {
    if(flag) {
        return *(str + 1);
    } else {
        return *str;
    }
}

func give_char_by_offset(str : *char, offset : int) : char {
    return *(str + offset);
}

func give_char_by_ptr_offset(str : *char, offset : int) : char {
    var x = str as *char
    x += offset
    return *x;
}

func test_inc_dec_work_ptr1() : char {
    const str = "hello world"
    var ptr = str as *char;
    ptr++
    return *ptr
}

func test_inc_dec_work_ptr2() : char {
    const str = "hello world"
    var ptr = str as *char;
    ptr++
    ptr--
    return *ptr
}


func test_pointers_in_comptime() {
    test("comptime pointers work - 1", () => {
        return comptime { give_char_by_offset("hello", 0) } == give_char_by_offset("hello", 0)
    })
    test("comptime pointers work - 2", () => {
        return comptime { give_char_by_ptr_offset("hello", 0) } == give_char_by_ptr_offset("hello", 0)
    })
    test("comptime pointers work - 3", () => {
        return comptime { give_char_by_ptr_offset("hello", 1) } == give_char_by_ptr_offset("hello", 1)
    })
    test("comptime pointers work - 4", () => {
        return comptime { give_char_by_ptr_offset("hello", 4) } == give_char_by_ptr_offset("hello", 4)
    })
    test("comptime pointers work with increment decrement - 1", () => {
        return comptime { test_inc_dec_work_ptr1() } == test_inc_dec_work_ptr1()
    })
    test("comptime pointers work with increment decrement - 2", () => {
        return comptime { test_inc_dec_work_ptr2() } == test_inc_dec_work_ptr2()
    })

    // New tests for full comptime pointer arithmetic:
    test("comptime pointer arithmetic with direct comptime func - 1", () => {
        return comptime_give_char_by_offset("hello", 0) == 'h'
    })
    test("comptime pointer arithmetic with direct comptime func - 2", () => {
        return comptime_give_char_by_offset("hello", 1) == 'e'
    })
    test("comptime pointer arithmetic with direct comptime func - 3", () => {
        return comptime_give_char_by_offset("hello", 4) == 'o'
    })
    test("comptime pointer arithmetic with ptr offset - 1", () => {
        return comptime_give_char_by_ptr_offset("hello", 0) == 'h'
    })
    test("comptime pointer arithmetic with ptr offset - 2", () => {
        return comptime_give_char_by_ptr_offset("hello", 1) == 'e'
    })
    test("comptime pointer arithmetic with ptr offset - 3", () => {
        return comptime_give_char_by_ptr_offset("hello", 4) == 'o'
    })
    test("comptime pointer with if/else return - true branch", () => {
        return comptime_ptr_arith_if("hello", true) == 'e'
    })
    test("comptime pointer with if/else return - false branch", () => {
        return comptime_ptr_arith_if("hello", false) == 'h'
    })
    test("comptime pointer add with stride 2", () => {
        return comptime_ptr_add_stride("abcde") == 'c'
    })
    test("comptime pointer add with zero offset", () => {
        return comptime_ptr_add_zero("hello") == 'h'
    })
}
