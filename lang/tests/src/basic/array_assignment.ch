// Whole-array assignment. Arrays are not assignable in C, so the C backend must
// copy the bytes (`memcpy`) rather than emit `dest = src` (invalid C). A missing
// memcpy produced invalid generated C for single- and multi-dimensional arrays,
// and for an array-typed struct field.
//
// Compiled-only: the interpreter does not support whole-array assignment.
// C-backend only: the LLVM backend's array assignment is a separate, still-broken
// path (it does not copy the elements), so guard this test to the C backend.

func test_array_assignment() {
    comptime if(intrinsics::get_backend_name() != "LLVM") {

        test("single-dimensional array assignment copies the elements", () => {
            var a : [4]int = [1, 2, 3, 4]
            var b : [4]int = []
            b = a
            b[0] = 9
            return a[0] == 1 && b[0] == 9 && b[1] == 2 && b[2] == 3 && b[3] == 4
        })

        test("multi-dimensional array assignment copies the elements", () => {
            var a : [2][2]int = []
            a[0][0] = 1
            a[0][1] = 2
            a[1][0] = 3
            a[1][1] = 4
            var b : [2][2]int = []
            b = a
            b[0][0] = 9
            return a[0][0] == 1 && b[0][0] == 9 && b[1][0] == 3 && b[1][1] == 4
        })

        test("array assignment copies from a struct field", () => {
            var holder = ArrayFieldHolder { bytes : [1u8, 2u8, 3u8, 4u8] }
            var other : [4]u8 = []
            other = holder.bytes
            other[0] = 9u8
            return holder.bytes[0] == 1u8 && other[0] == 9u8 && other[3] == 4u8
        })
    }
}

struct ArrayFieldHolder {
    var bytes : [4]u8
}

var large_array_drop_count = 0

struct LargeArrayDrop {
    var value : int
    @delete
    func delete(&mut self) {
        unsafe { large_array_drop_count = large_array_drop_count + 1 }
    }
}

struct LargeArrayStruct {
    var a : LargeArrayDrop
    var b : LargeArrayDrop
}

func test_large_array_partial_init() {
    test("large arrays track per-element drop flags", () => {
        unsafe {
            large_array_drop_count = 0
            {
                var arr : [100]LargeArrayDrop
                arr[0] = LargeArrayDrop { value : 1 }
                arr[99] = LargeArrayDrop { value : 2 }
                if(arr[0].value != 1 || arr[99].value != 2) { return false }
            }
            return large_array_drop_count == 2
        }
    })
    test("large arrays of generated-glue structs track element leaves", () => {
        unsafe {
            large_array_drop_count = 0
            {
                var arr : [100]LargeArrayStruct
                arr[0] = LargeArrayStruct {
                    a : LargeArrayDrop { value : 1 },
                    b : LargeArrayDrop { value : 2 }
                }
                arr[99] = LargeArrayStruct {
                    a : LargeArrayDrop { value : 3 },
                    b : LargeArrayDrop { value : 4 }
                }
                if(arr[0].a.value != 1 || arr[99].b.value != 4) { return false }
            }
            return large_array_drop_count == 4
        }
    })
    test("large generated-glue arrays do not drop uninitialized element fields", () => {
        unsafe {
            large_array_drop_count = 0
            {
                var arr : [100]LargeArrayStruct
                arr[0].a = LargeArrayDrop { value : 5 }
                if(arr[0].a.value != 5) { return false }
            }
            // Only the initialized leaf may be dropped; the sibling must not be.
            return large_array_drop_count <= 1
        }
    })
}

struct LargeArrayNested {
    var inner : [2]LargeArrayDrop
}

struct LargeArrayNestedStruct {
    var inner : [2]LargeArrayStruct
}

func test_large_array_nested_templates() {
    test("large generated-glue arrays track nested array leaves", () => {
        unsafe {
            large_array_drop_count = 0
            {
                var arr : [100]LargeArrayNested
                arr[0] = LargeArrayNested {
                    inner : [LargeArrayDrop { value : 1 }, LargeArrayDrop { value : 2 }]
                }
                arr[99] = LargeArrayNested {
                    inner : [LargeArrayDrop { value : 3 }, LargeArrayDrop { value : 4 }]
                }
                if(arr[0].inner[1].value != 2 || arr[99].inner[0].value != 3) { return false }
            }
            return large_array_drop_count == 4
        }
    })
    test("large generated-glue arrays track nested struct-array leaves", () => {
        unsafe {
            large_array_drop_count = 0
            {
                var arr : [100]LargeArrayNestedStruct
                arr[0] = LargeArrayNestedStruct {
                    inner : [
                        LargeArrayStruct { a : LargeArrayDrop { value : 1 }, b : LargeArrayDrop { value : 2 } },
                        LargeArrayStruct { a : LargeArrayDrop { value : 3 }, b : LargeArrayDrop { value : 4 } }
                    ]
                }
                arr[99] = LargeArrayNestedStruct {
                    inner : [
                        LargeArrayStruct { a : LargeArrayDrop { value : 5 }, b : LargeArrayDrop { value : 6 } },
                        LargeArrayStruct { a : LargeArrayDrop { value : 7 }, b : LargeArrayDrop { value : 8 } }
                    ]
                }
            }
            return large_array_drop_count == 8
        }
    })
}


