struct SV_OrderTesting1 {
    var a : int
    var b : int
}

var first_val_init_index = -1

func sv_order_checker_1st() : int {
    if(first_val_init_index == -1) {
        first_val_init_index = 0
    }
    return 0;
}

func sv_order_checker_2nd() : int {
    if(first_val_init_index == -1) {
        first_val_init_index = 1
    }
    return 0;
}

func struct_values_initialized_in_order_1() : bool {
    first_val_init_index = -1;
    var s = SV_OrderTesting1 {
        a : sv_order_checker_1st(),
        b : sv_order_checker_2nd()
    }
    return first_val_init_index == 0
}

func struct_values_initialized_in_order_2() : bool {
    first_val_init_index = -1;
    var s = SV_OrderTesting1 {
        a : sv_order_checker_2nd(),
        b : sv_order_checker_1st()
    }
    return first_val_init_index == 1
}

func test_struct_values() {
    // we run the same test 10 times
    test("struct values are initialized in order - 1", struct_values_initialized_in_order_1)
    test("struct values are initialized in order - 2", struct_values_initialized_in_order_2)
    test("struct values are initialized in order - 3", struct_values_initialized_in_order_2)
    test("struct values are initialized in order - 4", struct_values_initialized_in_order_1)
    test("struct values are initialized in order - 5", struct_values_initialized_in_order_2)
    test("struct values are initialized in order - 6", struct_values_initialized_in_order_1)
    test("struct values are initialized in order - 7", struct_values_initialized_in_order_1)
    test("struct values are initialized in order - 8", struct_values_initialized_in_order_2)
    test("struct values are initialized in order - 9", struct_values_initialized_in_order_1)
    test("struct values are initialized in order - 10", struct_values_initialized_in_order_2)
}