// Regression: `sizeof(<variable>)` must use the variable's declared type, not
// decay to an opaque pointer.
//
// The MIR backend used to lower `sizeof(buffer)` where `buffer : [160]char` as
// `sizeof(void*)` (8) instead of 160. That silently truncated a formatted test
// id in the test runner (which used a `[160]char` buffer), so spawned test
// children matched no test and every suite reported "success" without running
// anything. These assertions run in the main compiled suite so the bug fails
// fast and loudly instead.

struct SizeofProbe {
    var a : int
    var b : double
}

func test_sizeof_variables() {
    test("sizeof array variable uses the whole array size", () => {
        var buffer : [160]char
        return sizeof(buffer) == 160
    })
    test("sizeof array variable scales with element size", () => {
        var ints : [8]int
        return sizeof(ints) == 8 * sizeof(int)
    })
    test("sizeof array variable is consistent with sizeof of its type", () => {
        var bytes : [32]char
        return sizeof(bytes) == sizeof([32]char)
    })
    test("sizeof struct variable matches sizeof of the struct type", () => {
        var probe : SizeofProbe
        return sizeof(probe) == sizeof(SizeofProbe)
    })
    test("sizeof scalar variables match their declared types", () => {
        var i : int
        var d : double
        var c : char
        return sizeof(i) == sizeof(int) && sizeof(d) == sizeof(double) && sizeof(c) == sizeof(char)
    })
    test("sizeof a pointer variable is pointer-sized", () => {
        var p : *void
        return sizeof(p) == sizeof(*void)
    })
}
