func run_executable_tests() {

    // ---------------------------------
    // Basic language that should work
    // Constructs that are mostly C like
    // ----------------------------------
    run_common_tests();
    run_native_common_tests();
    test_char_ptr_strings();
    test_downloaded_module()
    test_deref_pass_func_call()


    test_variant_native();
    test_static_external();
    // -------------------------------------------
    // Extended Syntax
    // this syntax is mostly present in languages like C++ and Rust
    // -------------------------------------------
    test_temp_view_lifetime()
    test_native_generic_specifics();
    test_imported_modules();
    test_modules_import();
    test_external_interfaces();
    test_capturing_lambda();

    // --------------------------------------
    // Comptime and Compiler Intrinsics Stuff
    // --------------------------------------
    test_macros();
    test_comptime_intrinsics()

    // --------------------------------------
    // Language Helpers
    // may or may not be part of the standard library
    // --------------------------------------
    test_atomics();

    // --------------------------------------
    // Standard Library and Complicated stuff
    // --------------------------------------
    test_for_in()
    test_strings();
    test_hashing();
    test_vectors();
    test_array_refs();
    test_optional_type();
    test_result_type();
    test_unordered_map();
    test_ordered_map();
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
