import "/test.ch"

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
    test("test long value can be truncated", () => {
        var num1 = 30;
        var num2 : long = 16;
        return (num1 > (num2 as int));
    })
    test("test int value can be extended", () => {
        var num1 : int = 30;
        var num2 : long = 16;
        return ((num1 as long) > num2);
    })
    test("test cast without parenthesis works - 1", () => {
        var num1 = 30;
        var num2 : long = 16;
        return (num1 > num2 as int);
    })
    test("test cast without parenthesis works - 2", () => {
        var num1 : int = 30;
        var num2 : long = 16;
        return (num1 as long > num2);
    })
    test("test cast without parenthesis works - 3", () => {
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
}