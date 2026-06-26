// Copyright (c) Chemical Language Foundation 2026.

// -------------------------------------------------------
// Common Tests
// These tests run in both compiled and interpretation mode.
// -------------------------------------------------------

// Self-contained comptime test functions

func very_basic_arithemetic() : bool {
    return 5 + 3 == 8
}

func traditional_for_loop_with_sum() : bool {
    var sum = 0
    for(var i = 0; i < 5; i++) {
        sum += i
    }
    return sum == 10
}

public func run_common_tests() {

    // Test 1: Basic comptime arithmetic
    test("very basic arithmetic works", () => {
        return very_basic_arithemetic();
    })

    // Test 2: Comptime loop works
    test("C loop with sum works", () => {
        return traditional_for_loop_with_sum();
    })

    // Basic language tests (no pointers — compatible with all backends)
    test_numbers();
    test_floating_expr();
    test_struct_values();
    test_in_value();
    test_for_loop();
    test_inc_dec();

    // Type system tests
    test_typealias();
    test_is_value();
    test_satisfies();
    test_casts();

    // Comptime tests
    test_comptime_expressions();
    test_comptime_features();
    test_annotations();
    test_switch_statement();
    test_bodmas();

    // First 10 (confirmed working)
    test("basic generic function with no generic args works", () => {
        return gen_sum(10, 20) == 30;
    })
    test("basic generic function with generic args works", () => {
        return gen_sum<long, long, long>(20, 20) == 40;
    })
    test("generic functions can be called inside other calls", () => {
        return is_this_60(gen_sum<long, long, long>(30, 30));
    })
    test("generic functions result can be saved into variables", () => {
        var i = gen_sum<long, long, long>(30, 40);
        return i == 70;
    })
    test("generic struct works - 1", () => {
        var p = PairGen <int, int, int> { a : 10, b : 12 }
        return p.add() == 22;
    })
    test("generic struct works - 2", () => {
        var p = PairGen <long, long, long> { a : 20, b : 15 }
        return p.add() == 35
    })
    test("generic struct can be passed as function arg", () => {
        return mul_int_pair(PairGen <int, int, int> {
            a : 2,
            b : 9
        }) == 18
    })
    test("generic structs can be returned - 1", () => {
        const p = create_pair_gen();
        return p.add() == 25;
    })
    test("generic structs can be returned - 2", () => {
        const p = create_pair_gen_long();
        return p.add() == 26;
    })
    test("extension functions work on generic nodes", () => {
        var p = PairGen<short, short, short> {
            a : 56,
            b : 7
        }
        return p.ext_div<short, short, short>() == 8;
    })
    // Generic tests (no pointers, no std deps, no native calls)
    test_basic_generics();
    test_generic_type_deduction();

    // Language feature tests moved from native (no variant matching, no raw pointers, no std deps)
    test_unicode();
    test_lambda();

    // More moved tests (no variant matching, no raw ptrs, no std deps)
    test_enum();

    // Newly moved tests (v3)
    test_constructors_with_init();
    // test_basic_interfaces() — skipped: interface definitions (even unused) crash interpreter
    test_var_init();
    // test_interface_generic_dispatch() — skipped: interface definitions crash interpreter

    test_comptime();
    test_compiler_vector();
    test_constructors();
    test_extension_functions();
    test_functions();
    test_implicit_functions();
    test_name_hiding();

    test_destructors();
    test_continue_destruction()
    test_break_destruction()
    test_self_ref_destruct()
    test_call_destruction()


    // References
    test_references()
    test_deref_and_refs()
    test_multi_field_ref();

    test_dereferences();
    test_if_switch_loop_value();
    test_arrays();
    test_zeroed_value();
    test_common_char_ptr_strings();
    test_namespaces();
    test_structs();
    test_unions();
    test_nodes();
    test_variants();
    test_moves()
    test_static_interfaces()
    test_basic_interfaces()
    test_dynamic_dispatch()
    test_primitive_implementations()
    test_external_functions()
    test_hidden_ptr_self_in_methods();
    test_ref_of_deref_hidden_ptr();
    test_interface_generic_dispatch()
    test_generic_static_interfaces()

    test_new();
    test_core_ops();

    test_for_in()
}
