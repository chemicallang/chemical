import "../test.ch"

func test_satisfies() {
    test("uchar types satisfy", () => {
        typealias T = uchar;
        return compiler::satisfies(T, T);
    })
    test("short types satisfy", () => {
        typealias T = short;
        return compiler::satisfies(T, T);
    })
    test("ushort types satisfy", () => {
        typealias T = ushort;
        return compiler::satisfies(T, T);
    })
    test("int types satisfy", () => {
        typealias T = int;
        return compiler::satisfies(T, T);
    })
    test("uint types satisfy", () => {
        typealias T = uint;
        return compiler::satisfies(T, T);
    })
    test("long types satisfy", () => {
        typealias T = long;
        return compiler::satisfies(T, T);
    })
    test("ulong types satisfy", () => {
        typealias T = ulong;
        return compiler::satisfies(T, T);
    })
    test("bigint types satisfy", () => {
        typealias T = bigint;
        return compiler::satisfies(T, T);
    })
    test("ubigint types satisfy", () => {
        typealias T = ubigint;
        return compiler::satisfies(T, T);
    })
}