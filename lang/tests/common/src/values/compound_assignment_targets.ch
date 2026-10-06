// Compound assignment (`+=`, `-=`, `*=`, `&=`, `<<=`, ...) whose target is not a
// plain local variable: array elements, pointer indices and dereferences. These
// targets must load the current value, apply the operator and store the result;
// a codegen bug previously stored the right-hand side directly (e.g. `p[0] += 4`
// produced `p[0] = 4`), silently corrupting buffers.

struct CompoundAssignStruct {
    var a : int
    var b : int
}

func compound_add_one_through_ptr(p : *mut int) {
    p[0] += 1
}

func test_compound_assignment_targets() {

    test("compound add on array element works", () => {
        var arr : [3]int
        arr[0] = 10
        arr[1] = 20
        arr[2] = 30
        arr[1] += 5
        return arr[0] == 10 && arr[1] == 25 && arr[2] == 30
    })

    test("compound sub on array element works", () => {
        var arr : [2]int
        arr[0] = 10
        arr[1] = 20
        arr[0] -= 4
        return arr[0] == 6 && arr[1] == 20
    })

    test("compound mul on array element works", () => {
        var arr : [2]int
        arr[0] = 3
        arr[1] = 4
        arr[1] *= 5
        return arr[0] == 3 && arr[1] == 20
    })

    test("compound bitand on array element works", () => {
        var arr : [2]int
        arr[0] = 6
        arr[1] = 14
        arr[0] &= 3
        return arr[0] == 2 && arr[1] == 14
    })

    test("compound shift on array element works", () => {
        var arr : [1]int
        arr[0] = 3
        arr[0] <<= 2
        return arr[0] == 12
    })

    test("compound add through pointer index works", () => {
        var buf : [2]int
        buf[0] = 7
        buf[1] = 8
        var p = &raw mut buf[0]
        p[1] += 10
        return buf[0] == 7 && buf[1] == 18
    })

    test("compound add through dereference works", () => {
        var x = 5
        var q = &mut x
        *q += 3
        return x == 8
    })

    test("compound assignment on struct fields works", () => {
        var s = CompoundAssignStruct { a : 1, b : 2 }
        s.a += 4
        s.b *= 3
        return s.a == 5 && s.b == 6
    })

    test("compound add through pointer parameter works", () => {
        var v = 41
        compound_add_one_through_ptr(&raw mut v)
        return v == 42
    })
}
