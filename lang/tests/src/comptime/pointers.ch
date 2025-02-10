import "./test.ch"

@comptime
func give_comptime_char_by_offset(str : *char, offset : int) : char {
    return *(str + offset);
}

@comptime
func give_comptime_char_by_ptr_offset(str : *char, offset : int) : char {
    var x = str as *char
    x += offset
    return *x;
}

@comptime
func test_inc_dec_work_comptime_ptr1() : char {
    const str = "hello world"
    const ptr = str as *char;
    ptr++
    return *ptr
}

@comptime
func test_inc_dec_work_comptime_ptr2() : char {
    const str = "hello world"
    const ptr = str as *char;
    ptr++
    ptr--
    return *ptr
}


func test_pointers_in_comptime() {
    test("comptime pointers work - 1", () => {
        return give_comptime_char_by_offset("hello", 0) == 'h';
    })
    test("comptime pointers work - 2", () => {
        return give_comptime_char_by_ptr_offset("hello", 0) == 'h'
    })
    test("comptime pointers work - 3", () => {
        return give_comptime_char_by_ptr_offset("hello", 1) == 'e'
    })
    test("comptime pointers work - 4", () => {
        return give_comptime_char_by_ptr_offset("hello", 4) == 'o'
    })
    test("comptime pointers work with increment decrement - 1", () => {
        return test_inc_dec_work_comptime_ptr1() == 'e'
    })
    test("comptime pointers work with increment decrement - 2", () => {
        return test_inc_dec_work_comptime_ptr2() == 'h';
    })
}