// Copyright (c) Chemical Language Foundation 2026.

// -------------------------------------------------------
// Comptime Feature Tests
// Tests for various comptime features including:
// bitwise operations, loops, casting, logical ops, etc.
// -------------------------------------------------------

// ---------- Comptime bitwise operations ----------

comptime func comptime_bitwise_and(a : int, b : int) : int {
    return a & b;
}

comptime func comptime_bitwise_or(a : int, b : int) : int {
    return a | b;
}

comptime func comptime_bitwise_xor(a : int, b : int) : int {
    return a ^ b;
}

comptime func comptime_left_shift(a : int, b : int) : int {
    return a << b;
}

comptime func comptime_right_shift(a : int, b : int) : int {
    return a >> b;
}

comptime func comptime_bitwise_not(a : int) : int {
    return ~a;
}

// ---------- While loop in comptime ----------

comptime func comptime_while_sum(limit : int) : int {
    var sum = 0;
    var i = 0;
    while(i < limit) {
        sum += i;
        i++;
    }
    return sum;
}

// ---------- For loop in comptime ----------

comptime func comptime_for_sum(limit : int) : int {
    var sum = 0;
    for(var i = 0; i < limit; i++) {
        sum += i;
    }
    return sum;
}

// ---------- Comptime do-while loop ----------

comptime func comptime_do_while_sum(limit : int) : int {
    var sum = 0;
    var i = 0;
    do {
        sum += i;
        i++;
    } while(i < limit);
    return sum;
}

// ---------- Casting at comptime ----------

comptime func comptime_cast_int_to_long(value : int) : long {
    return value as long;
}

comptime func comptime_cast_long_to_int(value : long) : int {
    return value as int;
}

// ---------- Logical operations ----------

comptime func comptime_logical_and(a : bool, b : bool) : bool {
    return a && b;
}

comptime func comptime_logical_or(a : bool, b : bool) : bool {
    return a || b;
}

comptime func comptime_logical_not(a : bool) : bool {
    return !a;
}

// ---------- Comptime with modulo ----------

comptime func comptime_modulo(a : int, b : int) : int {
    return a % b;
}

// ---------- Break with value in while loop ----------

comptime func comptime_break_value() : int {
    var sum = 0;
    var i = 0;
    while(i < 100) {
        if(i == 5) {
            break;
        }
        sum += i;
        i++;
    }
    return sum;
}

// ---------- Comptime with continue ----------

comptime func comptime_continue_sum() : int {
    var sum = 0;
    var i = 0;
    while(i < 10) {
        i++;
        if(i == 3 || i == 7) {
            continue;
        }
        sum += i;
    }
    return sum;
}

// ---------- Nested scopes inside comptime ----------

comptime func comptime_nested_scopes() : int {
    var x = 5;
    {
        var y = 10;
        x = x + y;
        {
            var z = 20;
            x = x + z;
        }
    }
    return x;
}

// ---------- Simple comptime block expressions (single expression inside) ----------

comptime func comptime_block_expr_test() : bool {
    return comptime { (5 + 3) == 8 }
}

comptime func comptime_block_bitwise_and() : bool {
    return comptime { (12 & 5) == 4 }
}

comptime func comptime_block_bitwise_or() : bool {
    return comptime { (12 | 5) == 13 }
}

comptime func comptime_block_left_shift() : bool {
    return comptime { (3 << 2) == 12 }
}

comptime func comptime_block_right_shift() : bool {
    return comptime { (16 >> 2) == 4 }
}

comptime func comptime_block_modulo() : bool {
    return comptime { (10 % 3) == 1 }
}

comptime func comptime_block_combined_bitwise() : bool {
    return comptime { ((12 & 5) | (3 << 2)) == 12 }
}

// ---------- For-in loop in comptime (array iteration) ----------

comptime func comptime_forin_array_sum() : int {
    const arr : []int = [1, 2, 3, 4, 5]
    var sum = 0
    for(var i in arr) {
        sum += i
    }
    return sum
}

comptime func comptime_forin_chars() : int {
    const arr : []char = ['h', 'e', 'l', 'l', 'o']
    var cnt = 0
    for(var c in arr) {
        cnt++
    }
    return cnt
}

comptime func comptime_forin_with_break() : int {
    const arr : []int = [1, 2, 3, 4, 5]
    var sum = 0
    for(var i in arr) {
        if(i == 3) {
            break
        }
        sum += i
    }
    return sum
}

comptime func comptime_forin_with_continue() : int {
    const arr : []int = [1, 2, 3, 4, 5]
    var sum = 0
    for(var i in arr) {
        if(i == 3 || i == 5) {
            continue
        }
        sum += i
    }
    return sum
}

comptime func comptime_forin_with_index() : int {
    const arr : []int = [10, 20, 30, 40]
    var sum = 0
    for(var i, idx in arr) {
        sum += i + idx as int
    }
    return sum
}

// ---------- Struct field mutation in comptime ----------

struct ComptimeMutablePoint {
    var x : int
    var y : int
}

comptime func comptime_mutate_struct() : int {
    var p = ComptimeMutablePoint { x : 10, y : 20 }
    p.x = 30
    p.y = 40
    return p.x + p.y;
}

comptime func comptime_mutate_struct_in_loop() : int {
    var p = ComptimeMutablePoint { x : 0, y : 0 }
    var i = 0;
    while(i < 5) {
        p.x = p.x + i;
        i++;
    }
    return p.x;
}

comptime func comptime_struct_method_mutation() : int {
    var p = ComptimeMutablePoint { x : 5, y : 15 }
    p.x = p.y + p.x
    return p.x;
}

// ---------- Struct with nested struct mutation ----------

