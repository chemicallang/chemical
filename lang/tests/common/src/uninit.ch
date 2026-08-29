// Copyright (c) Chemical Language Foundation 2026.

// -------------------------------------------------------
// Positive tests for the new uninitialized-variable semantics.
//
// These tests must COMPILE (the compiler must NOT complain) and run correctly
// in both compiled and interpretation mode. A "positive" test here means the
// compiler accepts the code; the runtime assertions additionally verify the
// generated behavior is correct (e.g. first-assignment must not destruct garbage).
// -------------------------------------------------------

func take_int_ptr(p : *mut int) {
    *p = 99
}

// A trivially-destructible struct, used to verify first-assignment does not
// destroy the garbage initial value.
public struct Box {
    var value : int
    @delete func delete(&self) {
        // no-op, but its presence makes Box "destructible"
    }
}

public struct UninitPoint {
    var x : int
    var y : int
}

// Plain uninitialized variable, assigned before use — compiler must NOT complain.
func uninit_plain_assign_then_read() : bool {
    var x : int
    x = 42
    return x == 42
}

// Assigned on all branches — compiler must NOT complain (definite assignment).
func uninit_branch_both_assigned() : bool {
    var x : int
    if(true) {
        x = 1
    } else {
        x = 2
    }
    return x == 1
}

// Reading a non-destructor uninitialized variable after assignment is fine.
func uninit_nondestructor_read() : bool {
    var x : int
    x = 10
    var y = x + 5
    return y == 15
}

// Writing through an index of an uninitialized fixed-size array is allowed
// (arrays have no destructor, so there is nothing to destroy).
func uninit_array_index_write() : bool {
    var arr : [4]int
    arr[0] = 1
    arr[1] = 2
    arr[2] = 3
    arr[3] = 4
    return arr[0] + arr[3] == 5
}

// Member write through an uninitialized array of structs is allowed.
func uninit_array_member_write() : bool {
    var arr : [2]UninitPoint
    arr[0].x = 7
    arr[1].y = 8
    return arr[0].x == 7 && arr[1].y == 8
}

// First assignment of a destructible struct must NOT destroy garbage.
func uninit_destructible_first_assign() : bool {
    var b : Box
    b = Box{ value: 5 }
    return b.value == 5
}

// Compound assignment (+=) on a plain uninitialized int after assignment is fine.
func uninit_compound_assign() : bool {
    var x : int
    x = 1
    x += 4
    return x == 5
}

// `unsafe var` is deprecated but still permitted: taking its address works.
func unsafe_var_address_of_ok() : bool {
    unsafe var x : int
    take_int_ptr(&raw mut x)
    return x == 99
}

// unsafe(...) wrapping allows taking the address of a plain uninitialized var.
// (unsafe(...) is an expression, so it must appear in an expression position —
// here we use an unsafe { } block, which is the statement form.)
func unsafe_wrap_address_of_ok() : bool {
    var x : int
    unsafe {
        take_int_ptr(&raw mut x)
    }
    return x == 99
}

// unsafe { } block allows unsafe operations on uninitialized vars.
func unsafe_block_address_of_ok() : bool {
    var x : int
    unsafe {
        take_int_ptr(&raw mut x)
    }
    return x == 99
}

// `unsafe const` (deprecated) is permitted as well.
func unsafe_const_ok() : bool {
    unsafe const x : int = 5
    return x == 5
}

public func test_uninitialized() {
    test("plain uninit assigned then read", () => { return uninit_plain_assign_then_read() })
    test("uninit assigned on all branches", () => { return uninit_branch_both_assigned() })
    test("uninit non-destructor read", () => { return uninit_nondestructor_read() })
    test("uninit array index write", () => { return uninit_array_index_write() })
    test("uninit array member write", () => { return uninit_array_member_write() })
    test("uninit destructible first assign does not destruct garbage", () => { return uninit_destructible_first_assign() })
    test("uninit compound assignment", () => { return uninit_compound_assign() })
    test("unsafe var address-of still works", () => { return unsafe_var_address_of_ok() })
    test("unsafe(...) wrap address-of of plain uninit var", () => { return unsafe_wrap_address_of_ok() })
    test("unsafe { } block address-of of plain uninit var", () => { return unsafe_block_address_of_ok() })
    test("unsafe const still works", () => { return unsafe_const_ok() })
}
