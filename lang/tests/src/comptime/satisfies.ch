import "/test.ch"

struct EmptySatisfies {

}

struct BaseSatisfies11 {
    var x : int
    var y : int
    var z : int
}

struct DerivedSatisfies11 : BaseSatisfies11 {

}

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
    test("unsigned types satisfy each other - 1", () => {
        typealias T = uchar;
        typealias U = uint
        return compiler::satisfies(T, U) && compiler::satisfies(U, T)
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
    test("signed types satisfy each other - 1", () => {
        typealias T = char;
        typealias U = int
        return compiler::satisfies(T, U) && compiler::satisfies(U, T)
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
    test("signed types do not satisfy unsigned and vice versa - 1", () => {
        typealias T = int
        typealias U = uint
        return !compiler::satisfies(T, U) && !compiler::satisfies(U, T)
    })
    test("signed types do not satisfy unsigned and vice versa - 2", () => {
        typealias T = long
        typealias U = ulong
        return !compiler::satisfies(T, U) && !compiler::satisfies(U, T)
    })
    test("signed types do not satisfy unsigned and vice versa - 3", () => {
        typealias T = bigint
        typealias U = ubigint
        return !compiler::satisfies(T, U) && !compiler::satisfies(U, T)
    })
    test("signed types do not satisfy unsigned and vice versa - 4", () => {
        typealias T = short
        typealias U = ushort
        return !compiler::satisfies(T, U) && !compiler::satisfies(U, T)
    })
    test("signed types do not satisfy unsigned and vice versa - 5", () => {
        typealias T = char
        typealias U = uchar
        return !compiler::satisfies(T, U) && !compiler::satisfies(U, T)
    })
    test("signed types do not satisfy unsigned and vice versa - 6", () => {
        typealias T = long
        typealias U = uint
        return !compiler::satisfies(T, U) && !compiler::satisfies(U, T)
    })
    test("signed types do not satisfy unsigned and vice versa - 7", () => {
        typealias T = ubigint
        typealias U = int
        return !compiler::satisfies(T, U) && !compiler::satisfies(U, T)
    })
    test("signed types do not satisfy unsigned and vice versa - 7", () => {
        typealias T = uint
        typealias U = bigint
        return !compiler::satisfies(T, U) && !compiler::satisfies(U, T)
    })
    test("signed types do not satisfy unsigned and vice versa - 8", () => {
        typealias T = ushort
        typealias U = bigint
        return !compiler::satisfies(T, U) && !compiler::satisfies(U, T)
    })
    test("signed types do not satisfy unsigned and vice versa - 9", () => {
        typealias T = ulong
        typealias U = short
        return !compiler::satisfies(T, U) && !compiler::satisfies(U, T)
    })
    test("signed types do not satisfy unsigned and vice versa - 9", () => {
        typealias T = long
        typealias U = ushort
        return !compiler::satisfies(T, U) && !compiler::satisfies(U, T)
    })
    test("signed types do not satisfy unsigned and vice versa - 10", () => {
        typealias T = ubigint
        typealias U = long
        return !compiler::satisfies(T, U) && !compiler::satisfies(U, T)
    })
    test("struct types satisfy", () => {
        return compiler::satisfies(BaseSatisfies11, BaseSatisfies11)
    })
    test("derived struct types satisfy base but not the other way around", () => {
        return compiler::satisfies(BaseSatisfies11, DerivedSatisfies11) && !compiler::satisfies(DerivedSatisfies11, BaseSatisfies11)
    })
    test("intN pointer types satisfy", () => {
        typealias T = *int
        return compiler::satisfies(T, T)
    })
    test("struct pointer types satisfy", () => {
        typealias T = *BaseSatisfies11
        return compiler::satisfies(T, T)
    })
    test("derived struct pointer types satisfy base struct pointer types but not the other way around", () => {
        typealias T = *DerivedSatisfies11
        typealias U = *BaseSatisfies11
        return compiler::satisfies(U, T) && !compiler::satisfies(T, U)
    })
    test("immutable pointer types do not satisfy mutable pointer types", () => {
        typealias T = *mut int
        typealias U = *int
        return !compiler::satisfies(T, U)
    })
    test("mutable pointer types satisfy immutable pointer types", () => {
        typealias T = *mut int
        typealias U = *int
        return compiler::satisfies(U, T)
    })
    test("immutable pointer struct types do not satisfy mutable pointer struct types", () => {
        typealias T = *mut BaseSatisfies11
        typealias U = *BaseSatisfies11
        return !compiler::satisfies(T, U)
    })
    test("mutable pointer struct types satisfy immutable pointer struct types", () => {
        typealias T = *mut BaseSatisfies11
        typealias U = *BaseSatisfies11
        return compiler::satisfies(U, T)
    })
    test("intN pointer types NOT satisfy other intN pointer types - 1", () => {
        typealias T = *int
        typealias U = *long
        return !compiler::satisfies(T, U)
    })
    test("intN pointer types NOT satisfy other intN pointer types - 2", () => {
        typealias T = *int
        typealias U = *long
        return !compiler::satisfies(T, U)
    })
    test("intN pointer types NOT satisfy other intN pointer types - 3", () => {
        typealias T = *uint
        typealias U = *ulong
        return !compiler::satisfies(T, U)
    })
    test("intN pointer types NOT satisfy other intN pointer types - 4", () => {
        typealias T = *uint
        typealias U = *ulong
        return !compiler::satisfies(T, U)
    })
    test("intN pointer types satisfy other intN pointer types - 5", () => {
        typealias T = *int
        typealias U = *ulong
        return !compiler::satisfies(T, U)
    })
    test("intN pointer types satisfy other intN pointer types - 6", () => {
        typealias T = *uint
        typealias U = *long
        return !compiler::satisfies(T, U)
    })
    test("intN reference types satisfy", () => {
        typealias T = &int
        return compiler::satisfies(T, T)
    })
    test("struct reference types satisfy", () => {
        typealias T = &BaseSatisfies11
        return compiler::satisfies(T, T)
    })
    test("derived struct reference types satisfy base struct reference types but not the other way around", () => {
        typealias T = &DerivedSatisfies11
        typealias U = &BaseSatisfies11
        return compiler::satisfies(U, T) && !compiler::satisfies(T, U)
    })
    test("immutable reference types do not satisfy mutable reference types", () => {
        typealias T = &mut int
        typealias U = &int
        return !compiler::satisfies(T, U)
    })
    test("mutable reference types satisfy immutable reference types", () => {
        typealias T = &mut int
        typealias U = &int
        return compiler::satisfies(U, T)
    })
    test("immutable reference struct types do not satisfy mutable reference struct types", () => {
        typealias T = &mut BaseSatisfies11
        typealias U = &BaseSatisfies11
        return !compiler::satisfies(T, U)
    })
    test("mutable reference struct types satisfy immutable reference struct types", () => {
        typealias T = &mut BaseSatisfies11
        typealias U = &BaseSatisfies11
        return compiler::satisfies(U, T)
    })
    test("intN reference types satisfy other intN reference types - 1", () => {
        typealias T = &int
        typealias U = &long
        return compiler::satisfies(T, U)
    })
    test("intN reference types satisfy other intN reference types - 2", () => {
        typealias T = &int
        typealias U = &long
        return compiler::satisfies(T, U)
    })
    test("intN reference types satisfy other intN reference types - 3", () => {
        typealias T = &uint
        typealias U = &ulong
        return compiler::satisfies(T, U)
    })
    test("intN reference types satisfy other intN reference types - 4", () => {
        typealias T = &uint
        typealias U = &ulong
        return compiler::satisfies(T, U)
    })
    test("intN reference types do not satisfy other intN reference types - 5", () => {
        typealias T = &int
        typealias U = &ulong
        return !compiler::satisfies(T, U)
    })
    test("intN reference types do not satisfy other intN reference types - 6", () => {
        typealias T = &uint
        typealias U = &long
        return !compiler::satisfies(T, U)
    })
    test("direct int n types do not satisfy reference int n types without backing storage - 1", () => {
        typealias T = &int
        typealias U = int
        return !compiler::satisfies(T, U)
    })
    test("direct int n types do not satisfy reference int n types without backing storage - 2", () => {
        typealias T = &int
        typealias U = long
        return !compiler::satisfies(T, U)
    })
    test("direct int n types do not satisfy reference int n types without backing storage - 3", () => {
        typealias T = &uint
        typealias U = uint
        return !compiler::satisfies(T, U)
    })
    test("direct int n types do not satisfy reference int n types without backing storage - 4", () => {
        typealias T = &uint
        typealias U = ulong
        return !compiler::satisfies(T, U)
    })
    test("direct int n types satisfy reference int n types with backing storage - 1", () => {
        typealias T = &int
        var U : int = 0
        return compiler::satisfies(T, U)
    })
    test("direct int n types satisfy reference int n types with backing storage - 2", () => {
        typealias T = &int
        var U : long = 0
        return compiler::satisfies(T, U)
    })
    test("direct int n types satisfy reference int n types with backing storage - 3", () => {
        typealias T = &uint
        var U : uint = 0
        return compiler::satisfies(T, U)
    })
    test("direct int n types satisfy reference int n types with backing storage - 4", () => {
        typealias T = &uint
        var U : ulong = 0
        return compiler::satisfies(T, U)
    })
    test("constant int n declarations do not qualify as backing storage for reference types - 1", () => {
        typealias T = &int
        const U : int = 0
        return !compiler::satisfies(T, U)
    })
    test("constant int n declarations do not qualify as backing storage for reference types - 2", () => {
        typealias T = &int
        const U : long = 0
        return !compiler::satisfies(T, U)
    })
    test("constant int n declarations do not qualify as backing storage for reference types - 3", () => {
        typealias T = &uint
        const U : uint = 0
        return !compiler::satisfies(T, U)
    })
    test("constant int n declarations do not qualify as backing storage for reference types - 4", () => {
        typealias T = &uint
        const U : ulong = 0
        return !compiler::satisfies(T, U)
    })
    test("direct structs always satisfy reference immutable types", () => {
        typealias T = &EmptySatisfies
        typealias U = EmptySatisfies
        return compiler::satisfies(T, U)
    })
    test("direct structs always satisfy reference mutable types", () => {
        typealias T = &mut EmptySatisfies
        typealias U = EmptySatisfies
        return compiler::satisfies(T, U)
    })
    test("direct structs with const declaration always satisfy reference immutable types", () => {
        typealias T = &EmptySatisfies
        const U = EmptySatisfies {}
        return compiler::satisfies(T, U)
    })
    /**
    TODO make sure this works, currently const declarations return linked types which are considered
        always mutable
    test("direct structs with const declaration don't satisfy reference mutable types", () => {
        typealias T = &mut EmptySatisfies
        const U = EmptySatisfies {}
        return !compiler::satisfies(T, U)
    })
    **/
    test("direct structs with var declaration always satisfy reference immutable types", () => {
        typealias T = &EmptySatisfies
        var U = EmptySatisfies {}
        return compiler::satisfies(T, U)
    })
    test("direct structs with var declaration always satisfy reference mutable types", () => {
        typealias T = &mut EmptySatisfies
        var U = EmptySatisfies {}
        return compiler::satisfies(T, U)
    })
}