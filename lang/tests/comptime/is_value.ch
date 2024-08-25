import "../test.ch"

func test_is_value() {
    test("char types are same", () => {
        typealias T = char;
        return T is char;
    })
    test("uchar types are same", () => {
        typealias T = uchar;
        return T is uchar;
    })
    test("short types are same", () => {
        typealias T = short;
        return T is short;
    })
    test("ushort types are same", () => {
        typealias T = ushort;
        return T is ushort;
    })
    test("int types are same", () => {
        typealias T = int;
        return T is int;
    })
    test("int types are not not same", () => {
        typealias T = int;
        return !(T !is int);
    })
    test("uint types are same", () => {
        typealias T = uint;
        return T is uint;
    })
    test("long types are same", () => {
        typealias T = long;
        return T is long;
    })
    test("ulong types are same", () => {
        typealias T = ulong;
        return T is ulong;
    })
    test("bigint types are same", () => {
        typealias T = bigint;
        return T is bigint;
    })
    test("ubigint types are same", () => {
        typealias T = ubigint;
        return T is ubigint;
    })
    test("char type is different from other intn", () => {
        typealias T = char;
        return !(T is int || T is uint || T is long || T is bigint || T is ubigint || T is uchar || T is short || T is ushort);
    })
    test("uchar type is different from other intN types", () => {
        typealias T = uchar;
        return !(T is int || T is uint || T is long || T is bigint || T is ubigint || T is char || T is short || T is ushort);
    })
    test("short type is different from other intN types", () => {
        typealias T = short;
        return !(T is int || T is uint || T is long || T is bigint || T is ubigint || T is char || T is uchar || T is ushort);
    })
    test("ushort type is different from other intN types", () => {
        typealias T = ushort;
        return !(T is int || T is uint || T is long || T is bigint || T is ubigint || T is char || T is uchar || T is short);
    })
    test("int type is different from other intN types", () => {
        typealias T = int;
        return !(T is ushort || T is uint || T is long || T is bigint || T is ubigint || T is char || T is uchar || T is short);
    })
    test("uint type is different from other intN types", () => {
        typealias T = uint;
        return !(T is ushort || T is int || T is long || T is ulong || T is bigint || T is ubigint || T is char || T is uchar || T is short);
    })
    test("long type is different from other intN types", () => {
        typealias T = long;
        return !(T is ushort || T is int || T is uint || T is ubigint || T is char || T is uchar || T is short);
    })
    test("ulong type is different from other intN types", () => {
        typealias T = ulong;
        return !(T is ushort || T is int || T is uint || T is bigint || T is char || T is uchar || T is short || T is long);
    })
    test("bigint type is different from other intN types", () => {
        typealias T = bigint;
        return !(T is ushort || T is int || T is uint || T is ulong || T is ubigint || T is char || T is uchar || T is short);
    })
    test("ubigint type is different from other intN types", () => {
        typealias T = ubigint;
        return !(T is ushort || T is int || T is uint || T is bigint || T is char || T is uchar || T is short || T is long);
    })
    test("long is either an int or a big int but not both", () => {
        typealias T = long;
        return (T is int || T is bigint) && !(T is int && T is bigint)
    })
    test("ulong is either an uint or a ubigint but not both", () => {
        typealias T = ulong;
        return (T is uint || T is ubigint) && !(T is uint && T is ubigint)
    })
    test("same pointer types do match", () => {
        typealias T = void*
        typealias K = char*
        return T is void* && K is char*
    })
    test("different pointer types don't match", () => {
        typealias T = void*
        return (T !is int* && T !is long* && T !is char*)
    })
    test("not a single type is void", () => {
        typealias T = void
        return !(T is ushort || T is ulong || T is int || T is uint || T is bigint || T is char || T is uchar || T is short || T is long || T is void*)
    })

}