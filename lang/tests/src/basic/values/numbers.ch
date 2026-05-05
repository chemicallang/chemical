func implicit_cast_ret_test() : bigint {
    var i : int = 55 as int;
    return i;
}

func implicit_cast_ret_test_2() : int {
    var i : bigint = 55 as bigint;
    return i;
}

enum NumberLiteralEnum : uint {
    OwnerPerm = 0o700,
    GroupPerm = 0o070,
    ByteMask = 0xff
}

func test_numbers() {
    test("unsigned int works", () => {
        var i : uint = 33;
        var w : uint = 33;
        return i == w;
    })
    test("short works", () => {
        var i : short = 22;
        var w : short = 22;
        return i == w;
    })
    test("unsigned short works", () => {
        var i : ushort = 44;
        var w : ushort = 44;
        return i == w;
    })
    test("long works", () => {
        var i : long = 777;
        var w : long = 777;
        return i == w;
    })
    test("unsigned long works", () => {
        var i : ulong = 777;
        var w : ulong = 777;
        return i == w;
    })
    test("big integer works", () => {
        var i : bigint = 888;
        var w : bigint = 888;
        return i == w;
    })
    test("unsigned big integer works", () => {
        var i : ubigint = 999;
        var w : ubigint = 999;
        return i == w;
    })
    test("unsigned int demotes int32 to uint", () => {
        var i : uint = 1;
        return i == 1;
    })
    test("short comparison demotes int32 to short", () => {
        var i : short = 1;
        return i == 1;
    })
    test("short comparison demotes int32 to short", () => {
        var i : short = 1;
        return i == 1;
    })
    test("ushort comparison demotes int32 to ushort", () => {
        var i : ushort = 1;
        return i == 1;
    })
    test("constant long is demoted to int32 for comparison with int32", () => {
        return 123 == 123L;
    })
    test("short can be assigned", () => {
        var i : short = 44;
        i = 33;
        return i == 33;
    })
    test("int can be assigned int max", () => {
        var i : int = 10;
        i = 2147483647;
        return i == 2147483647;
    })
    // TODO long is 32bit on 64bit windows
    // TODO to test long for 64bit a larger max value should be used
    // TODO a larger max value for long only should be used when it's bit size is larger than 32bit
    // TODO to check that we must develop sizeof macro, we'll also develop bitsizeof
    test("long can be assigned long max", () => {
        var i : long = 10;
        i = 2147483647;
        return i == 2147483647;
    })
    test("bigint can be assigned bigint max", () => {
        var i : bigint = 10;
        i = 9223372036854775807;
        return i == 9223372036854775807;
    })
    test("can promote int constants to float when compared with float variable", () => {
        var i : float = 10.0f;
        return i == 10;
    })
    test("can promote int constants to double when compared with double variable", () => {
        var i : double = 10.0;
        return i == 10;
    })
    test("can extend int n type to compare with a different bit type", () => {
        var i : short = 5;
        var w : int = 5;
        return i == w;
    })
    test("implicit casting at return from lesser to greater int n type", () => {
        return implicit_cast_ret_test() == 55;
    })
    test("implicit casting at return from greater to lesser int n type", () => {
        return implicit_cast_ret_test_2() == 55;
    })
    test("octal number parses correctly", () => {
        var oct = 0o700
        return oct as u32 == 448ui32
    })
    test("prefixed integer literals parse with the correct base", () => {
        var bin = 0b101101
        var oct = 0o755
        var hex = 0x2d
        var hex_upper = 0X2D
        return bin == 45 && oct == 493 && hex == 45 && hex_upper == 45
    })
    test("uppercase prefixed integer literals parse with the correct base", () => {
        var bin = 0B11110000
        var oct = 0O755
        var hex = 0XCAFE
        return bin == 240 && oct == 493 && hex == 51966
    })
    test("zero prefixed integer literals parse correctly", () => {
        var bin = 0b0
        var oct = 0o0
        var hex = 0x0
        return bin == 0 && oct == 0 && hex == 0
    })
    test("prefixed integer literals parse inside enum members", () => {
        return NumberLiteralEnum.OwnerPerm == 448 &&
            NumberLiteralEnum.GroupPerm == 56 &&
            NumberLiteralEnum.ByteMask == 255
    })
    test("prefixed unsigned suffix literals parse correctly", () => {
        var bin : uint = 0b101010u
        var oct : uint = 0o644u
        var hex : uint = 0xffffffffu
        return bin == 42u && oct == 420u && hex == 4294967295u
    })
    test("prefixed int suffix literals parse correctly", () => {
        var bin : int = 0b101010i
        var oct : int = 0o644i
        var hex : int = 0x7fffffffi
        return bin == 42i && oct == 420i && hex == 2147483647i
    })
    test("prefixed long suffix literals parse correctly", () => {
        var lower : ulong = 0xfffffffful
        var upper : ulong = 0xffffffffUL
        var mixed : ulong = 0XffffffffUl
        return lower == 4294967295UL && upper == 4294967295UL && mixed == 4294967295UL
    })
    test("prefixed signed long suffix literals parse correctly", () => {
        var lower : long = 0x7fffffffl
        var upper : long = 0x7fffffffL
        return lower == 2147483647L && upper == 2147483647L
    })
    test("prefixed fixed-width integer literals parse correctly", () => {
        var b8 : u8 = 0b10101010u8
        var o16 : u16 = 0o1234u16
        var h32 : u32 = 0xffffffffu32
        var i8v : i8 = 0x7fi8
        var i16v : i16 = 0o777i16
        return b8 == 170u8 && o16 == 668u16 && h32 == 4294967295u32 && i8v == 127i8 && i16v == 511i16
    })
    test("prefixed ui fixed-width integer literals parse correctly", () => {
        var b8 : u8 = 0b10101010ui8
        var o16 : u16 = 0o1234ui16
        var h32 : u32 = 0xffffffffui32
        var h64 : u64 = 0xffffffffffffffffui64
        return b8 == 170ui8 && o16 == 668ui16 && h32 == 4294967295ui32 && h64 == 18446744073709551615ui64
    })
    test("prefixed i fixed-width integer literals parse correctly", () => {
        var b8 : i8 = 0b01111111i8
        var o16 : i16 = 0o77777i16
        var h32 : i32 = 0x7fffffffi32
        var h64 : i64 = 0x7fffffffffffffffi64
        return b8 == 127i8 && o16 == 32767i16 && h32 == 2147483647i32 && h64 == 9223372036854775807i64
    })
    test("negative prefixed integer literals parse correctly", () => {
        var bin = -0b101101
        var oct = -0o755
        var hex = -0x2d
        return bin == -45 && oct == -493 && hex == -45
    })
    test("decimal suffixed integer literals still parse correctly", () => {
        var u : uint = 4294967295u
        var ul : ulong = 4294967295UL
        var ui : u32 = 4294967295u32
        var si : i16 = 32767i16
        return u == 4294967295u && ul == 4294967295UL && ui == 4294967295u32 && si == 32767i16
    })
    test("decimal ui suffixed integer literals still parse correctly", () => {
        var b : u8 = 255ui8
        var s : u16 = 65535ui16
        var i : u32 = 4294967295ui32
        var l : u64 = 18446744073709551615ui64
        return b == 255ui8 && s == 65535ui16 && i == 4294967295ui32 && l == 18446744073709551615ui64
    })
}