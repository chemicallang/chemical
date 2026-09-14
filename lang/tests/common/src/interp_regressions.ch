// Copyright (c) Chemical Language Foundation 2026.
//
// Regression tests for suspected interpreter bugs.
//
// These tests assert behaviour that is correct in the compiled (C/LLVM) backends
// but is currently broken (or was broken) in the AST interpreter. They run in both
// `./scripts/test.sh --tcc` (compiled) and `./scripts/test.sh --tcc --interpret`.
//
// The "evaluated exactly once" tests guard against the interpreter re-evaluating
// sub-expressions (function calls with side effects) when a value is used as a
// switch subject, an assignment RHS, etc. Double evaluation is observable through
// a module-level side-effect counter and also causes incorrect programs.
//
// The `zeroed` array tests guard against `ZeroedValue::evaluated_value` not
// materialising an `ArrayValue` for array types.

var interp_side_effect_count = 0

var interp_delete_count = 0

func interp_bump() : int {
    interp_side_effect_count = interp_side_effect_count + 1
    return 100
}

func interp_ret_bump() : int {
    return interp_bump()
}

func interp_id(v : int) : int {
    return v
}

var interp_index_side_effect_count = 0

func interp_bump_index() : int {
    interp_index_side_effect_count = interp_index_side_effect_count + 1
    return 0
}

struct InterpBox {
    var value : int

    @make
    func make(v : int) : InterpBox {
        return InterpBox { value : v }
    }

    @delete
    func delete(&mut self) {
        interp_delete_count = interp_delete_count + 1
    }
}

struct InterpPair {
    var a : int
    var b : int
}

variant InterpVariant {
    Num(n : int)
    Str(s : *char)
}

// --- single evaluation of the switch subject -----------------------------------

func interp_test_switch_subject_evaluated_once() {
    test("interpreter: switch subject is evaluated exactly once", () => {
        interp_side_effect_count = 0
        switch(interp_bump()) {
            100 => {}
            default => {}
        }
        return interp_side_effect_count == 1
    })
}

// --- single evaluation of assignment right-hand sides --------------------------

func interp_test_assign_rhs_evaluated_once() {
    test("interpreter: assignment RHS is evaluated exactly once", () => {
        interp_side_effect_count = 0
        var x = 0
        x = interp_bump()
        return interp_side_effect_count == 1 && x == 100
    })
}

func interp_test_compound_assign_rhs_evaluated_once() {
    test("interpreter: compound assignment RHS is evaluated exactly once", () => {
        interp_side_effect_count = 0
        var x = 0
        x += interp_bump()
        return interp_side_effect_count == 1 && x == 100
    })
}

func interp_test_chain_assign_rhs_evaluated_once() {
    test("interpreter: access chain assignment RHS is evaluated exactly once", () => {
        interp_side_effect_count = 0
        var p = InterpPair { a : 0, b : 0 }
        p.a = interp_bump()
        return interp_side_effect_count == 1 && p.a == 100
    })
}

func interp_test_index_assign_rhs_evaluated_once() {
    test("interpreter: index assignment RHS is evaluated exactly once", () => {
        interp_side_effect_count = 0
        var a = [0, 0]
        a[0] = interp_bump()
        return interp_side_effect_count == 1 && a[0] == 100
    })
}

// --- single evaluation of other sub-expressions --------------------------------

func interp_test_if_condition_evaluated_once() {
    test("interpreter: if condition is evaluated exactly once", () => {
        interp_side_effect_count = 0
        if(interp_bump() == 100) {
            var y = 1
        } else {
            var y = 2
        }
        return interp_side_effect_count == 1
    })
}

func interp_test_var_init_evaluated_once() {
    test("interpreter: var initializer is evaluated exactly once", () => {
        interp_side_effect_count = 0
        var r = interp_bump()
        return interp_side_effect_count == 1 && r == 100
    })
}

func interp_test_return_expr_evaluated_once() {
    test("interpreter: return expression is evaluated exactly once", () => {
        interp_side_effect_count = 0
        var r = interp_ret_bump()
        return interp_side_effect_count == 1 && r == 100
    })
}

func interp_test_function_arg_evaluated_once() {
    test("interpreter: function argument is evaluated exactly once", () => {
        interp_side_effect_count = 0
        var r = interp_id(interp_bump())
        return interp_side_effect_count == 1 && r == 100
    })
}

func interp_test_index_expr_evaluated_once() {
    test("interpreter: index expression is evaluated exactly once", () => {
        interp_index_side_effect_count = 0
        var a = [5, 6]
        var r = a[interp_bump_index()]
        return interp_index_side_effect_count == 1 && r == 5
    })
}

