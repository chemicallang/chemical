import common_tests

func test_core_ops() {
    test_arithmetic_bin_op_with_primitive()
    test_arithmetic_bin_op_with_structure()
    test_unary_arithmetic()
    test_primitive_single_assignment()
    test_structural_single_assignment()
}

func run_common_tests_wrapper() {
    run_common_tests()
}