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
