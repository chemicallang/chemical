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
