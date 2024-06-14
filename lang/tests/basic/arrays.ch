import "../test.ch"

func arr_index(arr : int[2], index : int) : int {
    return arr[index]
}

func test_arrays() {
    test("test arrays can be passed to functions", () => {
        var arr = {}int(2);
        arr[0] = 2;
        arr[1] = 4;
        return arr_index(arr, 0) == 2 && arr_index(arr, 1) == 4;
    })
}