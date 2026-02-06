func ret_log_expr_1() : bool {
    return (false || false) && !(false && false)
}

public func test_dot_filter(name_w : *char) : bool {
    if (!(name_w[0] == '.' && name_w[1] == 0) &&
        !(name_w[0] == '.' &&
          name_w[1] == '.' &&
          name_w[2] == 0)) {
        return true;
    }
    return false;
}

public func test_dot_filter2(name_w : *u16) : bool {
    // return true if name should be processed
    // return false if name is "." or ".."

    if (!(name_w[0] == '.' as u16 && name_w[1] == 0) &&
        !(name_w[0] == '.' as u16 &&
          name_w[1] == '.' as u16 &&
          name_w[2] == 0)) {
        return true;
    }

    return false;
}

// small no-op to create call boundaries
public func noop_barrier(x : i32) : i32 { return x + 0; }

// 1) Force stack temporaries: copy chars to local allocas then test.
//    This reproduces the pattern in your big IR where values are stored into
//    small allocas and then loaded before comparisons.
public func temp_stack_copy_u16(name_w : *u16) : bool {
    // allocate locals (will generate allocas)
    var a0 : u16 = name_w[0];
    var a1 : u16 = name_w[1];
    var a2 : u16 = name_w[2];

    // store into local variables explicitly (encourages store/load in IR)
    var t0 : u16 = a0;
    var t1 : u16 = a1;
    var t2 : u16 = a2;

    // re-load (in source it's a simple read, but generator may produce loads)
    if (!(t0 == '.' as u16 && t1 == 0u16) &&
        !(t0 == '.' as u16 && t1 == '.' as u16 && t2 == 0u16)) {
        return true;
    }
    return false;
}

// 2) Interleaved calls between comparisons
//    Calls create basic-block/call boundaries and can alter lowering/rescheduling.
public func interleaved_calls_u16(name_w : *u16) : bool {
    var c0 : u16 = name_w[0];
    var c1 : u16 = name_w[1];
    var c2 : u16 = name_w[2];

    // interleave calls to force split blocks
    var x = noop_barrier(1);
    if (c0 == '.' as u16) {
        x = noop_barrier(x + 1);
        if (c1 == 0u16) { return false; }
        x = noop_barrier(x + 1);
        if (c1 == '.' as u16) {
            x = noop_barrier(x + 1);
            if (c2 == 0u16) { return false; }
        }
        return true;
    } else {
        x = noop_barrier(x + 1);
        return true;
    }
}

// 3) Pointer bitcasts and GEP with different index types
public func gep_and_bitcast(name_w : *u16) : bool {
    // bitcast to opaque pointer and back to force generator to emit casts
    var p : *void = name_w as *void;
    var p2 : *u16 = p as *u16;

    // compute GEPs in slightly different ways
    var second = p2[1];
    var third = p2[2];

    // similar test
    if (!(p2[0] == '.' as u16 && second == 0u16) &&
        !(p2[0] == '.' as u16 && second == '.' as u16 && third == 0u16)) {
        return true;
    }
    return false;
}

// 4) Multi-way merging to create non-trivial PHI shapes (3 preds)
public func multi_way_merge(name_w : *u16) : bool {
    var partA : bool = false;
    var partB : bool = false;

    if (name_w[0] == '.' as u16) {
        // block A
        if (name_w[1] == 0u16) {
            partA = true;
            // fallthrough to merge
        } else {
            // block B
            if (name_w[1] == '.' as u16 && name_w[2] == 0u16) {
                partB = true;
            } else {
                // block C (first char '.' but not "." or "..")
                // set nothing, but create an extra branch path
                var dummy : i32 = 42;
                dummy = noop_barrier(dummy);
            }
        }
    } else {
        // block D: first char not '.', another predecessor to merge
        var dummy2 : i32 = 7;
        dummy2 = noop_barrier(dummy2);
    }

    // now merge multiple predecessors and compute skip via combined condition
    // this should create phi merges in IR
    var skip : bool = false;
    if (partA || partB) {
        skip = true;
    } else {
        skip = false;
    }
    return !skip;
}

