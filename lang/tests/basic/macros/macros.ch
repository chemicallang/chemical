import "../../test.ch"

@comptime
func <T> comptime_size_of() : ubigint {
    return #sizeof(T)
}

@comptime
func <T> comptime_align_of() : ubigint {
    return #alignof(T)
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
    var b : char[3];
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

func test_sizeof() {
    test("test sizeof int", () => {
        var i = #sizeof(int)
        return i == 4;
    })
    test("test sizeof long", () => {
        var i = #sizeof(long)
        if(def.is64Bit) {
            return i == 8;
        } else {
            return i == 4;
        }
    })
    test("test sizeof struct", () => {
        var i = #sizeof(SizeOfStrT1)
        return i == 12;
    })
    test("test alignof struct", () => {
        var i = #alignof(SizeOfStrT1)
        return i == 4;
    })
    test("test sizeof and align of struct SOT1", () => {
        var s = #sizeof(SOT1)
        var a = #alignof(SOT1)
        var cs = comptime_size_of<SOT1>();
        var ca = comptime_align_of<SOT1>();
        if(s != cs || a != ca) {
            printf("difference in runtime and comptime SOT1 sizeof and alignof %d, %d and %d, %d\n", s, a, cs, ca)
        }
        return s == cs && a == ca
    })
    test("test sizeof and align of struct SOT2", () => {
        var s = #sizeof(SOT2)
        var a = #alignof(SOT2)
        var cs = comptime_size_of<SOT2>();
        var ca = comptime_align_of<SOT2>();
        if(s != cs || a != ca) {
            printf("difference in runtime and comptime SOT2 sizeof and alignof %d, %d and %d, %d\n", s, a, cs, ca)
        }
        return s == cs && a == ca
    })
    test("test sizeof and align of struct SOT3", () => {
        var s = #sizeof(SOT3)
        var a = #alignof(SOT3)
        var cs = comptime_size_of<SOT3>();
        var ca = comptime_align_of<SOT3>();
        if(s != cs || a != ca) {
            printf("difference in runtime and comptime SOT3 sizeof and alignof %d, %d and %d, %d\n", s, a, cs, ca)
        }
        return s == cs && a == ca
    })
    test("test sizeof and align of struct SOT3", () => {
        var s = #sizeof(SOT3)
        var a = #alignof(SOT3)
        var cs = comptime_size_of<SOT3>();
        var ca = comptime_align_of<SOT3>();
        if(s != cs || a != ca) {
            printf("difference in runtime and comptime SOT3 sizeof and alignof %d, %d and %d, %d\n", s, a, cs, ca)
        }
        return s == cs && a == ca
    })
    test("test sizeof and align of struct SOT4", () => {
        var s = #sizeof(SOT4)
        var a = #alignof(SOT4)
        var cs = comptime_size_of<SOT4>();
        var ca = comptime_align_of<SOT4>();
        if(s != cs || a != ca) {
            printf("difference in runtime and comptime SOT4 sizeof and alignof %d, %d and %d, %d\n", s, a, cs, ca)
        }
        return s == cs && a == ca
    })
    test("test sizeof and align of struct SOT5", () => {
        var s = #sizeof(SOT5)
        var a = #alignof(SOT5)
        var cs = comptime_size_of<SOT5>();
        var ca = comptime_align_of<SOT5>();
        if(s != cs || a != ca) {
            printf("difference in runtime and comptime SOT5 sizeof and alignof %d, %d and %d, %d\n", s, a, cs, ca)
        }
        return s == cs && a == ca
    })
    test("test sizeof and align of struct SOT6", () => {
        var s = #sizeof(SOT6)
        var a = #alignof(SOT6)
        var cs = comptime_size_of<SOT6>();
        var ca = comptime_align_of<SOT6>();
        if(s != cs || a != ca) {
            printf("difference in runtime and comptime SOT6 sizeof and alignof %d, %d and %d, %d\n", s, a, cs, ca)
        }
        return s == cs && a == ca
    })
    test("test sizeof and align of struct SOT7", () => {
        var s = #sizeof(SOT7)
        var a = #alignof(SOT7)
        var cs = comptime_size_of<SOT7>();
        var ca = comptime_align_of<SOT7>();
        if(s != cs || a != ca) {
            printf("difference in runtime and comptime SOT7 sizeof and alignof %d, %d and %d, %d\n", s, a, cs, ca)
        }
        return s == cs && a == ca
    })
    test("test sizeof and align of struct SOT8", () => {
        var s = #sizeof(SOT8)
        var a = #alignof(SOT8)
        var cs = comptime_size_of<SOT8>();
        var ca = comptime_align_of<SOT8>();
        if(s != cs || a != ca) {
            printf("difference in runtime and comptime SOT8 sizeof and alignof %d, %d and %d, %d\n", s, a, cs, ca)
        }
        return s == cs && a == ca
    })
}

func test_macros() {
    test_sizeof();
    test("test evaluation macro works", () => {
        var evaluated = #eval { 2 + 2 };
        return evaluated == 4;
    });
}