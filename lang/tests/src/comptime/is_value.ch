func test_is_value() {
    test("char types are same", () => {
        type T = char;
        return T is char;
    })
    test("uchar types are same", () => {
        type T = uchar;
        return T is uchar;
    })
    test("short types are same", () => {
        type T = short;
        return T is short;
    })
    test("ushort types are same", () => {
        type T = ushort;
        return T is ushort;
    })
    test("int types are same", () => {
        type T = int;
        return T is int;
    })
    /** TODO !is doesn't work
    test("int types are not not same", () => {
        type T = int;
        return !(T !is int);
    })
    **/
    test("uint types are same", () => {
        type T = uint;
        return T is uint;
    })
    test("long types are same", () => {
        type T = long;
        return T is long;
    })
    test("ulong types are same", () => {
        type T = ulong;
        return T is ulong;
    })
    test("bigint types are same", () => {
        type T = bigint;
        return T is bigint;
    })
    test("ubigint types are same", () => {
        type T = ubigint;
        return T is ubigint;
    })
    test("char type is different from other intn", () => {
        type T = char;
        return !(T is int || T is uint || T is long || T is bigint || T is ubigint || T is uchar || T is short || T is ushort);
    })
    test("uchar type is different from other intN types", () => {
        type T = uchar;
        return !(T is int || T is uint || T is long || T is bigint || T is ubigint || T is char || T is short || T is ushort);
    })
    test("short type is different from other intN types", () => {
        type T = short;
        return !(T is int || T is uint || T is long || T is bigint || T is ubigint || T is char || T is uchar || T is ushort);
    })
    test("ushort type is different from other intN types", () => {
        type T = ushort;
        return !(T is int || T is uint || T is long || T is bigint || T is ubigint || T is char || T is uchar || T is short);
    })
    test("int type is different from other intN types", () => {
        type T = int;
        return !(T is ushort || T is uint || T is long || T is bigint || T is ubigint || T is char || T is uchar || T is short);
    })
    test("uint type is different from other intN types", () => {
        type T = uint;
        return !(T is ushort || T is int || T is long || T is ulong || T is bigint || T is ubigint || T is char || T is uchar || T is short);
    })
    test("long type is different from other intN types", () => {
        type T = long;
        return !(T is ushort || T is int || T is uint || T is ubigint || T is char || T is uchar || T is short);
    })
    test("ulong type is different from other intN types", () => {
        type T = ulong;
        return !(T is ushort || T is int || T is uint || T is bigint || T is char || T is uchar || T is short || T is long);
    })
    test("bigint type is different from other intN types", () => {
        type T = bigint;
        return !(T is ushort || T is int || T is uint || T is ulong || T is ubigint || T is char || T is uchar || T is short);
    })
    test("ubigint type is different from other intN types", () => {
        type T = ubigint;
        return !(T is ushort || T is int || T is uint || T is bigint || T is char || T is uchar || T is short || T is long);
    })
    test("bool type is different from other intN types", () => {
        type T = bool;
        return !(T is ushort || T is int || T is uint || T is bigint || T is char || T is uchar || T is short || T is long || T is ubigint);
    })
    test("same pointer types do match", () => {
        type T = *void
        type K = *char
        return T is *void && K is *char
    })
    /** TODO !is doesn't work
    test("different pointer types don't match", () => {
        type T = *void
        return (T !is *int && T !is *long && T !is *char)
    })
    **/
    test("not a single type is void", () => {
        type T = void
        return !(T is ushort || T is ulong || T is int || T is uint || T is bigint || T is char || T is uchar || T is short || T is long || T is *void)
    })
    /** TODO !is doesn't work
    test("double and float are not the same", () => {
        type T = double
        return T !is float
    })
    **/
    test("same array types match", () => {
        type T = int[]
        return T is int[]
    })
    /** TODO !is doesn't work
    test("different array types don't match", () => {
        type T = int[]
        return T !is double[]
    })
    **/
    test("all types satisfy any", () => {
        type A = int
        type B = long
        type C = char
        type D = double
        type E = bool
        type F = float
        type G = ulong
        type H = uint
        type I = bigint
        type J = ubigint
        type K = uchar
        type L = void
        type M = *void
        type N = int[]
        return A is any && B is any && C is any && D is any && E is any && F is any && G is any && H is any && I is any && J is any && K is any && L is any && M is any && N is any
    })
    test("any doesn't satisfy other types (though)", () => {
        type T = any
        return !(T is int || T is long || T is char || T is double || T is bool || T is float || T is ulong || T is uint || T is bigint || T is ubigint || T is uchar || T is void || T is *void || T is int[])
    })
}