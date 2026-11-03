
using namespace std;

func test_conversions() {
    test("std::string to_i32", () => {
        var Ok(val) = string("123").to_i32() else return false
        return val == 123
    })
    test("std::string to_i32 negative", () => {
        var Ok(val) = string("-123").to_i32() else return false
        return val == -123
    })
    test("std::string to_i32 with plus", () => {
        var Ok(val) = string("+123").to_i32() else return false
        return val == 123
    })
    test("std::string to_i32 with spaces", () => {
        var Ok(val) = string("  123  ").to_i32() else return false
        return val == 123
    })
    test("std::string to_i64", () => {
        var Ok(val) = string("1234567890123").to_i64() else return false
        return val == 1234567890123
    })
    test("std::string to_u32", () => {
        var Ok(val) = string("123").to_u32() else return false
        return val == 123u
    })
    test("std::string to_u64", () => {
        var Ok(val) = string("1234567890123").to_u64() else return false
        return val == 1234567890123u64
    })
    test("std::string to_int", () => {
        var Ok(val) = string("123").to_int() else return false
        return val == 123
    })
    test("std::string to_uint", () => {
        var Ok(val) = string("123").to_uint() else return false
        return val == 123u
    })
    test("std::string to_float", () => {
        var Ok(f) = string("3.14").to_float() else return false
        return f > 3.13f && f < 3.15f
    })
    test("std::string to_double", () => {
        var Ok(d) = string("3.1415926535").to_double() else return false
        return d > 3.14159265 && d < 3.14159266
    })

    test("std::string_view to_i32", () => {
        var Ok(val) = string_view("123").to_i32() else return false
        return val == 123
    })
    test("std::string_view to_i32 negative", () => {
        var Ok(val) = string_view("-123").to_i32() else return false
        return val == -123
    })
    test("std::string_view to_i32 with spaces", () => {
        var Ok(val) = string_view("  456  ").to_i32() else return false
        return val == 456
    })
    test("std::string_view to_i32 slice", () => {
        var s = "abc123def"
        var raw_view = unsafe(string_view(s))
        var view = raw_view.subview(3, 6)
        var Ok(val) = view.to_i32() else return false
        return val == 123
    })
    test("std::string_view to_u32", () => {
        var Ok(val) = string_view("123").to_u32() else return false
        return val == 123u
    })
    test("std::string_view to_float", () => {
        var Ok(f) = string_view("3.14").to_float() else return false
        return f > 3.13f && f < 3.15f
    })
    test("std::string_view to_double", () => {
        var Ok(d) = string_view("3.1415926535").to_double() else return false
        return d > 3.14159265 && d < 3.14159266
    })

    // Error cases
    test("std::string to_i32 empty", () => {
        return string("").to_i32() is Result.Err
    })
    test("std::string to_i32 invalid", () => {
        return string("abc").to_i32() is Result.Err
    })
    test("std::string to_i32 trailing", () => {
        return string("123a").to_i32() is Result.Err
    })
    test("std::string to_float invalid", () => {
        return string("abc").to_float() is Result.Err
    })
    test("std::string to_double trailing", () => {
        return string("3.14abc").to_double() is Result.Err
    })
}

func test_conversion_overflow() {
    test("to_i8 rejects overflow", () => {
        return string("128").to_i8() is Result.Err && string("-129").to_i8() is Result.Err
    })
    test("to_i8 accepts limits", () => {
        var Ok(lo) = string("-128").to_i8() else return false
        var Ok(hi) = string("127").to_i8() else return false
        return lo == -128 && hi == 127
    })
    test("to_u8 rejects overflow", () => {
        return string("256").to_u8() is Result.Err
    })
    test("to_i16 rejects overflow", () => {
        return string("32768").to_i16() is Result.Err && string("-32769").to_i16() is Result.Err
    })
    test("to_u16 rejects overflow", () => {
        return string("65536").to_u16() is Result.Err
    })
    test("to_i32 rejects overflow", () => {
        return string("2147483648").to_i32() is Result.Err && string("-2147483649").to_i32() is Result.Err
    })
    test("to_i32 accepts limits", () => {
        var Ok(lo) = string("-2147483648").to_i32() else return false
        var Ok(hi) = string("2147483647").to_i32() else return false
        return lo == -2147483648 && hi == 2147483647
    })
    test("to_u32 rejects overflow", () => {
        return string("4294967296").to_u32() is Result.Err
    })
    test("to_i64 rejects overflow", () => {
        return string("9223372036854775808").to_i64() is Result.Err && string("-9223372036854775809").to_i64() is Result.Err
    })
    test("to_i64 accepts limits", () => {
        var Ok(lo) = string("-9223372036854775808").to_i64() else return false
        var Ok(hi) = string("9223372036854775807").to_i64() else return false
        return lo == -9223372036854775807i64 - 1i64 && hi == 9223372036854775807i64
    })
    test("to_u64 rejects overflow", () => {
        return string("18446744073709551616").to_u64() is Result.Err
    })
    test("to_u64 accepts max", () => {
        var Ok(v) = string("18446744073709551615").to_u64() else return false
        return v == 18446744073709551615u64
    })
    test("string_view to_i32 rejects overflow", () => {
        return string_view("2147483648").to_i32() is Result.Err
    })
}