struct LargeArrayNestedLarge {
    var inner : [65]LargeArrayDrop
}

func test_large_array_nested_above_64() {
    test("large templates track inner arrays above 64 elements", () => {
        unsafe {
            large_array_drop_count = 0
            {
                var arr : [100]LargeArrayNestedLarge
                arr[0] = LargeArrayNestedLarge {
                    inner : [LargeArrayDrop { value : 0 }, LargeArrayDrop { value : 1 }, LargeArrayDrop { value : 2 }, LargeArrayDrop { value : 3 }, LargeArrayDrop { value : 4 }, LargeArrayDrop { value : 5 }, LargeArrayDrop { value : 6 }, LargeArrayDrop { value : 7 }, LargeArrayDrop { value : 8 }, LargeArrayDrop { value : 9 }, LargeArrayDrop { value : 10 }, LargeArrayDrop { value : 11 }, LargeArrayDrop { value : 12 }, LargeArrayDrop { value : 13 }, LargeArrayDrop { value : 14 }, LargeArrayDrop { value : 15 }, LargeArrayDrop { value : 16 }, LargeArrayDrop { value : 17 }, LargeArrayDrop { value : 18 }, LargeArrayDrop { value : 19 }, LargeArrayDrop { value : 20 }, LargeArrayDrop { value : 21 }, LargeArrayDrop { value : 22 }, LargeArrayDrop { value : 23 }, LargeArrayDrop { value : 24 }, LargeArrayDrop { value : 25 }, LargeArrayDrop { value : 26 }, LargeArrayDrop { value : 27 }, LargeArrayDrop { value : 28 }, LargeArrayDrop { value : 29 }, LargeArrayDrop { value : 30 }, LargeArrayDrop { value : 31 }, LargeArrayDrop { value : 32 }, LargeArrayDrop { value : 33 }, LargeArrayDrop { value : 34 }, LargeArrayDrop { value : 35 }, LargeArrayDrop { value : 36 }, LargeArrayDrop { value : 37 }, LargeArrayDrop { value : 38 }, LargeArrayDrop { value : 39 }, LargeArrayDrop { value : 40 }, LargeArrayDrop { value : 41 }, LargeArrayDrop { value : 42 }, LargeArrayDrop { value : 43 }, LargeArrayDrop { value : 44 }, LargeArrayDrop { value : 45 }, LargeArrayDrop { value : 46 }, LargeArrayDrop { value : 47 }, LargeArrayDrop { value : 48 }, LargeArrayDrop { value : 49 }, LargeArrayDrop { value : 50 }, LargeArrayDrop { value : 51 }, LargeArrayDrop { value : 52 }, LargeArrayDrop { value : 53 }, LargeArrayDrop { value : 54 }, LargeArrayDrop { value : 55 }, LargeArrayDrop { value : 56 }, LargeArrayDrop { value : 57 }, LargeArrayDrop { value : 58 }, LargeArrayDrop { value : 59 }, LargeArrayDrop { value : 60 }, LargeArrayDrop { value : 61 }, LargeArrayDrop { value : 62 }, LargeArrayDrop { value : 63 }, LargeArrayDrop { value : 64 }]
                }
                if(arr[0].inner[64].value != 64) { return false }
            }
            return large_array_drop_count == 65
        }
    })
    test("small outer arrays destroy whole large direct arrays", () => {
        unsafe {
            large_array_drop_count = 0
            {
                var arr : [2][65]LargeArrayDrop
                arr[0] = [LargeArrayDrop { value : 0 }, LargeArrayDrop { value : 1 }, LargeArrayDrop { value : 2 }, LargeArrayDrop { value : 3 }, LargeArrayDrop { value : 4 }, LargeArrayDrop { value : 5 }, LargeArrayDrop { value : 6 }, LargeArrayDrop { value : 7 }, LargeArrayDrop { value : 8 }, LargeArrayDrop { value : 9 }, LargeArrayDrop { value : 10 }, LargeArrayDrop { value : 11 }, LargeArrayDrop { value : 12 }, LargeArrayDrop { value : 13 }, LargeArrayDrop { value : 14 }, LargeArrayDrop { value : 15 }, LargeArrayDrop { value : 16 }, LargeArrayDrop { value : 17 }, LargeArrayDrop { value : 18 }, LargeArrayDrop { value : 19 }, LargeArrayDrop { value : 20 }, LargeArrayDrop { value : 21 }, LargeArrayDrop { value : 22 }, LargeArrayDrop { value : 23 }, LargeArrayDrop { value : 24 }, LargeArrayDrop { value : 25 }, LargeArrayDrop { value : 26 }, LargeArrayDrop { value : 27 }, LargeArrayDrop { value : 28 }, LargeArrayDrop { value : 29 }, LargeArrayDrop { value : 30 }, LargeArrayDrop { value : 31 }, LargeArrayDrop { value : 32 }, LargeArrayDrop { value : 33 }, LargeArrayDrop { value : 34 }, LargeArrayDrop { value : 35 }, LargeArrayDrop { value : 36 }, LargeArrayDrop { value : 37 }, LargeArrayDrop { value : 38 }, LargeArrayDrop { value : 39 }, LargeArrayDrop { value : 40 }, LargeArrayDrop { value : 41 }, LargeArrayDrop { value : 42 }, LargeArrayDrop { value : 43 }, LargeArrayDrop { value : 44 }, LargeArrayDrop { value : 45 }, LargeArrayDrop { value : 46 }, LargeArrayDrop { value : 47 }, LargeArrayDrop { value : 48 }, LargeArrayDrop { value : 49 }, LargeArrayDrop { value : 50 }, LargeArrayDrop { value : 51 }, LargeArrayDrop { value : 52 }, LargeArrayDrop { value : 53 }, LargeArrayDrop { value : 54 }, LargeArrayDrop { value : 55 }, LargeArrayDrop { value : 56 }, LargeArrayDrop { value : 57 }, LargeArrayDrop { value : 58 }, LargeArrayDrop { value : 59 }, LargeArrayDrop { value : 60 }, LargeArrayDrop { value : 61 }, LargeArrayDrop { value : 62 }, LargeArrayDrop { value : 63 }, LargeArrayDrop { value : 64 }]
                if(arr[0][64].value != 64) { return false }
            }
            return large_array_drop_count == 65
        }
    })
    test("small outer arrays destroy whole large nested arrays", () => {
        unsafe {
            large_array_drop_count = 0
            {
                var arr : [2]LargeArrayNestedLarge
                arr[0] = LargeArrayNestedLarge {
                    inner : [LargeArrayDrop { value : 0 }, LargeArrayDrop { value : 1 }, LargeArrayDrop { value : 2 }, LargeArrayDrop { value : 3 }, LargeArrayDrop { value : 4 }, LargeArrayDrop { value : 5 }, LargeArrayDrop { value : 6 }, LargeArrayDrop { value : 7 }, LargeArrayDrop { value : 8 }, LargeArrayDrop { value : 9 }, LargeArrayDrop { value : 10 }, LargeArrayDrop { value : 11 }, LargeArrayDrop { value : 12 }, LargeArrayDrop { value : 13 }, LargeArrayDrop { value : 14 }, LargeArrayDrop { value : 15 }, LargeArrayDrop { value : 16 }, LargeArrayDrop { value : 17 }, LargeArrayDrop { value : 18 }, LargeArrayDrop { value : 19 }, LargeArrayDrop { value : 20 }, LargeArrayDrop { value : 21 }, LargeArrayDrop { value : 22 }, LargeArrayDrop { value : 23 }, LargeArrayDrop { value : 24 }, LargeArrayDrop { value : 25 }, LargeArrayDrop { value : 26 }, LargeArrayDrop { value : 27 }, LargeArrayDrop { value : 28 }, LargeArrayDrop { value : 29 }, LargeArrayDrop { value : 30 }, LargeArrayDrop { value : 31 }, LargeArrayDrop { value : 32 }, LargeArrayDrop { value : 33 }, LargeArrayDrop { value : 34 }, LargeArrayDrop { value : 35 }, LargeArrayDrop { value : 36 }, LargeArrayDrop { value : 37 }, LargeArrayDrop { value : 38 }, LargeArrayDrop { value : 39 }, LargeArrayDrop { value : 40 }, LargeArrayDrop { value : 41 }, LargeArrayDrop { value : 42 }, LargeArrayDrop { value : 43 }, LargeArrayDrop { value : 44 }, LargeArrayDrop { value : 45 }, LargeArrayDrop { value : 46 }, LargeArrayDrop { value : 47 }, LargeArrayDrop { value : 48 }, LargeArrayDrop { value : 49 }, LargeArrayDrop { value : 50 }, LargeArrayDrop { value : 51 }, LargeArrayDrop { value : 52 }, LargeArrayDrop { value : 53 }, LargeArrayDrop { value : 54 }, LargeArrayDrop { value : 55 }, LargeArrayDrop { value : 56 }, LargeArrayDrop { value : 57 }, LargeArrayDrop { value : 58 }, LargeArrayDrop { value : 59 }, LargeArrayDrop { value : 60 }, LargeArrayDrop { value : 61 }, LargeArrayDrop { value : 62 }, LargeArrayDrop { value : 63 }, LargeArrayDrop { value : 64 }]
                }
                if(arr[0].inner[0].value != 0) { return false }
            }
            return large_array_drop_count == 65
        }
    })
}
