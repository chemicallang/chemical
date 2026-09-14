// Copyright (c) Chemical Language Foundation 2026.
//
// Regression tests for integer width normalization in the interpreter.
//
// The AST interpreter historically stored every integer as a full 64-bit value
// and never narrowed it to its declared type. The C/LLVM backends truncate (and
// sign extend for signed types) on every store, argument pass, return and cast,
// so programs that rely on fixed-width wraparound (hashing, checksums, crypto,
// image/audio processing) diverged when interpreted.
//
// These tests pin the backend-matching behaviour for:
//   * variable initialization
//   * direct assignment
//   * struct field assignment
//   * function arguments
//   * function returns
//   * `as` casts
//   * fixed-width literal suffixes
//
// They run in both compiled (`./scripts/test.sh --tcc`) and interpreted
// (`./scripts/test.sh --tcc --interpret`) modes.

struct InterpNarrow {
    var u : u8
    var i : i8
    var s : u16
}

func interp_take_u8(x : u8) : int {
    return x as int
}

func interp_return_u8() : u8 {
    return 300
}

func interp_init_u8() : int {
    var x : u8 = 300
    return x as int
}

func interp_assign_u8() : int {
    var x : u8 = 0
    x = 300
    return x as int
}

func interp_assign_i8() : int {
    var x : i8 = 0
    x = 200
    return x as int
}

func interp_assign_u16() : int {
    var x : u16 = 0
    x = 70000
    return x as int
}

func interp_compound_u8() : int {
    var x : u8 = 200
    x += 100
    return x as int
}

func interp_field_u8() : int {
    var n = InterpNarrow { u : 0, i : 0, s : 0 }
    n.u = 300
    return n.u as int
}

func interp_field_i8() : int {
    var n = InterpNarrow { u : 0, i : 0, s : 0 }
    n.i = 200
    return n.i as int
}

func interp_field_u16() : int {
    var n = InterpNarrow { u : 0, i : 0, s : 0 }
    n.s = 70000
    return n.s as int
}

func interp_cast_u8() : int {
    var x = 300 as u8
    return x as int
}

func interp_array_u8() : int {
    var a : [2]u8 = [0, 0]
    a[0] = 300
    return a[0] as int
}

func interp_fixed_width_literals() : int {
    // Guards the parser fix where an unsigned u8 literal was sign extended
    // ((char) 170 -> -86) instead of zero extended ((unsigned char) 170 -> 170).
    var b8 : u8 = 0b10101010u8
    if(b8 != 170u8) { return 1 }
    var o16 : u16 = 0o1234u16
    if(o16 != 668u16) { return 2 }
    var h32 : u32 = 0xffffffffu32
    if(h32 != 4294967295u32) { return 3 }
    var i8v : i8 = 0x7fi8
    if(i8v != 127i8) { return 4 }
    var i16v : i16 = 0o777i16
    if(i16v != 511i16) { return 5 }
    return 0
}

func interp_ui_width_literals() : int {
    var b : u8 = 255ui8
    if(b != 255ui8) { return 1 }
    var s : u16 = 65535ui16
    if(s != 65535ui16) { return 2 }
    var i : u32 = 4294967295ui32
    if(i != 4294967295ui32) { return 3 }
    var l : u64 = 18446744073709551615ui64
    if(l != 18446744073709551615ui64) { return 4 }
    return 0
}

func test_interp_int_width() {
    test("interpreter: u8 initializer truncates to 44", () => {
        return interp_init_u8() == 44
    })
    test("interpreter: u8 assignment truncates to 44", () => {
        return interp_assign_u8() == 44
    })
    test("interpreter: i8 assignment truncates to -56", () => {
        return interp_assign_i8() == -56
    })
    test("interpreter: u16 assignment truncates to 4464", () => {
        return interp_assign_u16() == 4464
    })
    test("interpreter: u8 compound assignment wraps to 44", () => {
        return interp_compound_u8() == 44
    })
    test("interpreter: u8 field assignment truncates to 44", () => {
        return interp_field_u8() == 44
    })
    test("interpreter: i8 field assignment truncates to -56", () => {
        return interp_field_i8() == -56
    })
    test("interpreter: u16 field assignment truncates to 4464", () => {
        return interp_field_u16() == 4464
    })
    test("interpreter: u8 argument truncates to 44", () => {
        return interp_take_u8(300) == 44
    })
    test("interpreter: u8 return truncates to 44", () => {
        return interp_return_u8() as int == 44
    })
    test("interpreter: cast to u8 truncates to 44", () => {
        return interp_cast_u8() == 44
    })
    test("interpreter: u8 array element truncates to 44", () => {
        return interp_array_u8() == 44
    })
    test("interpreter: fixed-width unsigned literals stay unsigned", () => {
        return interp_fixed_width_literals() == 0
    })
    test("interpreter: ui fixed-width literals stay unsigned", () => {
        return interp_ui_width_literals() == 0
    })
}
