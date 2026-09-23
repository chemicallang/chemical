type ubigint_typealias = ubigint;

func can_cast_to_intN_typealias(value : ushort) : ubigint_typealias {
    return value as ubigint_typealias
}

func can_implicit_cast_to_intN_typealias(value : ushort) : ubigint_typealias {
    return value;
}

func can_cast_from_intN_typealias(value : ubigint_typealias) : ushort {
    return value as ushort
}

func can_implicit_cast_from_intN_typealias(value : ubigint_typealias) : ushort {
    return value
}

func test_casts() {
    test("long value can be truncated", () => {
        var num1 = 30;
        var num2 : long = 16;
        return (num1 > (num2 as int));
    })
    test("int value can be extended", () => {
        var num1 : int = 30;
        var num2 : long = 16;
        return ((num1 as long) > num2);
    })
    test("cast without parenthesis works - 1", () => {
        var num1 = 30;
        var num2 : long = 16;
        return (num1 > num2 as int);
    })
    test("cast without parenthesis works - 2", () => {
        var num1 : int = 30;
        var num2 : long = 16;
        return (num1 as long > num2);
    })
    test("cast without parenthesis works - 3", () => {
        var d = 1.0 as double
        var fl = d as float
        var i = (fl as int) == 1
        return i;
    })
    test("cast double to an integer", () => {
        var num = 1.0 as double
        var num2 = num as int
        return num2 == 1;
    })
    test("cast float to a double", () => {
        var num = 1.0f as float
        var num2 = num as int
        return num2 == 1;
    })
    test("cast int to a double", () => {
        var num = 1
        var num2 = num as double
        return num2 == 1.0;
    })
    test("cast int to a float", () => {
        var num = 1
        var num2 = num as float
        return num2 == 1.0f;
    })
    test("cast a float to a double", () => {
        var fl = 1.0f
        var d = fl as double
        return ((d as int) == 1)
    })
    test("cast a double to a float", () => {
        var d = 1.0 as double
        var fl = d as float
        return ((fl as int) == 1)
    })
    test("cast a float to a double", () => {
        var fl = 1.0f
        return ((fl as double) == 1.0)
    })
    test("cast a double to a float", () => {
        var d = 1.0 as double
        return ((d as float) == 1.0f)
    })
    test("intN casts to intN typealias explicit", () => {
        const casted = can_cast_to_intN_typealias(16);
        return casted == 16;
    })
    test("intN casts to intN typealias implicit", () => {
        const casted = can_implicit_cast_to_intN_typealias(13);
        return casted == 13;
    })
    test("intN casts from intN typealias explicit", () => {
        const casted = can_cast_from_intN_typealias(26);
        return casted == 26;
    })
    test("intN casts from intN typealias implicit", () => {
        const casted = can_implicit_cast_from_intN_typealias(18);
        return casted == 18;
    })
    test("cast negative int to pointer (sign extension)", () => {
        var i : int = -1;
        var p = i as *void;
        // if sign extended correctly, it should be -1L (64-bit)
        // if zero extended (bug), it would be 4294967295L
        return (p as long) == -1L;
    })
}

// ---------------------------------------------------------------------------
// bool -> integer widening
//
// A `bool` is an i1 and can only be 0 or 1, so widening it to an integer must
// zero extend. Sign extending `true` produces -1. These casts are explicit, but
// the result still flows through the implicit cast at the return / assignment,
// which is where the LLVM backend used to emit `sext i1`.
// ---------------------------------------------------------------------------

func bool_cast_to_int(value : bool) : int {
    return value as int
}

func bool_cast_to_long(value : bool) : long {
    return value as long
}

func bool_cast_to_bigint(value : bool) : bigint {
    return value as bigint
}

func bool_cast_to_short(value : bool) : short {
    return value as short
}

func bool_cast_to_uint(value : bool) : uint {
    return value as uint
}

func test_bool_casts() {
    test("true cast to int is 1", () => {
        return (true as int) == 1
    })
    test("false cast to int is 0", () => {
        return (false as int) == 0
    })
    test("true returned as int is 1", () => {
        return bool_cast_to_int(true) == 1
    })
    test("true returned as long is 1 (no sign extension)", () => {
        return bool_cast_to_long(true) == 1L
    })
    test("true returned as bigint is 1 (no sign extension)", () => {
        return bool_cast_to_bigint(true) == 1
    })
    test("true returned as short is 1 (no sign extension)", () => {
        return bool_cast_to_short(true) == 1
    })
    test("true returned as uint is 1", () => {
        return bool_cast_to_uint(true) == 1u
    })
    test("false returned as long is 0", () => {
        return bool_cast_to_long(false) == 0L
    })
    test("bool widened by assignment is 1", () => {
        var b : bool = true
        var widened : long = b as long
        return widened == 1L
    })
}