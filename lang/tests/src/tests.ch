func run_executable_tests() {

    // ---------------------------------
    // Basic language that should work
    // Constructs that are mostly C like
    // ----------------------------------
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
    test_arrays();
    test_pointer_math();
    test_casts();
    test_functions();
    test_name_hiding()
    test_implicit_functions();
    test_unions();
    test_namespaces();
    test_external_functions();
    test_is_value();
    test_for_loop()
    test_dereferences();
    test_auto_deref();
    test_inc_dec();
    test_zeroed_value()
    test_typealias();

    // -------------------------------------------
    // Extended Syntax
    // this syntax is mostly present in languages like C++ and Rust
    // -------------------------------------------
    test_destructors();
    test_continue_destruction()
    test_break_destruction()
    test_temp_view_lifetime()
    test_self_ref_destruct()
    test_call_destruction()
    test_references();
    test_hidden_ptr_self_in_methods();
    test_ref_of_deref_hidden_ptr();
    test_multi_field_ref();
    test_basic_generics();
    test_generic_type_deduction();
    test_dynamic_dispatch();
    test_variants();
    test_imported_modules();
    test_modules_import();
    test_moves();
    test_generic_moves();
    test_new();
    test_extension_functions();
    test_static_interfaces();
    test_interface_generic_dispatch();
    test_for_in()
    test_struct_values()
    test_in_value();
    test_core_ops();
    test_primitive_implementations()
    test_basic_interfaces()
    test_constructors_with_init()
    test_generic_static_interfaces();
    test_external_interfaces();
    test_capturing_lambda();

    // --------------------------------------
    // Comptime and Compiler Intrinsics Stuff
    // --------------------------------------
    test_satisfies();
    test_macros();
    test_pointers_in_comptime();
    test_comptime();
    test_comptime_expressions();
    test_compiler_vector();

    // --------------------------------------
    // Others
    // --------------------------------------
    test_failing_code();

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
    test_constructors();
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
