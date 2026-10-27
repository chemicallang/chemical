// `&raw mut a[i]` on a multi-dimensional array must yield a pointer to the row
// (a pointer-to-array `T (*)[N]`), not a pointer-to-pointer. A legacy-compat
// type decay (`*[N]T` -> `T**`) made stride/indexing through the pointer read a
// pointer value out of the array data, crashing at runtime.
//
// Compiled-only: the interpreter does not model this pointer arithmetic.

func test_pointer_to_array() { unsafe {

    test("address of a multi-dimensional array row has the right stride", () => {
        var a : [2][2]int = []
        a[0][0] = 1
        a[0][1] = 2
        a[1][0] = 3
        a[1][1] = 4
        var p = &raw mut a[0]
        var q = p + 1
        return q[0][0] == 3 && q[0][1] == 4
    })

    test("address of a multi-dimensional array element strides by element", () => {
        var a : [2][2]int = []
        a[0][0] = 1
        a[0][1] = 2
        a[1][0] = 3
        a[1][1] = 4
        var e = &raw mut a[0][0]
        var f = e + 2
        return *f == 3
    })
 }}