// 5) Recreate the exact pattern of storing last char into an alloca then loading it
//    (as in your big IR: store last char into alloca and then read it to choose branch)
public func store_last_then_compare(name_w : *u16) : bool {
    // compute length-like behavior (but small)
    var i : i32 = 0;
    while (i < 3 && name_w[i] != 0u16) {
        i = i + 1;
    }

    // store last char (if any) into a local that forces an alloca
    var last : u16 = 0u16;
    if (i > 0) {
        last = name_w[i - 1];
    } else {
        last = 0u16;
    }
    // now compare using the alloca-backed local
    if (!(name_w[0] == '.' as u16 && name_w[1] == 0u16) &&
        !(name_w[0] == '.' as u16 && name_w[1] == '.' as u16 && name_w[2] == 0u16)) {
        // make a small extra branch based on last to create more complex preds
        if (last == '\\' as u16) {
            return true;
        }
        return true;
    }
    return false;
}

// 6) Artificially long chain of tiny blocks that set booleans and eventually merge.
//    This stresses phi predecessor ordering and many small basic blocks like the large function.
public func long_chain_merge(name_w : *u16) : bool {
    var b0 : bool = false;
    var b1 : bool = false;
    var b2 : bool = false;
    var b3 : bool = false;

    if (name_w[0] == '.' as u16) {
        b0 = true;
    } else {
        b1 = true;
    }

    if (name_w[1] == 0u16) {
        b2 = true;
    } else {
        b3 = true;
    }

    // many merges
    var skip : bool = false;
    if (b0 && b2) skip = true;
    else if (b0 && b3) skip = false;
    else if (b1 && b2) skip = false;
    else skip = false;

    return !skip;
}

