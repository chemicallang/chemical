struct DataStr1 {

    var data : int

}

func arr_index(arr : [2]int, index : int) : int {
    return arr[index]
}

func test_arrays() {
    test("arrays can be passed to functions", () => {
        var arr : [2]int = [];
        arr[0] = 2;
        arr[1] = 4;
        return arr_index(arr, 0) == 2 && arr_index(arr, 1) == 4;
    })
    test("can index on struct inside array directly", () => {
        var arr : [1]DataStr1 = [];
        arr[0].data = 5;
        return arr[0].data == 5;
    })
    test("can index on struct inside array indirectly", () => {
        var arr : [1]DataStr1 = [];
        var ptr = &mut arr[0];
        ptr.data = 5;
        return ptr.data == 5;
    })
}

struct EmptyArrStruct {
    var bytes : [16]u8;
}

@direct_init
struct PartialArrStruct {
    var bytes : [16]u8;
}

func test_empty_array_literal_zeroes_struct_field() : bool {
    var s = EmptyArrStruct { bytes : [] };
    var total : uint = 0;
    for (var i = 0u; i < 16u; i++) {
        total += s.bytes[i] as uint;
    }
    return total == 0u;
}

func test_partial_array_literal_zeroes_remaining() : bool {
    var s = PartialArrStruct { bytes : [1u8, 2u8, 3u8] };
    if(s.bytes[0] != 1u8 || s.bytes[1] != 2u8 || s.bytes[2] != 3u8) return false;
    var total : uint = 0;
    for (var i = 3u; i < 16u; i++) {
        total += s.bytes[i] as uint;
    }
    return total == 0u;
}

func test_array_literal_zero_fill() {
    test("empty array literal in struct field zero-fills all elements", () => {
        return test_empty_array_literal_zeroes_struct_field();
    })
    test("partial array literal in struct field zero-fills remaining elements", () => {
        return test_partial_array_literal_zeroes_remaining();
    })
}