// ---------- Multiple struct field assignments ----------

comptime func comptime_multiple_field_assign() : int {
    var p = ComptimeMutablePoint { x : 0, y : 0 }
    p.x = 5
    p.y = 10
    p.x = p.x + p.y
    p.y = p.x * 2
    return p.x + p.y;
}

// ---------- Comptime destructor tests ----------

struct ComptimeDestructCount {
    @delete
    func delete(&self) {
        // no-op destructor — just verifies the destructor body can be interpreted
    }
}

struct ComptimeDestructWithBody {
    var data : int
    @delete
    func delete(&self) {
        self.data = self.data + 1
    }
}

comptime func comptime_destruct_noop() : bool {
    var d = ComptimeDestructCount {}
    return true
}

comptime func comptime_destruct_in_block() : bool {
    {
        var d = ComptimeDestructCount {}
    }
    return true
}

comptime func comptime_destruct_in_if() : bool {
    if(true) {
        var d = ComptimeDestructCount {}
    }
    return true
}

comptime func comptime_destruct_in_while() : bool {
    var i = 0
    while(i < 3) {
        var d = ComptimeDestructCount {}
        i++
    }
    return true
}

comptime func comptime_destruct_for_loop() : bool {
    for(var i = 0; i < 3; i++) {
        var d = ComptimeDestructCount {}
    }
    return true
}

// ---------- Test runner ----------

func test_comptime_features() {

    // Bitwise operations
    test("comptime bitwise AND works", () => {
        return comptime_bitwise_and(12, 5) == 4;
    })
    test("comptime bitwise OR works", () => {
        return comptime_bitwise_or(12, 5) == 13;
    })
    test("comptime bitwise XOR works", () => {
        return comptime_bitwise_xor(12, 5) == 9;
    })
    test("comptime left shift works", () => {
        return comptime_left_shift(3, 2) == 12;
    })
    test("comptime right shift works", () => {
        return comptime_right_shift(16, 2) == 4;
    })
    test("comptime bitwise NOT works", () => {
        return comptime_bitwise_not(5) == ~5;
    })

    // Loops
    test("comptime while loop works", () => {
        return comptime_while_sum(5) == 10;
    })
    test("comptime for loop works", () => {
        return comptime_for_sum(5) == 10;
    })
    test("comptime do-while loop works", () => {
        return comptime_do_while_sum(5) == 10;
    })
    test("comptime while loop with break works", () => {
        return comptime_break_value() == 10;
    })
    test("comptime while loop with continue works", () => {
        return comptime_continue_sum() == 45;
    })

    // Casting
    test("comptime int to long cast works", () => {
        return comptime_cast_int_to_long(42) == 42;
    })
    test("comptime long to int cast works", () => {
        return comptime_cast_long_to_int(100 as long) == 100;
    })

    // Logical operations
    test("comptime logical AND works", () => {
        return comptime_logical_and(true, true) && !comptime_logical_and(true, false);
    })
    test("comptime logical OR works", () => {
        return comptime_logical_or(true, false) && !comptime_logical_or(false, false);
    })
    test("comptime logical NOT works", () => {
        return comptime_logical_not(false) && !comptime_logical_not(true);
    })

    // Modulo
    test("comptime modulo works", () => {
        return comptime_modulo(10, 3) == 1;
    })

    // Nested scopes
    test("comptime nested scopes work", () => {
        return comptime_nested_scopes() == 35;
    })

    // Comptime block expressions (simple single-expression blocks)
    test("comptime block expression works", () => {
        return comptime_block_expr_test();
    })
    test("comptime: bitwise AND in comptime block", () => {
        return comptime_block_bitwise_and();
    })
    test("comptime: bitwise OR in comptime block", () => {
        return comptime_block_bitwise_or();
    })
    test("comptime: left shift in comptime block", () => {
        return comptime_block_left_shift();
    })
    test("comptime: right shift in comptime block", () => {
        return comptime_block_right_shift();
    })
    test("comptime: modulo in comptime block", () => {
        return comptime_block_modulo();
    })
    test("comptime: combined bitwise in comptime block", () => {
        return comptime_block_combined_bitwise();
    })

    // For-in loop tests
    test("comptime for-in over array sums elements", () => {
        return comptime_forin_array_sum() == 15;
    })
    test("comptime for-in over char array counts", () => {
        return comptime_forin_chars() == 5;
    })
    test("comptime for-in loop with break works", () => {
        return comptime_forin_with_break() == 3;
    })
    test("comptime for-in loop with continue works", () => {
        return comptime_forin_with_continue() == 7;
    })
    test("comptime for-in loop with index works", () => {
        return comptime_forin_with_index() == 106;
    })

    // Struct mutation tests
    test("comptime struct field mutation works", () => {
        return comptime_mutate_struct() == 70;
    })
    test("comptime struct field mutation in loop works", () => {
        return comptime_mutate_struct_in_loop() == 10;
    })
    test("comptime struct method mutation works", () => {
        return comptime_struct_method_mutation() == 20;
    })
    test("comptime multiple struct field assignments work", () => {
        return comptime_multiple_field_assign() == 45;
    })

    // Comptime destructor tests
    test("comptime destructor no-op works", () => {
        return comptime_destruct_noop();
    })
    test("comptime destructor called in block scope works", () => {
        return comptime_destruct_in_block();
    })
    test("comptime destructor called in if scope works", () => {
        return comptime_destruct_in_if();
    })
    test("comptime destructor called in while loop scope works", () => {
        return comptime_destruct_in_while();
    })
    test("comptime destructor called in for loop scope works", () => {
        return comptime_destruct_for_loop();
    })
}
