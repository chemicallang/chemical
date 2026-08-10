// ---------------------------------------------------------------------------
// offsetof tests
// Runs in compiled (tcc / llvm) and interpretation mode.
// ---------------------------------------------------------------------------

// structs with ABI-stable layouts (all common x86/x64 ABIs agree on these)

struct OFT1 {
    var a : char;
    var b : int;
    var c : char;
}; // a=0, b=4, c=8, sizeof=12

struct OFT2 {
    var a : double;
    var b : char;
}; // a=0, b depends on the ABI (8 on x64, 4 on some 32-bit ABIs)

struct OFT3 {
    var a : int;
    var b : [3]char;
    var c : short;
}; // a=0, b=4, c=8, sizeof=12

struct OFT4 {
    var x : char;
    var b : OFT3
    var z : int;
}; // x=0, b=4, z=16, sizeof=20

struct OFT7 {
    var a : *int;
    var b : char;
    var c : *double;
}; // a=0, b=ptr_size, c=2*ptr_size

struct OFT5 {
    var a : char;
    var b : char;
}; // a=0, b=1, sizeof=2 (no trailing padding)

// vtable-like struct (mirrors the webview Webview2ComHandler use case)
struct OFTVtbl {
    var first : *void;
    var second : *void;
    var third : *void;
}; // 0, 8, 16 on 64-bit

// comptime helpers (interpretation path)
comptime func comptime_off_oft1_b() : ubigint {
    return offsetof(OFT1, b)
}

comptime func comptime_off_oft4_z() : ubigint {
    return offsetof(OFT4, z)
}

comptime func comptime_off_oftv_third() : ubigint {
    return offsetof(OFTVtbl, third)
}

func test_offsetof() {
    test("offsetof first member is always 0", () => {
        return offsetof(OFT1, a) == 0 && offsetof(OFT2, a) == 0 &&
            offsetof(OFT3, a) == 0 && offsetof(OFT4, x) == 0 &&
            offsetof(OFT7, a) == 0 && offsetof(OFTVtbl, first) == 0
    })
    test("offsetof char then int is 4", () => {
        return offsetof(OFT1, b) == 4
    })
    test("offsetof char after int is 8", () => {
        return offsetof(OFT1, c) == 8
    })
    test("offsetof with array member", () => {
        return offsetof(OFT3, b) == 4
    })
    test("offsetof short after array is 8", () => {
        return offsetof(OFT3, c) == 8
    })
    test("offsetof nested struct member", () => {
        return offsetof(OFT4, b) == 4
    })
    test("offsetof member after nested struct", () => {
        return offsetof(OFT4, z) == 16
    })
    test("offsetof with pointer members", () => {
        if(def.is64Bit) {
            return offsetof(OFT7, b) == 8 && offsetof(OFT7, c) == 16
        } else {
            return offsetof(OFT7, b) == 4 && offsetof(OFT7, c) == 8
        }
    })
    test("offsetof vtable-like struct", () => {
        if(def.is64Bit) {
            return offsetof(OFTVtbl, second) == 8 && offsetof(OFTVtbl, third) == 16
        } else {
            return offsetof(OFTVtbl, second) == 4 && offsetof(OFTVtbl, third) == 8
        }
    })
    test("offsetof is less than or equal to the member area in sizeof", () => {
        return offsetof(OFT1, c) + 1 <= sizeof(OFT1)
    })
    test("offsetof member extends exactly to the struct size", () => {
        return offsetof(OFT5, b) + sizeof(char) == sizeof(OFT5)
    })
    test("runtime offsetof matches comptime offsetof", () => {
        var r = offsetof(OFT1, b)
        var c = comptime_off_oft1_b()
        if(r != c) {
            printf("runtime and comptime OFT1.b differ: %d and %d\n", r, c)
        }
        return r == c
    })
    test("runtime offsetof matches comptime offsetof (nested)", () => {
        var r = offsetof(OFT4, z)
        var c = comptime_off_oft4_z()
        if(r != c) {
            printf("runtime and comptime OFT4.z differ: %d and %d\n", r, c)
        }
        return r == c
    })
    test("runtime offsetof matches comptime offsetof (vtable-like)", () => {
        var r = offsetof(OFTVtbl, third)
        var c = comptime_off_oftv_third()
        if(r != c) {
            printf("runtime and comptime OFTVtbl.third differ: %d and %d\n", r, c)
        }
        return r == c
    })
    test("offsetof of the first vtable pointer is 0 (webview handler pattern)", () => {
        return offsetof(OFTVtbl, first) == 0
    })
}
