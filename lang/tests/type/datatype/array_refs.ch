import "../../../std/array_ref.ch"

func array_ref_size(array : ArrayRef<int>) : size_t {
    return array.size;
}

func array_ref_at(array : ArrayRef<int>, ind : int) : int {
    return array.data[ind]
}

func test_array_refs() {
    test("array reference works implicitly with array values", () => {
        return array_ref_size({ 10, 20 }) == 2;
    })
    test("array reference works implicitly with referenced arrays", () => {
        var arr = { 10 }
        return array_ref_size(arr) == 1;
    })
    test("array reference works implicitly with referenced arrays", () => {
        var arr = { 10 }
        return array_ref_at(arr, 0) == 10;
    })
    test("array pointer is passed properly with array values", () => {
        return array_ref_at({ 10, 20, 30 }, 2) == 30 && array_ref_at({ 10, 20, 30 }, 1) == 20;
    })
    test("array pointer is passed properly with referenced vectors", () => {
        var arr = vector<int>()
        arr.push(10)
        arr.push(20)
        arr.push(30)
        return array_ref_at(arr, 2) == 30 && array_ref_at(arr, 1) == 20;
    })
    test("array reference works implicitly with referenced vectors", () => {
        var arr = vector<int>()
        arr.push(10)
        return array_ref_size(arr) == 1;
    })
    test("array reference works implicitly with referenced vectors", () => {
        var arr = vector<int>()
        arr.push(10)
        return array_ref_at(arr, 0) == 10;
    })
    test("array pointer is passed properly with referenced vectors", () => {
        var arr = vector<int>()
        arr.push(10)
        arr.push(20)
        arr.push(30)
        return array_ref_at(arr, 2) == 30 && array_ref_at(arr, 1) == 20;
    })
}