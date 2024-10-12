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
    test("unsigned types (not) satisfy each other - 0", () => {
        typealias T = bool;
        typealias U = uint
        return !compiler::satisfies(T, U) && !compiler::satisfies(U, T)
    })
    test("unsigned types (not) satisfy each other - 1", () => {
        typealias T = uchar;
        typealias U = uint
        return !compiler::satisfies(T, U) && !compiler::satisfies(U, T)
    })
    test("unsigned types satisfy each other - 2", () => {
        typealias T = ushort;
        typealias U = uint
        return compiler::satisfies(T, U) && compiler::satisfies(U, T)
    })
    test("unsigned types satisfy each other - 3", () => {
        typealias T = ubigint;
        typealias U = ulong
        return compiler::satisfies(T, U) && compiler::satisfies(U, T)
    })
    test("unsigned types satisfy each other - 4", () => {
        typealias T = ulong;
        typealias U = ushort
        return compiler::satisfies(T, U) && compiler::satisfies(U, T)
    })
    test("unsigned types satisfy each other - 5", () => {
        typealias T = ushort;
        typealias U = ubigint
        return compiler::satisfies(T, U) && compiler::satisfies(U, T)
    })
    test("signed types (not) satisfy each other - 0", () => {
        typealias T = bool;
        typealias U = int
        return !compiler::satisfies(T, U) && !compiler::satisfies(U, T)
    })
    test("signed types (not) satisfy each other - 1", () => {
        typealias T = char;
        typealias U = int
        return !compiler::satisfies(T, U) && !compiler::satisfies(U, T)
    })
    test("signed types satisfy each other - 2", () => {
        typealias T = short;
        typealias U = int
        return compiler::satisfies(T, U) && compiler::satisfies(U, T)
    })
    test("signed types satisfy each other - 3", () => {
        typealias T = bigint;
        typealias U = long
        return compiler::satisfies(T, U) && compiler::satisfies(U, T)
    })
    test("signed types satisfy each other - 4", () => {
        typealias T = long;
        typealias U = short
        return compiler::satisfies(T, U) && compiler::satisfies(U, T)
    })
    test("signed types satisfy each other - 5", () => {
        typealias T = short;
        typealias U = bigint
        return compiler::satisfies(T, U) && compiler::satisfies(U, T)
    })
}