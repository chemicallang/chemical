import "./test.ch"

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
}