func test_bodmas() {
    test("4 + 2 / 2 == 5", () => {
        return (4 + 2 / 2) == 5;
    });
    test("4 / 2 + 2 == 4", () => {
        return (4 / 2 + 2) == 4;
    });
    test("3 * (4 + 2) - 8 / 2 == 5", () => {
        return (4 + 2 / 2) == 5;
    });
    test("3 * (4 + 2) - 8 / 2 == 14", () => {
        return (3 * (4 + 2) - 8 / 2) == 14;
    });
    test("8 / (2 + 2) * 3 - 1 == 5", () => {
        return (8 / (2 + 2) * 3 - 1) == 5;
    });
    test("(5 + 3) * 2 - 4 / (1 + 1) == 14", () => {
        return ((5 + 3) * 2 - 4 / (1 + 1)) == 14;
    });
    test("single braced value", () => {
        return (5 + (5)) == 10;
    })
    test("Simple addition and multiplication", () => {
        return (2 + 3 * 4) == 14;
    });
    test("Addition, subtraction, multiplication", () => {
        return (10 - 4 + 2 * 3) == 12;
    });
    test("Division and multiplication", () => {
        return (20 / 4 * 2) == 10;
    });
    test("Brackets and multiplication", () => {
        return ((5 + 3) * 2) == 16;
    });
    test("Brackets and division", () => {
        return (8 / (2 + 2)) == 2;
    });
    test("Complex expression", () => {
        return ((5 + 3) * 2 - 4 / (1 + 1)) == 14;
    });
    test("Nested brackets", () => {
        return (((4 * 2) + 3) * (10 / 5)) == 22;
    });
    test("Expression with negative numbers", () => {
        return (-3 * (-4 + 2)) == 6;
    });
    test("double with integer addition", () => {
        return 4 + 1.5 == 5.5;
    });
    test("Expression with decimal numbers", () => {
        return (1.5 * (4 + 1.5)) == 8.25;
    });
    test("Expression with variables", () => {
        var a = 5;
        var b = 3;
        return ((a + b) * 2 - 4 / (1 + 1)) == 14;
    });
    test("Expression with modulo", () => {
        return (10 % 3 == 1);
    });
    test("Expression with mixed operations", () => {
        return (5 + 3) * 2 - 4 / (1 + 1) == 14;
    });
    test("Longer expression with mixed operations", () => {
        return (2 * (3 + 4) / 2 - (7 % 3) + 5) == 11;
    });
    test("Expression with nested brackets and subtraction", () => {
        return (10 - (2 * (3 + 4) / 2) + (7 % 3) - 5) == -1;
    });
    test("Expression with multiple division and subtraction", () => {
        return (100 / 5 / 2 - 4 / 2 - 3) == 5;
    });
    test("Expression with consecutive multiplication", () => {
        return (2 * 3 * 4 * 5) == 120;
    });
    test("Expression with consecutive division", () => {
        return (100 / 5 / 2) == 10;
    });
    test("Expression with consecutive addition", () => {
        return (1 + 2 + 3 + 4 + 5) == 15;
    });
    test("Expression with consecutive subtraction", () => {
        return (20 - 5 - 4 - 3 - 2 - 1) == 5;
    });
    test("Expression with mixed operations and negative numbers", () => {
        return (10 - (-3 * (4 + 2)) + 8 / 2) == 32;
    });
    test("Expression with complex nested operations", () => {
        return ((10 * (5 + (3 * 2) - 4 / 2)) - (2 * (7 % 3))) == 88;
    });
    test("Expression with repeated brackets", () => {
        return (((((2 + 3) * 4) / 2) - 1) + ((((8 / 2) + 1) * 3) - 2)) == 22;
    });
    test("Negative braced expression", () => {
        return -(2 + 3) == -5;
    });
    test("pointer math in expressions work", () => {
        var i = 22;
        const p = &i;
        const j = p + 1;
        const k = j - 1;
        return *k == 22;
    })
    test("logical right shift", () => {
        var value : uint = 0b10010010; // 146 in decimal
        var expected : uint = 0b00100100; // 36 in decimal
        var result : uint = value >> 2u; // Logical right shift (unsigned)
        return result == expected;
    })
    test("arithmetic right shift", () => {
        var value : int = -8; // 0xFFFFFFF8 in 32-bit signed representation
        var expected : int = -2; // Right shift retains the sign
        var result : int = value >> 2; // Arithmetic right shift (signed)
        return result == expected;
    })
    test("logical left shift", () => {
        var value : uint = 0b00010010; // 18 in decimal
        var expected : uint = 0b01001000; // 72 in decimal
        var result : uint = value << 2u; // Logical left shift (unsigned)
        return result == expected;
    })
    test("arithmetic left shift", () => {
        var value : int = -8; // -8 in signed
        var expected : int = -32; // Left shift retains the sign
        var result : int = value << 2; // Arithmetic left shift
        return result == expected;
    })
    test("parenthesized expressions can be casted", () => {
        var v = (10 + 20) as ushort
        return v == 30
    })
    test("second value in expression can be on next line, if it contains operator on previous line", () => {
        var v = 33
        var j = 44
        if(v == 33 ||
           j == 44
        ) {
            return true;
        } else {
            return false;
        }
    })
    test("nested logical expressions work - 1", () => {
        return ret_log_expr_1() == false;
    })
    test("nested logical expressions work - 2", () => {
        const res = (false || false) && !(false && false)
        return !res;
    })
    test("nested logical expressions work - 3", () => {
        var res = (false || false) && !(false && false)
        return !res;
    })
    test("complex branching based on logical expression - 1", () => {
        return test_dot_filter(".") == false;
    })
    test("complex branching based on logical expression - 2", () => {
        return test_dot_filter("..") == false;
    })
    test("complex branching based on logical expression - 3", () => {
        return test_dot_filter("a");
    })
    test("complex branching based on logical expression - 4", () => {
        return test_dot_filter(".a")
    })
    test("complex branching based on logical expression - 5", () => {
        const arr = ['.' as u16, 0u16]
        return test_dot_filter2(&arr[0]) == false;
    })
    test("complex branching based on logical expression - 6", () => {
        const arr = ['.' as u16, '.' as u16, 0u16]
        return test_dot_filter2(&arr[0]) == false;
    })
    test("complex branching based on logical expression - 7", () => {
        const arr = ['a' as u16, 0u16]
        return test_dot_filter2(&arr[0]);
    })
    test("complex branching based on logical expression - 8", () => {
        const arr = ['.' as u16, 'a' as u16, 0u16]
        return test_dot_filter2(&arr[0])
    })

    // run every new test across those vectors
    test("phi xor lowering \"temp_stack_copy_u16(.)\"", () => { return temp_stack_copy_u16(&dot_u16[0]) == false });
    test("phi xor lowering \"temp_stack_copy_u16(..)\"", () => { return temp_stack_copy_u16(&dotdot_u16[0]) == false });
    test("phi xor lowering \"temp_stack_copy_u16(a)\"", () => { return temp_stack_copy_u16(&a_u16[0]) == true });
    test("phi xor lowering \"temp_stack_copy_u16(.a)\"", () => { return temp_stack_copy_u16(&dota_u16[0]) == true });

    test("phi xor lowering \"interleaved_calls_u16(.)\"", () => { return interleaved_calls_u16(&dot_u16[0]) == false });
    test("phi xor lowering \"interleaved_calls_u16(..)\"", () => { return interleaved_calls_u16(&dotdot_u16[0]) == false });
    test("phi xor lowering \"interleaved_calls_u16(a)\"", () => { return interleaved_calls_u16(&a_u16[0]) == true });
    test("phi xor lowering \"interleaved_calls_u16(.a)\"", () => { return interleaved_calls_u16(&dota_u16[0]) == true });

    test("phi xor lowering \"gep_and_bitcast(.)\"", () => { return gep_and_bitcast(&dot_u16[0]) == false });
    test("phi xor lowering \"gep_and_bitcast(..)\"", () => { return gep_and_bitcast(&dotdot_u16[0]) == false });
    test("phi xor lowering \"gep_and_bitcast(a)\"", () => { return gep_and_bitcast(&a_u16[0]) == true });
    test("phi xor lowering \"gep_and_bitcast(.a)\"", () => { return gep_and_bitcast(&dota_u16[0]) == true });

    test("phi xor lowering \"multi_way_merge(.)\"", () => { return multi_way_merge(&dot_u16[0]) == false });
    test("phi xor lowering \"multi_way_merge(..)\"", () => { return multi_way_merge(&dotdot_u16[0]) == false });
    test("phi xor lowering \"multi_way_merge(a)\"", () => { return multi_way_merge(&a_u16[0]) == true });
    test("phi xor lowering \"multi_way_merge(.a)\"", () => { return multi_way_merge(&dota_u16[0]) == true });

    test("phi xor lowering \"store_last_then_compare(.)\"", () => { return store_last_then_compare(&dot_u16[0]) == false });
    test("phi xor lowering \"store_last_then_compare(..)\"", () => { return store_last_then_compare(&dotdot_u16[0]) == false });
    test("phi xor lowering \"store_last_then_compare(a)\"", () => { return store_last_then_compare(&a_u16[0]) == true });
    test("phi xor lowering \"store_last_then_compare(.a)\"", () => { return store_last_then_compare(&dota_u16[0]) == true });

    test("phi xor lowering \"long_chain_merge(.)\"", () => { return long_chain_merge(&dot_u16[0]) == false });
    test("phi xor lowering \"long_chain_merge(..)\"", () => { return long_chain_merge(&dotdot_u16[0]) == true });
    test("phi xor lowering \"long_chain_merge(a)\"", () => { return long_chain_merge(&a_u16[0]) == true });
    test("phi xor lowering \"long_chain_merge(.a)\"", () => { return long_chain_merge(&dota_u16[0]) == true });

}

// test vectors (., .., a, .a)
var dot_u16 : [3]u16 = ['.' as u16, 0u16, 0u16];
var dotdot_u16 : [4]u16 = ['.' as u16, '.' as u16, 0u16, 0u16];
var a_u16 : [3]u16 = ['a' as u16, 0u16, 0u16];
var dota_u16 : [4]u16 = ['.' as u16, 'a' as u16, 0u16, 0u16];