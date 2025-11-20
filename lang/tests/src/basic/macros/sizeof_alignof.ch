
comptime func <T> comptime_size_of() : ubigint {
    return sizeof(T)
}

comptime func <T> comptime_align_of() : ubigint {
    return alignof(T)
}

struct SizeOfStrT1 {
    var p1 : int;
    var p2 : int
    var p3 : int
}

struct SOT1 {
    var a : char;
    var b : int;
    var c : char;
};

struct SOT2 {
    var a : double;
    var b : char;
};

struct SOT3 {
    var a : int;
    var b : [3]char;
    var c : short;
};

struct SOT4 {
    var x : char;
    var b : SOT3
    var z : int;
};

struct SOT5 {
    var a : short;
    var b : char;
    var c : double;
};

struct SOT6 {};

struct SOT7 {
    var a : *int;
    var b : char;
    var c : *double;
};

struct SOT8 {
    var a : short;
    var b : long;
    var c : char;
};

func test_sizeof_alignof() {
    test("sizeof int", () => {
        var i = sizeof(int)
        return i == 4;
    })
    test("sizeof long", () => {
        var i = sizeof(long)
        comptime if(def.is64Bit) {
            comptime if(def.windows) {
                return i == 4;
            } else {
                return i == 8;
            }
        } else {
            return i == 4;
        }
    })
    test("sizeof ulong works", () => {
        return sizeof(ulong) == comptime_size_of<ulong>()
    })
    test("sizeof struct", () => {
        var i = sizeof(SizeOfStrT1)
        return i == 12;
    })
    test("alignof struct", () => {
        var i = alignof(SizeOfStrT1)
        return i == 4;
    })
    test("sizeof and align of struct SOT1", () => {
        var s = sizeof(SOT1)
        var a = alignof(SOT1)
        var cs = comptime_size_of<SOT1>();
        var ca = comptime_align_of<SOT1>();
        if(s != cs || a != ca) {
            printf("difference in runtime and comptime SOT1 sizeof and alignof %d, %d and %d, %d\n", s, a, cs, ca)
        }
        return s == cs && a == ca
    })
    test("sizeof and align of struct SOT2", () => {
        var s = sizeof(SOT2)
        var a = alignof(SOT2)
        var cs = comptime_size_of<SOT2>();
        var ca = comptime_align_of<SOT2>();
        if(s != cs || a != ca) {
            printf("difference in runtime and comptime SOT2 sizeof and alignof %d, %d and %d, %d\n", s, a, cs, ca)
        }
        return s == cs && a == ca
    })
    test("sizeof and align of struct SOT3", () => {
        var s = sizeof(SOT3)
        var a = alignof(SOT3)
        var cs = comptime_size_of<SOT3>();
        var ca = comptime_align_of<SOT3>();
        if(s != cs || a != ca) {
            printf("difference in runtime and comptime SOT3 sizeof and alignof %d, %d and %d, %d\n", s, a, cs, ca)
        }
        return s == cs && a == ca
    })
    test("sizeof and align of struct SOT3", () => {
        var s = sizeof(SOT3)
        var a = alignof(SOT3)
        var cs = comptime_size_of<SOT3>();
        var ca = comptime_align_of<SOT3>();
        if(s != cs || a != ca) {
            printf("difference in runtime and comptime SOT3 sizeof and alignof %d, %d and %d, %d\n", s, a, cs, ca)
        }
        return s == cs && a == ca
    })
    test("sizeof and align of struct SOT4", () => {
        var s = sizeof(SOT4)
        var a = alignof(SOT4)
        var cs = comptime_size_of<SOT4>();
        var ca = comptime_align_of<SOT4>();
        if(s != cs || a != ca) {
            printf("difference in runtime and comptime SOT4 sizeof and alignof %d, %d and %d, %d\n", s, a, cs, ca)
        }
        return s == cs && a == ca
    })
    test("sizeof and align of struct SOT5", () => {
        var s = sizeof(SOT5)
        var a = alignof(SOT5)
        var cs = comptime_size_of<SOT5>();
        var ca = comptime_align_of<SOT5>();
        if(s != cs || a != ca) {
            printf("difference in runtime and comptime SOT5 sizeof and alignof %d, %d and %d, %d\n", s, a, cs, ca)
        }
        return s == cs && a == ca
    })
    test("sizeof and align of struct SOT6", () => {
        var s = sizeof(SOT6)
        var a = alignof(SOT6)
        var cs = comptime_size_of<SOT6>();
        var ca = comptime_align_of<SOT6>();
        if(s != cs || a != ca) {
            printf("difference in runtime and comptime SOT6 sizeof and alignof %d, %d and %d, %d\n", s, a, cs, ca)
        }
        return s == cs && a == ca
    })
    test("sizeof and align of struct SOT7", () => {
        var s = sizeof(SOT7)
        var a = alignof(SOT7)
        var cs = comptime_size_of<SOT7>();
        var ca = comptime_align_of<SOT7>();
        if(s != cs || a != ca) {
            printf("difference in runtime and comptime SOT7 sizeof and alignof %d, %d and %d, %d\n", s, a, cs, ca)
        }
        return s == cs && a == ca
    })
    test("sizeof and align of struct SOT8", () => {
        var s = sizeof(SOT8)
        var a = alignof(SOT8)
        var cs = comptime_size_of<SOT8>();
        var ca = comptime_align_of<SOT8>();
        if(s != cs || a != ca) {
            printf("difference in runtime and comptime SOT8 sizeof and alignof %d, %d and %d, %d\n", s, a, cs, ca)
        }
        return s == cs && a == ca
    })
    test("sizeof on references returns sizeof underlying type - 1", () => {
        return sizeof(&char) == sizeof(char) && sizeof(&uchar) == sizeof(uchar)
    })
    test("sizeof on references returns sizeof underlying type - 2", () => {
        return sizeof(&short) == sizeof(short) && sizeof(&ushort) == sizeof(ushort)
    })
    test("sizeof on references returns sizeof underlying type - 3", () => {
        return sizeof(&int) == sizeof(int) && sizeof(&uint) == sizeof(uint)
    })
    test("sizeof on references returns sizeof underlying type - 4", () => {
        return sizeof(&long) == sizeof(long) && sizeof(&ulong) == sizeof(ulong)
    })
    test("sizeof on references returns sizeof underlying type - 5", () => {
        return sizeof(&bigint) == sizeof(bigint) && sizeof(&ubigint) == sizeof(ubigint)
    })
    test("sizeof on references returns sizeof underlying type - 6", () => {
        return sizeof(&SizeOfStrT1) == sizeof(SizeOfStrT1)
    })
    test("alignof on references returns alignof underlying type - 1", () => {
        return alignof(&char) == alignof(char) && alignof(&uchar) == alignof(uchar)
    })
    test("alignof on references returns alignof underlying type - 2", () => {
        return alignof(&short) == alignof(short) && alignof(&ushort) == alignof(ushort)
    })
    test("alignof on references returns alignof underlying type - 3", () => {
        return alignof(&int) == alignof(int) && alignof(&uint) == alignof(uint)
    })
    test("alignof on references returns alignof underlying type - 4", () => {
        return alignof(&long) == alignof(long) && alignof(&ulong) == alignof(ulong)
    })
    test("alignof on references returns alignof underlying type - 5", () => {
        return alignof(&bigint) == alignof(bigint) && alignof(&ubigint) == alignof(ubigint)
    })
    test("alignof on references returns alignof underlying type - 6", () => {
        return alignof(&SizeOfStrT1) == alignof(SizeOfStrT1)
    })
    test("sizeof with array types work", () => {
        return sizeof([4]int) == 4 * 4;
    })
    test("sizeof with array of structs work", () => {
        return sizeof([4]SizeOfStrT1) == 4 * 4 * 3;
    })
}