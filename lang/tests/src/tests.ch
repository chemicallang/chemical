func run_executable_tests() {
    test_var_init();
    test_lambda();
    test_bodmas();
    test_floating_expr();
    test_nodes();
    test_if_switch_loop_value();
    test_numbers();
    test_structs();
    test_enum();
    test_char_ptr_strings();
    test_strings();
    test_macros();
    test_arrays();
    test_pointer_math();
    test_casts();
    test_functions();
    test_implicit_functions();
    test_destructors();
    test_continue_destruction()
    test_break_destruction()
    test_unions();
    test_namespaces();
    test_comptime();
    test_comptime_expressions();
    test_compiler_vector();
    test_external_functions();
    test_basic_generics();
    test_generic_type_deduction();
    test_hashing();
    test_vectors();
    test_array_refs();
    test_dynamic_dispatch();
    test_variants();
    test_variant_pattern_matching();
    test_optional_type();
    test_result_type();
    test_is_value();
    test_for_loop()
    test_imported_modules();
    test_modules_import();
    test_moves();
    test_generic_moves();
    test_html();
    test_references();
    test_auto_deref();
    test_satisfies();
    test_new();
    test_extension_functions();
    test_typealias();
    test_inc_dec();
    test_static_interfaces();
    test_pointers_in_comptime();
    test_constructors();
    test_unordered_map();
    test_abstract_structs();
    test_generic_static_interfaces();
    test_external_interfaces();
    test_interface_generic_dispatch();
    test_capturing_lambda();
    test_in_value();
    test_failing_code();
    test_core_ops();
    test_atomics();
    test_thread_pool()
    test_constructors_with_init()
    test_primitive_implementations()
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