func interp_bool_side_effect() : bool {
    interp_side_effect_count = interp_side_effect_count + 1
    return true
}

func interp_test_logical_short_circuit() {
    test("interpreter: && and || short circuit side effects", () => {
        interp_side_effect_count = 0
        var a = false && interp_bool_side_effect()
        var b = true || interp_bool_side_effect()
        return interp_side_effect_count == 0 && a == false && b == true
    })
}

// --- zeroed arrays --------------------------------------------------------------

func interp_test_zeroed_array_of_int() {
    test("interpreter: zeroed array of primitives can be indexed and assigned", () => {
        var a : [3]int = zeroed<[3]int>()
        a[0] = 10
        a[1] = 20
        a[2] = 30
        return a[0] == 10 && a[1] == 20 && a[2] == 30
    })
}

func interp_test_zeroed_array_of_struct() {
    test("interpreter: zeroed array of structs can be indexed and assigned", () => {
        var a : [2]InterpBox = zeroed:unsafe<[2]InterpBox>()
        a[0] = InterpBox.make(5)
        a[1] = InterpBox.make(6)
        return a[0].value == 5 && a[1].value == 6
    })
}

// --- arrays / loops / variant sanity (currently passing; keep as regressions) ---

func interp_test_array_struct_field_assignment() {
    test("interpreter: array of struct element field assignment persists", () => {
        var a : [2]InterpBox = [InterpBox.make(1), InterpBox.make(2)]
        a[0].value = 100
        return a[0].value == 100 && a[1].value == 2
    })
}

func interp_test_nested_array_indexing() {
    test("interpreter: nested array literal indexing works", () => {
        var a = [[1, 2], [3, 4]]
        return a[0][0] == 1 && a[1][1] == 4
    })
}

func interp_test_partial_array_initializer() {
    test("interpreter: explicit-size array literal zero fills the remainder", () => {
        var a : [5]int = [1, 2]
        return a[0] == 1 && a[1] == 2 && a[4] == 0
    })
}

func interp_test_nested_loop_break_value() {
    test("interpreter: nested loop break values are independent", () => {
        var x = loop {
            loop {
                break 3
            }
            break 7
        }
        return x == 7
    })
}

func interp_test_global_mutation() {
    test("interpreter: module-level variable mutation works", () => {
        interp_side_effect_count = 0
        interp_side_effect_count = interp_side_effect_count + 5
        return interp_side_effect_count == 5
    })
}

func interp_test_variant_payload_access() {
    test("interpreter: variant payload is accessible in switch", () => {
        var v = InterpVariant.Num(7)
        switch(v) {
            Num(n) => {
                return n == 7
            }
            Str(s) => {
                return false
            }
        }
        return false
    })
}

func interp_recursive_loop_with_continue(n : int) : int {
    var s = 0
    var i = 0
    while(i < 3) {
        i = i + 1
        if(i == 2) { continue }
        s = s + n
    }
    if(n > 1) {
        s = s + interp_recursive_loop_with_continue(n - 1)
    }
    return s
}

func interp_recursive_loop_with_break(n : int) : int {
    var s = 0
    var i = 0
    while(i < 5) {
        if(i == 2) { break }
        s = s + n
        i = i + 1
    }
    if(n > 1) {
        s = s + interp_recursive_loop_with_break(n - 1)
    }
    return s
}

func interp_test_reentrant_loops() {
    test("interpreter: reentrant loop with continue across recursion", () => {
        return interp_recursive_loop_with_continue(3) == 12
    })
    test("interpreter: reentrant loop with break across recursion", () => {
        return interp_recursive_loop_with_break(3) == 12
    })
}

func interp_test_loop_value_break_in_if() {
    test("interpreter: loop break value from inside an if block", () => {
        var x = loop {
            var i = 0
            if(i == 0) {
                break 42
            }
            break 0
        }
        return x == 42
    })
}

func interp_test_nested_loop_value() {
    test("interpreter: nested loop values do not clobber each other", () => {
        var x = loop {
            var inner = loop {
                break 5
            }
            if(inner != 5) {
                break 0
            }
            break 9
        }
        return x == 9
    })
}

func interp_break_stops_siblings() : int {
    var log = 0
    var i = 0
    while(i < 5) {
        i = i + 1
        if(i == 2) {
            log = log + 100
            break
            log = log + 1000
        }
        log = log + 1
    }
    return log
}

func interp_continue_stops_siblings() : int {
    var log = 0
    var i = 0
    while(i < 4) {
        i = i + 1
        if(i == 2) {
            log = log + 100
            continue
            log = log + 1000
        }
        log = log + 1
    }
    return log
}

