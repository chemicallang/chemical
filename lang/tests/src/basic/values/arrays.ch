import "/test.ch"

struct DataStr1 {

    var data : int

}

func arr_index(arr : int[2], index : int) : int {
    return arr[index]
}

func test_arrays() {
    test("arrays can be passed to functions", () => {
        var arr = {}int(2);
        arr[0] = 2;
        arr[1] = 4;
        return arr_index(arr, 0) == 2 && arr_index(arr, 1) == 4;
    })
    test("can index on struct inside array directly", () => {
        var arr = {}DataStr1(1);
        arr[0].data = 5;
        return arr[0].data == 5;
    })
    test("can index on struct inside array indirectly", () => {
        var arr = {}DataStr1(1);
        var ptr = &arr[0];
        ptr.data = 5;
        return ptr.data == 5;
    })
}