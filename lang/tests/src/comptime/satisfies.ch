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
    test("bool type satisfy", () => {
        type T = bool;
        return intrinsics::satisfies<T, T>();
    })
    test("uchar types satisfy", () => {
        type T = uchar;
        return intrinsics::satisfies<T, T>();
    })
    test("short types satisfy", () => {
        type T = short;
        return intrinsics::satisfies<T, T>();
    })
    test("ushort types satisfy", () => {
        type T = ushort;
        return intrinsics::satisfies<T, T>();
    })
    test("int types satisfy", () => {
        type T = int;
        return intrinsics::satisfies<T, T>();
    })
    test("uint types satisfy", () => {
        type T = uint;
        return intrinsics::satisfies<T, T>();
    })
    test("long types satisfy", () => {
        type T = long;
        return intrinsics::satisfies<T, T>();
    })
    test("ulong types satisfy", () => {
        type T = ulong;
        return intrinsics::satisfies<T, T>();
    })
    test("bigint types satisfy", () => {
        type T = bigint;
        return intrinsics::satisfies<T, T>();
    })
    test("ubigint types satisfy", () => {
        type T = ubigint;
        return intrinsics::satisfies<T, T>();
    })
    test("unsigned types (not) satisfy each other - 0", () => {
        type T = bool;
        type U = uint
        return !intrinsics::satisfies<T, U>() && !intrinsics::satisfies<U, T>()
    })
    test("unsigned types satisfy each other - 1", () => {
        type T = uchar;
        type U = uint
        return intrinsics::satisfies<T, U>() && intrinsics::satisfies<U, T>()
    })
    test("unsigned types satisfy each other - 2", () => {
        type T = ushort;
        type U = uint
        return intrinsics::satisfies<T, U>() && intrinsics::satisfies<U, T>()
    })
    test("unsigned types satisfy each other - 3", () => {
        type T = ubigint;
        type U = ulong
        return intrinsics::satisfies<T, U>() && intrinsics::satisfies<U, T>()
    })
    test("unsigned types satisfy each other - 4", () => {
        type T = ulong;
        type U = ushort
        return intrinsics::satisfies<T, U>() && intrinsics::satisfies<U, T>()
    })
    test("unsigned types satisfy each other - 5", () => {
        type T = ushort;
        type U = ubigint
        return intrinsics::satisfies<T, U>() && intrinsics::satisfies<U, T>()
    })
    test("signed types (not) satisfy each other - 0", () => {
        type T = bool;
        type U = int
        return !intrinsics::satisfies<T, U>() && !intrinsics::satisfies<U, T>()
    })
    test("signed types satisfy each other - 1", () => {
        type T = char;
        type U = int
        return intrinsics::satisfies<T, U>() && intrinsics::satisfies<U, T>()
    })
    test("signed types satisfy each other - 2", () => {
        type T = short;
        type U = int
        return intrinsics::satisfies<T, U>() && intrinsics::satisfies<U, T>()
    })
    test("signed types satisfy each other - 3", () => {
        type T = bigint;
        type U = long
        return intrinsics::satisfies<T, U>() && intrinsics::satisfies<U, T>()
    })
    test("signed types satisfy each other - 4", () => {
        type T = long;
        type U = short
        return intrinsics::satisfies<T, U>() && intrinsics::satisfies<U, T>()
    })
    test("signed types satisfy each other - 5", () => {
        type T = short;
        type U = bigint
        return intrinsics::satisfies<T, U>() && intrinsics::satisfies<U, T>()
    })
    test("signed types do not satisfy unsigned and vice versa - 1", () => {
        type T = int
        type U = uint
        return !intrinsics::satisfies<T, U>() && !intrinsics::satisfies<U, T>()
    })
    test("signed types do not satisfy unsigned and vice versa - 2", () => {
        type T = long
        type U = ulong
        return !intrinsics::satisfies<T, U>() && !intrinsics::satisfies<U, T>()
    })
    test("signed types do not satisfy unsigned and vice versa - 3", () => {
        type T = bigint
        type U = ubigint
        return !intrinsics::satisfies<T, U>() && !intrinsics::satisfies<U, T>()
    })
    test("signed types do not satisfy unsigned and vice versa - 4", () => {
        type T = short
        type U = ushort
        return !intrinsics::satisfies<T, U>() && !intrinsics::satisfies<U, T>()
    })
    test("signed types do not satisfy unsigned and vice versa - 5", () => {
        type T = char
        type U = uchar
        return !intrinsics::satisfies<T, U>() && !intrinsics::satisfies<U, T>()
    })
    test("signed types do not satisfy unsigned and vice versa - 6", () => {
        type T = long
        type U = uint
        return !intrinsics::satisfies<T, U>() && !intrinsics::satisfies<U, T>()
    })
    test("signed types do not satisfy unsigned and vice versa - 7", () => {
        type T = ubigint
        type U = int
        return !intrinsics::satisfies<T, U>() && !intrinsics::satisfies<U, T>()
    })
    test("signed types do not satisfy unsigned and vice versa - 7", () => {
        type T = uint
        type U = bigint
        return !intrinsics::satisfies<T, U>() && !intrinsics::satisfies<U, T>()
    })
    test("signed types do not satisfy unsigned and vice versa - 8", () => {
        type T = ushort
        type U = bigint
        return !intrinsics::satisfies<T, U>() && !intrinsics::satisfies<U, T>()
    })
    test("signed types do not satisfy unsigned and vice versa - 9", () => {
        type T = ulong
        type U = short
        return !intrinsics::satisfies<T, U>() && !intrinsics::satisfies<U, T>()
    })
    test("signed types do not satisfy unsigned and vice versa - 9", () => {
        type T = long
        type U = ushort
        return !intrinsics::satisfies<T, U>() && !intrinsics::satisfies<U, T>()
    })
    test("signed types do not satisfy unsigned and vice versa - 10", () => {
        type T = ubigint
        type U = long
        return !intrinsics::satisfies<T, U>() && !intrinsics::satisfies<U, T>()
    })
    test("struct types satisfy", () => {
        return intrinsics::satisfies<BaseSatisfies11, BaseSatisfies11>()
    })
    test("derived struct types satisfy base but not the other way around", () => {
        return intrinsics::satisfies<BaseSatisfies11, DerivedSatisfies11>() && !intrinsics::satisfies<DerivedSatisfies11, BaseSatisfies11>()
    })
    test("intN pointer types satisfy", () => {
        type T = *int
        return intrinsics::satisfies<T, T>()
    })
    test("struct pointer types satisfy", () => {
        type T = *BaseSatisfies11
        return intrinsics::satisfies<T, T>()
    })
    test("derived struct pointer types satisfy base struct pointer types but not the other way around", () => {
        type T = *DerivedSatisfies11
        type U = *BaseSatisfies11
        return intrinsics::satisfies<U, T>() && !intrinsics::satisfies<T, U>()
    })
    test("immutable pointer types do not satisfy mutable pointer types", () => {
        type T = *mut int
        type U = *int
        return !intrinsics::satisfies<T, U>()
    })
    test("mutable pointer types satisfy immutable pointer types", () => {
        type T = *mut int
        type U = *int
        return intrinsics::satisfies<U, T>()
    })
    test("immutable pointer struct types do not satisfy mutable pointer struct types", () => {
        type T = *mut BaseSatisfies11
        type U = *BaseSatisfies11
        return !intrinsics::satisfies<T, U>()
    })
    test("mutable pointer struct types satisfy immutable pointer struct types", () => {
        type T = *mut BaseSatisfies11
        type U = *BaseSatisfies11
        return intrinsics::satisfies<U, T>()
    })
    test("intN pointer types NOT satisfy other intN pointer types - 1", () => {
        type T = *int
        type U = *long
        return !intrinsics::satisfies<T, U>()
    })
    test("intN pointer types NOT satisfy other intN pointer types - 2", () => {
        type T = *int
        type U = *long
        return !intrinsics::satisfies<T, U>()
    })
    test("intN pointer types NOT satisfy other intN pointer types - 3", () => {
        type T = *uint
        type U = *ulong
        return !intrinsics::satisfies<T, U>()
    })
    test("intN pointer types NOT satisfy other intN pointer types - 4", () => {
        type T = *uint
        type U = *ulong
        return !intrinsics::satisfies<T, U>()
    })
    test("intN pointer types satisfy other intN pointer types - 5", () => {
        type T = *int
        type U = *ulong
        return !intrinsics::satisfies<T, U>()
    })
    test("intN pointer types satisfy other intN pointer types - 6", () => {
        type T = *uint
        type U = *long
        return !intrinsics::satisfies<T, U>()
    })
    test("intN reference types satisfy", () => {
        type T = &int
        return intrinsics::satisfies<T, T>()
    })
    test("struct reference types satisfy", () => {
        type T = &BaseSatisfies11
        return intrinsics::satisfies<T, T>()
    })
    test("derived struct reference types satisfy base struct reference types but not the other way around", () => {
        type T = &DerivedSatisfies11
        type U = &BaseSatisfies11
        return intrinsics::satisfies<U, T>() && !intrinsics::satisfies<T, U>()
    })
    test("immutable reference types do not satisfy mutable reference types", () => {
        type T = &mut int
        type U = &int
        return !intrinsics::satisfies<T, U>()
    })
    test("mutable reference types satisfy immutable reference types", () => {
        type T = &mut int
        type U = &int
        return intrinsics::satisfies<U, T>()
    })
    test("immutable reference struct types do not satisfy mutable reference struct types", () => {
        type T = &mut BaseSatisfies11
        type U = &BaseSatisfies11
        return !intrinsics::satisfies<T, U>()
    })
    test("mutable reference struct types satisfy immutable reference struct types", () => {
        type T = &mut BaseSatisfies11
        type U = &BaseSatisfies11
        return intrinsics::satisfies<U, T>()
    })
    test("intN reference types satisfy other intN reference types - 1", () => {
        type T = &int
        type U = &long
        return intrinsics::satisfies<T, U>()
    })
    test("intN reference types satisfy other intN reference types - 2", () => {
        type T = &int
        type U = &long
        return intrinsics::satisfies<T, U>()
    })
    test("intN reference types satisfy other intN reference types - 3", () => {
        type T = &uint
        type U = &ulong
        return intrinsics::satisfies<T, U>()
    })
    test("intN reference types satisfy other intN reference types - 4", () => {
        type T = &uint
        type U = &ulong
        return intrinsics::satisfies<T, U>()
    })
    test("intN reference types do not satisfy other intN reference types - 5", () => {
        type T = &int
        type U = &ulong
        return !intrinsics::satisfies<T, U>()
    })
    test("intN reference types do not satisfy other intN reference types - 6", () => {
        type T = &uint
        type U = &long
        return !intrinsics::satisfies<T, U>()
    })
    test("direct bool type does not satisfy mutable reference bool type without backing storage", () => {
        type T = &mut bool
        type U = bool
        return !intrinsics::satisfies<T, U>()
    })
    test("direct int n types do not satisfy mutable reference int n types without backing storage - 1", () => {
        type T = &mut int
        type U = int
        return !intrinsics::satisfies<T, U>()
    })
    test("direct int n types do not satisfy mutable reference int n types without backing storage - 2", () => {
        type T = &mut int
        type U = long
        return !intrinsics::satisfies<T, U>()
    })
    test("direct int n types do not satisfy mutable reference int n types without backing storage - 3", () => {
        type T = &mut uint
        type U = uint
        return !intrinsics::satisfies<T, U>()
    })
    test("direct int n types do not satisfy mutable reference int n types without backing storage - 4", () => {
        type T = &mut uint
        type U = ulong
        return !intrinsics::satisfies<T, U>()
    })
    test("direct bool types satisfies reference bool type without backing storage", () => {
        type T = &bool
        type U = bool
        return intrinsics::satisfies<T, U>()
    })
    test("direct int n types satisfies reference int n types without backing storage - 1", () => {
        type T = &int
        type U = int
        return intrinsics::satisfies<T, U>()
    })
    test("direct int n types satisfies reference int n types without backing storage - 2", () => {
        type T = &int
        type U = long
        return intrinsics::satisfies<T, U>()
    })
    test("direct int n types satisfies reference int n types without backing storage - 3", () => {
        type T = &uint
        type U = uint
        return intrinsics::satisfies<T, U>()
    })
    test("direct int n types satisfies reference int n types without backing storage - 4", () => {
        type T = &uint
        type U = ulong
        return intrinsics::satisfies<T, U>()
    })
    test("direct bool type satisfy reference bool type with backing storage", () => {
        type T = &bool
        var U : bool = false
        return intrinsics::satisfies<T, U>()
    })
    test("direct int n types satisfy reference int n types with backing storage - 1", () => {
        type T = &int
        var U : int = 0
        return intrinsics::satisfies<T, U>()
    })
    test("direct int n types satisfy reference int n types with backing storage - 2", () => {
        type T = &int
        var U : long = 0
        return intrinsics::satisfies<T, U>()
    })
    test("direct int n types satisfy reference int n types with backing storage - 3", () => {
        type T = &uint
        var U : uint = 0
        return intrinsics::satisfies<T, U>()
    })
    test("direct int n types satisfy reference int n types with backing storage - 4", () => {
        type T = &uint
        var U : ulong = 0
        return intrinsics::satisfies<T, U>()
    })
    test("direct bool type satisfy mutable reference bool type with backing storage", () => {
        var x = false
        var T : &mut bool = x
        var U : bool = false
        return intrinsics::value_satisfies(T, U)
    })
    test("direct int n types satisfy mutable reference int n types with backing storage - 1", () => {
        var x = 0
        var T : &mut int = x
        var U : int = 0
        return intrinsics::value_satisfies(T, U)
    })
    test("direct int n types satisfy mutable reference int n types with backing storage - 2", () => {
        var x = 0
        var T : &mut int = x
        var U : long = 0
        return intrinsics::value_satisfies(T, U)
    })
    test("direct int n types satisfy mutable reference int n types with backing storage - 3", () => {
        var x : uint = 0
        var T : &mut uint = x
        var U : uint = 0
        return intrinsics::value_satisfies(T, U)
    })
    test("direct int n types satisfy mutable reference int n types with backing storage - 4", () => {
        var x : uint = 0
        var T : &mut uint = x
        var U : ulong = 0
        return intrinsics::value_satisfies(T, U)
    })
    test("constant bool declaration does not qualify as backing storage for mutable reference types", () => {
        type T = &mut bool
        const U : bool = false
        return !intrinsics::satisfies<T, U>()
    })
    test("constant int n declarations do not qualify as backing storage for mutable reference types - 1", () => {
        type T = &mut int
        const U : int = 0
        return !intrinsics::satisfies<T, U>()
    })
    test("constant int n declarations do not qualify as backing storage for mutable reference types - 2", () => {
        type T = &mut int
        const U : long = 0
        return !intrinsics::satisfies<T, U>()
    })
    test("constant int n declarations do not qualify as backing storage for mutable reference types - 3", () => {
        type T = &mut uint
        const U : uint = 0
        return !intrinsics::satisfies<T, U>()
    })
    test("constant int n declarations do not qualify as backing storage for mutable reference types - 4", () => {
        type T = &mut uint
        const U : ulong = 0
        return !intrinsics::satisfies<T, U>()
    })
    test("constant bool declaration satisfies const reference types", () => {
        type T = &bool
        const U : bool = false
        return intrinsics::satisfies<T, U>()
    })
    test("constant int n declarations satisfy const reference types - 1", () => {
        type T = &int
        const U : int = 0
        return intrinsics::satisfies<T, U>()
    })
    test("constant int n declarations satisfy const reference types - 2", () => {
        type T = &int
        const U : long = 0
        return intrinsics::satisfies<T, U>()
    })
    test("constant int n declarations satisfy const reference types - 3", () => {
        type T = &uint
        const U : uint = 0
        return intrinsics::satisfies<T, U>()
    })
    test("constant int n declarations satisfy const reference types - 4", () => {
        type T = &uint
        const U : ulong = 0
        return intrinsics::satisfies<T, U>()
    })
    test("bool reference type satisfies bool type for auto dereference", () => {
        type T = bool
        type U = &bool
        type V = bool
        type X = &mut bool
        return intrinsics::satisfies<T, U>() && intrinsics::satisfies<V, X>()
    })
    test("reference int n types satisfies int n types for auto dereference - 1", () => {
        var x = 0
        var T : int = 0
        var U : &int = x
        return intrinsics::value_satisfies(T, U)
    })
    test("reference int n types satisfies int n types for auto dereference - 2", () => {
        var x = 0
        var T : int = 0
        var U : &long = x
        return intrinsics::value_satisfies(T, U)
    })
    test("reference int n types satisfies int n types for auto dereference - 3", () => {
        var x : uint = 0
        var T : uint = 0
        var U : &uint = x
        return intrinsics::value_satisfies(T, U)
    })
    test("reference int n types satisfies int n types for auto dereference - 4", () => {
        var x : ulong = 0
        var T : uint = 0
        var U : &ulong = x
        return intrinsics::value_satisfies(T, U)
    })
    test("reference int n types satisfies int n types for auto dereference - 5", () => {
        var x = 0
        var V : int = 0
        var X : &mut int = x
        return intrinsics::value_satisfies(V, X)
    })
    test("reference int n types satisfies int n types for auto dereference - 6", () => {
        var x = 0
        var V : int = 0
        var X : &mut long = x
        return intrinsics::value_satisfies(V, X)
    })
    test("reference int n types satisfies int n types for auto dereference - 7", () => {
        var x : uint = 0
        var V : uint = 0
        var X : &mut uint = x
        return intrinsics::value_satisfies(V, X)
    })
    test("reference int n types satisfies int n types for auto dereference - 8", () => {
        var x : ulong = 0
        var V : uint = 0
        var X : &mut ulong = x
        return intrinsics::value_satisfies(V, X)
    })
    test("function type satisfies *void type", () => {
        type T = *void
        type U = () => void
        return intrinsics::satisfies<T, U>()
    })
    test("direct structs always satisfy reference immutable types", () => {
        type T = &EmptySatisfies
        type U = EmptySatisfies
        return intrinsics::satisfies<T, U>()
    })
    test("direct structs always satisfy reference mutable types", () => {
        type T = &mut EmptySatisfies
        type U = EmptySatisfies
        return intrinsics::satisfies<T, U>()
    })
    test("direct structs with const declaration always satisfy reference immutable types", () => {
        type T = &EmptySatisfies
        const U = EmptySatisfies {}
        return intrinsics::satisfies<T, U>()
    })
    /**
    TODO make sure this works, currently const declarations return linked types which are considered
        always mutable
    test("direct structs with const declaration don't satisfy reference mutable types", () => {
        type T = &mut EmptySatisfies
        const U = EmptySatisfies {}
        return !intrinsics::satisfies<T, U>()
    })
    **/
    test("direct structs with var declaration always satisfy reference immutable types", () => {
        type T = &EmptySatisfies
        var U = EmptySatisfies {}
        return intrinsics::satisfies<T, U>()
    })
    test("direct structs with var declaration always satisfy reference mutable types", () => {
        type T = &mut EmptySatisfies
        var U = EmptySatisfies {}
        return intrinsics::satisfies<T, U>()
    })
}