func interp_for_break_continue() : int {
    var log = 0
    for(var i = 0; i < 5; i++) {
        if(i == 1) { continue }
        if(i == 3) { break }
        log = log + i
    }
    return log
}

func interp_dowhile_continue() : int {
    var log = 0
    var i = 0
    do {
        i = i + 1
        if(i == 2) { continue }
        log = log + i
    } while(i < 4)
    return log
}

func interp_forin_break() : int {
    var sum = 0
    var arr = [1, 2, 3, 4]
    for(var x in arr) {
        if(x == 3) { break }
        sum = sum + x
    }
    return sum
}

func interp_forin_continue() : int {
    var sum = 0
    var arr = [1, 2, 3, 4]
    for(var x in arr) {
        if(x == 2) { continue }
        sum = sum + x
    }
    return sum
}

func interp_test_control_flow() {
    test("interpreter: break stops siblings in an enclosing if block", () => {
        return interp_break_stops_siblings() == 101
    })
    test("interpreter: continue stops siblings in an enclosing if block", () => {
        return interp_continue_stops_siblings() == 103
    })
    test("interpreter: for loop break/continue", () => {
        return interp_for_break_continue() == 2
    })
    test("interpreter: do-while continue evaluates the condition", () => {
        return interp_dowhile_continue() == 8
    })
    test("interpreter: for-in break", () => {
        return interp_forin_break() == 3
    })
    test("interpreter: for-in continue", () => {
        return interp_forin_continue() == 8
    })
}

// --- struct value semantics, copies and destructor timing ----------------------

func interp_struct_copy_param(p : InterpPair) : int {
    p.a = 999
    return p.a
}

func interp_pass_plain_twice() : int {
    var p = InterpPair { a : 1, b : 2 }
    return interp_struct_copy_param(p) + interp_struct_copy_param(p)
}

func interp_returned_struct_field() : int {
    var b = InterpBox.make(7)
    return b.value
}

func interp_nested_struct_read() : int {
    var inner = InterpBox.make(3)
    var outer = InterpPair { a : inner.value, b : 0 }
    return outer.a + inner.value
}

func interp_move_then_delete_once() : int {
    interp_delete_count = 0
    if(true) {
        var a = InterpBox.make(1)
        var b = a
    }
    return interp_delete_count
}

func interp_array_of_destructible() : int {
    var a = [InterpBox.make(1), InterpBox.make(2)]
    return a[0].value + a[1].value
}

func interp_loop_local_destructor() : int {
    interp_delete_count = 0
    for(var i = 0; i < 3; i++) {
        var b = InterpBox.make(i)
    }
    return interp_delete_count
}

func test_interp_struct_semantics() {
    test("interpreter: plain struct passed by value copies", () => {
        return interp_struct_copy_param(InterpPair { a : 1, b : 2 }) == 999
    })
    test("interpreter: plain struct passed twice is unaffected", () => {
        return interp_pass_plain_twice() == 1998
    })
    test("interpreter: returned struct field", () => {
        return interp_returned_struct_field() == 7
    })
    test("interpreter: nested struct field read", () => {
        return interp_nested_struct_read() == 6
    })
    test("interpreter: move then delete once", () => {
        return interp_move_then_delete_once() == 1
    })
    test("interpreter: array of destructible structs", () => {
        return interp_array_of_destructible() == 3
    })
    test("interpreter: loop local destructor runs each iteration", () => {
        return interp_loop_local_destructor() == 3
    })
}

func test_interp_regressions() {
    // Suspected interpreter bugs (expected to fail under --interpret until fixed)
    interp_test_switch_subject_evaluated_once()
    interp_test_assign_rhs_evaluated_once()
    interp_test_chain_assign_rhs_evaluated_once()
    interp_test_index_assign_rhs_evaluated_once()
    interp_test_zeroed_array_of_int()
    interp_test_zeroed_array_of_struct()

    // Regression coverage for behaviour that currently works
    interp_test_compound_assign_rhs_evaluated_once()
    interp_test_if_condition_evaluated_once()
    interp_test_var_init_evaluated_once()
    interp_test_return_expr_evaluated_once()
    interp_test_function_arg_evaluated_once()
    interp_test_index_expr_evaluated_once()
    interp_test_logical_short_circuit()
    interp_test_array_struct_field_assignment()
    interp_test_nested_array_indexing()
    interp_test_partial_array_initializer()
    interp_test_nested_loop_break_value()
    interp_test_global_mutation()
    interp_test_variant_payload_access()
    interp_test_reentrant_loops()
    interp_test_loop_value_break_in_if()
    interp_test_nested_loop_value()
    interp_test_control_flow()
}
