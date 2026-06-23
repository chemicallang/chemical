func run_executable_tests() {

    // ---------------------------------
    // Basic language that should work
    // Constructs that are mostly C like
    // ----------------------------------
    run_common_tests();
    run_native_common_tests();
    // test_var_init() moved to common (covered by run_common_tests())
    // Native-only: variant init tests (crash interpreter)
    test_native_var_init();
    // test_lambda() moved to common (covered by run_common_tests())
    test_structs();
    test_nodes();
    test_if_switch_loop_value();
    test_arrays();
    test_char_ptr_strings();
    test_strings();
    test_unions();
    test_namespaces();
    test_external_functions();
    // test_enum() moved to common (covered by run_common_tests())
    // Native-only: address of enum works in function (crashes interpreter)
    test_native_enum();
    // test_interface_generic_dispatch() called in extended syntax section below
    test_auto_deref();
    test_zeroed_value()


    // -------------------------------------------
    // Extended Syntax
    // this syntax is mostly present in languages like C++ and Rust
    // -------------------------------------------
    test_temp_view_lifetime()

    test_references();
    test_hidden_ptr_self_in_methods();
    test_ref_of_deref_hidden_ptr();
    // test_multi_field_ref() moved to common (covered by run_common_tests())
    // test_basic_generics() and test_generic_type_deduction() are now
    // covered by run_common_tests() (common module version without variant
    // pattern matching). Native-only specifics are called separately:
    test_native_generic_specifics();
    test_native_deduction_variants();
    test_dynamic_dispatch();
    test_variants();
    test_imported_modules();
    test_modules_import();
    test_moves();
    test_generic_moves();
    test_new();
    test_static_interfaces();
    test_interface_generic_dispatch();
    test_for_in()
    test_core_ops();
    test_primitive_implementations()
    test_basic_interfaces()
    // test_constructors_with_init() moved to common (covered by run_common_tests())
    test_generic_static_interfaces();
    test_external_interfaces();
    test_capturing_lambda();

    // --------------------------------------
    // Comptime and Compiler Intrinsics Stuff
    // --------------------------------------
    test_macros();
    test_failing_code();
    test_comptime_intrinsics()

    // --------------------------------------
    // Language Helpers
    // may or may not be part of the standard library
    // --------------------------------------
    test_atomics();

    // --------------------------------------
    // Standard Library and Complicated stuff
    // --------------------------------------
    test_hashing();
    test_vectors();
    test_array_refs();
    test_variant_pattern_matching();
    test_optional_type();
    test_result_type();
    test_unordered_map();
    test_deque();
    test_conversions();
    test_thread_pool()
    test_time_types();

}

public func main(argc : int, argv : **char) : int {
    if(argc <= 1) {
        run_executable_tests()
        // print a separator
        printf("\n");
    }
    // this will trigger tests with @test annotation
    test_runner(argc, argv)
    if(argc <= 1) {
        // this will print the test stats
        print_test_stats();
    }
    return 0;
}
