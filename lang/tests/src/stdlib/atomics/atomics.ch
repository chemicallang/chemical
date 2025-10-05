// =================== helpers (public API only used in tests) ===================

var s64 : u64 = 0

func reset_u64(v: u64) : *mut u64 {
    s64 = v
    return &mut s64
}

var s32 : u32 = 0

func reset_u32(v: u32) : *mut u32 {
    s32 = v
    return &mut s32
}

var s16 : u16 = 0

func reset_u16(v: u16) : *mut u16 {
    s16 = v
    return &mut s16
}

var s8 : u8 = 0

func reset_u8(v: u8) : *mut u8 {
    s8 = v
    return &mut s8
}

// =================== fence tests ===================

func test_atomic_fence_seq_cst() : bool {
    atomic_fence(memory_order.seq_cst)
    return true
}

func test_atomic_fence_acquire_release_relaxed() : bool {
    atomic_fence(memory_order.acquire)
    atomic_fence(memory_order.release)
    return true
}

// =================== u64 tests ===================

func test_u64_load_store() : bool {
    var p = reset_u64(0)
    atomic_store_u64(p, 1234567890123456789, memory_order.release)
    var v = atomic_load_u64(p, memory_order.acquire)
    return v == 1234567890123456789
}

func test_u64_exchange() : bool {
    var p = reset_u64(7)
    var old = atomic_exchange_u64(p, 99, memory_order.seq_cst)
    var now = atomic_load_u64(p)
    return old == 7 && now == 99
}

func test_u64_fetch_add_sub() : bool {
    var p = reset_u64(10)
    var old = atomic_fetch_add_u64(p, 5, memory_order.seq_cst)
    if(!(old == 10 && atomic_load_u64(p) == 15)) return false
    var old2 = atomic_fetch_sub_u64(p, 3, memory_order.seq_cst)
    return old2 == 15 && atomic_load_u64(p) == 12
}

func test_u64_bitwise_and_or_xor() : bool {
    var p = reset_u64(0b11110000)
    var old_and = atomic_fetch_and_u64(p, 0b10101010, memory_order.seq_cst)
    if (!(old_and == 0b11110000 && atomic_load_u64(p) == 0b10100000)) return false
    var old_or = atomic_fetch_or_u64(p, 0b00001111, memory_order.seq_cst)
    if (!(old_or == 0b10100000 && atomic_load_u64(p) == 0b10101111)) return false
    var old_xor = atomic_fetch_xor_u64(p, 0b11111111, memory_order.seq_cst)
    if (!(old_xor == 0b10101111 && atomic_load_u64(p) == 0b01010000)) return false
    return true
}

func test_u64_compare_exchange_strong_success() : bool {
    var p = reset_u64(42)
    var expected: u64 = 42
    var ok = atomic_compare_exchange_strong_u64(p, &mut expected, 4242, memory_order.acq_rel, memory_order.acquire)
    return ok == true && atomic_load_u64(p) == 4242
}

func test_u64_compare_exchange_strong_fail() : bool {
    var p = reset_u64(500)
    var expected: u64 = 111
    var ok = atomic_compare_exchange_strong_u64(p, &mut expected, 222, memory_order.seq_cst, memory_order.seq_cst)
    // on failure expected should be updated to actual stored value (500)
    return ok == false && atomic_load_u64(p) == 500 && expected == 500
}

func test_u64_compare_exchange_weak_success() : bool {
    var p = reset_u64(13)
    var expected: u64 = 13
    // retry a few times to tolerate spurious failure (weak)
    for(var i = 0;i < 10; i++) {
        var ok = atomic_compare_exchange_weak_u64(p, &mut expected, 31, memory_order.seq_cst, memory_order.seq_cst)
        if (ok == true) {
            return atomic_load_u64(p) == 31
        }
    }
    return false
}

func test_u64_compare_exchange_weak_fail() : bool {
    var p = reset_u64(77)
    var expected: u64 = 5
    var ok = atomic_compare_exchange_weak_u64(p, &mut expected, 99, memory_order.seq_cst, memory_order.seq_cst)
    return ok == false && expected == 77 && atomic_load_u64(p) == 77
}

func test_u64_overflow_wrap() : bool {
    var p = reset_u64(0xFFFFFFFFFFFFFFFF)
    var old = atomic_fetch_add_u64(p, 1, memory_order.seq_cst)
    return old == 0xFFFFFFFFFFFFFFFF && atomic_load_u64(p) == 0
}

// =================== u32 tests ===================

func test_u32_load_store() : bool {
    var p = reset_u32(0)
    atomic_store_u32(p, 0xDEADBEEF, memory_order.release)
    var v = atomic_load_u32(p, memory_order.acquire)
    return v == 0xDEADBEEF
}

func test_u32_exchange() : bool {
    var p = reset_u32(1234)
    var old = atomic_exchange_u32(p, 5678, memory_order.seq_cst)
    return old == 1234 && atomic_load_u32(p) == 5678
}

func test_u32_fetch_add_sub() : bool {
    var p = reset_u32(100)
    var old = atomic_fetch_add_u32(p, 50, memory_order.seq_cst)
    if(!(old == 100 && atomic_load_u32(p) == 150)) return false
    var old2 = atomic_fetch_sub_u32(p, 25, memory_order.seq_cst)
    return old2 == 150 && atomic_load_u32(p) == 125
}

func test_u32_bitwise_and_or_xor() : bool {
    var p = reset_u32(0xFF00FF00)
    var old_and = atomic_fetch_and_u32(p, 0x0F0F0F0F, memory_order.seq_cst)
    if(!(old_and == 0xFF00FF00 && atomic_load_u32(p) == (0xFF00FF00 & 0x0F0F0F0F))) return false
    var old_or = atomic_fetch_or_u32(p, 0x0000FFFF, memory_order.seq_cst)
    if(!(old_or == (0xFF00FF00 & 0x0F0F0F0F) && atomic_load_u32(p) == ((0xFF00FF00 & 0x0F0F0F0F) | 0x0000FFFF))) return false
    var old_xor = atomic_fetch_xor_u32(p, 0xAAAAAAAA, memory_order.seq_cst)
    if(!(atomic_load_u32(p) == (((0xFF00FF00 & 0x0F0F0F0F) | 0x0000FFFF) ^ 0xAAAAAAAA))) return false
    return true
}

func test_u32_compare_exchange_strong_success() : bool {
    var p = reset_u32(200)
    var expected: u32 = 200
    var ok = atomic_compare_exchange_strong_u32(p, &mut expected, 300, memory_order.acquire, memory_order.relaxed)
    return ok == true && atomic_load_u32(p) == 300
}

func test_u32_compare_exchange_strong_fail() : bool {
    var p = reset_u32(400)
    var expected: u32 = 1
    var ok = atomic_compare_exchange_strong_u32(p, &mut expected, 2, memory_order.seq_cst, memory_order.seq_cst)
    return ok == false && expected == 400 && atomic_load_u32(p) == 400
}

func test_u32_compare_exchange_weak_success() : bool {
    var p = reset_u32(8)
    var expected: u32 = 8
    for(var i = 0;i < 10;i++) {
        var ok = atomic_compare_exchange_weak_u32(p, &mut expected, 16, memory_order.seq_cst, memory_order.seq_cst)
        if (ok == true) {
            return atomic_load_u32(p) == 16
        }
    }
    return false
}

func test_u32_compare_exchange_weak_fail() : bool {
    var p = reset_u32(9)
    var expected: u32 = 123
    var ok = atomic_compare_exchange_weak_u32(p, &mut expected, 45, memory_order.seq_cst, memory_order.seq_cst)
    return ok == false && expected == 9 && atomic_load_u32(p) == 9
}

func test_u32_overflow_wrap() : bool {
    var p = reset_u32(0xFFFFFFFF)
    var old = atomic_fetch_add_u32(p, 1, memory_order.seq_cst)
    return old == 0xFFFFFFFF && atomic_load_u32(p) == 0
}

// =================== u16 tests ===================

func test_u16_load_store() : bool {
    var p = reset_u16(0)
    atomic_store_u16(p, 0xABCD, memory_order.release)
    var v = atomic_load_u16(p, memory_order.acquire)
    return v == 0xABCD
}

func test_u16_exchange() : bool {
    var p = reset_u16(7)
    var old = atomic_exchange_u16(p, 123, memory_order.seq_cst)
    return old == 7 && atomic_load_u16(p) == 123
}

func test_u16_fetch_add_sub() : bool {
    var p = reset_u16(1000)
    var old = atomic_fetch_add_u16(p, 2000, memory_order.seq_cst)
    if(!(old == 1000 && atomic_load_u16(p) == 3000)) return false
    var old2 = atomic_fetch_sub_u16(p, 500, memory_order.seq_cst)
    return old2 == 3000 && atomic_load_u16(p) == 2500
}

func test_u16_bitwise_and_or_xor() : bool {
    var p = reset_u16(0b1111000011110000)
    var old_and = atomic_fetch_and_u16(p, 0b1010101010101010, memory_order.seq_cst)
    if(!(old_and == 0b1111000011110000 && atomic_load_u16(p) == (0b1111000011110000 & 0b1010101010101010))) return false
    var old_or = atomic_fetch_or_u16(p, 0b0000111100001111, memory_order.seq_cst)
    if(!(atomic_load_u16(p) == ((0b1111000011110000 & 0b1010101010101010) | 0b0000111100001111))) return false
    var old_xor = atomic_fetch_xor_u16(p, 0xFFFF, memory_order.seq_cst)
    return old_xor == ((0b1111000011110000 & 0b1010101010101010) | 0b0000111100001111)
}

func test_u16_compare_exchange_strong_success() : bool {
    var p = reset_u16(55)
    var expected: u16 = 55
    var ok = atomic_compare_exchange_strong_u16(p, &mut expected, 66, memory_order.acquire, memory_order.relaxed)
    return ok == true && atomic_load_u16(p) == 66
}

func test_u16_compare_exchange_strong_fail() : bool {
    var p = reset_u16(77)
    var expected: u16 = 2
    var ok = atomic_compare_exchange_strong_u16(p, &mut expected, 3, memory_order.seq_cst, memory_order.seq_cst)
    return ok == false && expected == 77 && atomic_load_u16(p) == 77
}

func test_u16_compare_exchange_weak_success() : bool {
    var p = reset_u16(21)
    var expected: u16 = 21
    for(var i = 0;i < 10; i++) {
        var ok = atomic_compare_exchange_weak_u16(p, &mut expected, 42, memory_order.seq_cst, memory_order.seq_cst)
        if (ok == true) { return atomic_load_u16(p) == 42 }
    }
    return false
}

func test_u16_compare_exchange_weak_fail() : bool {
    var p = reset_u16(99)
    var expected: u16 = 0
    var ok = atomic_compare_exchange_weak_u16(p, &mut expected, 1, memory_order.seq_cst, memory_order.seq_cst)
    return ok == false && expected == 99 && atomic_load_u16(p) == 99
}

func test_u16_overflow_wrap() : bool {
    var p = reset_u16(0xFFFF)
    var old = atomic_fetch_add_u16(p, 1, memory_order.seq_cst)
    return old == 0xFFFF && atomic_load_u16(p) == 0
}

// =================== u8 tests ===================

func test_u8_load_store() : bool {
    var p = reset_u8(0)
    atomic_store_u8(p, 0x7F, memory_order.release)
    var v = atomic_load_u8(p, memory_order.acquire)
    return v == 0x7F
}

func test_u8_exchange() : bool {
    var p = reset_u8(3)
    var old = atomic_exchange_u8(p, 8, memory_order.seq_cst)
    return old == 3 && atomic_load_u8(p) == 8
}

func test_u8_fetch_add_sub() : bool {
    var p = reset_u8(200)
    var old = atomic_fetch_add_u8(p, 30, memory_order.seq_cst)
    if(!(old == 200 && atomic_load_u8(p) == 230)) return false
    var old2 = atomic_fetch_sub_u8(p, 50, memory_order.seq_cst)
    return old2 == 230 && atomic_load_u8(p) == 180
}

func test_u8_bitwise_and_or_xor() : bool {
    var p = reset_u8(0b11001100)
    var old_and = atomic_fetch_and_u8(p, 0b10101010, memory_order.seq_cst)
    if(!(old_and == 0b11001100 && atomic_load_u8(p) == (0b11001100 & 0b10101010))) return false
    var old_or = atomic_fetch_or_u8(p, 0b00001111, memory_order.seq_cst)
    if(!(atomic_load_u8(p) == ((0b11001100 & 0b10101010) | 0b00001111))) return false
    var old_xor = atomic_fetch_xor_u8(p, 0xFF, memory_order.seq_cst)
    return old_xor == ((0b11001100 & 0b10101010) | 0b00001111)
}

func test_u8_compare_exchange_strong_success() : bool {
    var p = reset_u8(10)
    var expected: u8 = 10
    var ok = atomic_compare_exchange_strong_u8(p, &mut expected, 20, memory_order.acquire, memory_order.release)
    return ok == true && atomic_load_u8(p) == 20
}

func test_u8_compare_exchange_strong_fail() : bool {
    var p = reset_u8(30)
    var expected: u8 = 5
    var ok = atomic_compare_exchange_strong_u8(p, &mut expected, 6, memory_order.seq_cst, memory_order.seq_cst)
    return ok == false && expected == 30 && atomic_load_u8(p) == 30
}

func test_u8_compare_exchange_weak_success() : bool {
    var p = reset_u8(4)
    var expected: u8 = 4
    for(var i = 0; i < 10; i++){
        var ok = atomic_compare_exchange_weak_u8(p, &mut expected, 9, memory_order.seq_cst, memory_order.seq_cst)
        if (ok == true) { return atomic_load_u8(p) == 9 }
    }
    return false
}

func test_u8_compare_exchange_weak_fail() : bool {
    var p = reset_u8(66)
    var expected: u8 = 12
    var ok = atomic_compare_exchange_weak_u8(p, &mut expected, 77, memory_order.seq_cst, memory_order.seq_cst)
    return ok == false && expected == 66 && atomic_load_u8(p) == 66
}

func test_u8_overflow_wrap() : bool {
    var p = reset_u8(0xFF)
    var old = atomic_fetch_add_u8(p, 1, memory_order.seq_cst)
    return old == 0xFF && atomic_load_u8(p) == 0
}

// =================== main - run everything ===================

func test_atomics() {

    // fence
    test("atomic: atomic_fence_seq_cst", test_atomic_fence_seq_cst)
    test("atomic: atomic_fence_acquire_release_relaxed", test_atomic_fence_acquire_release_relaxed)

    // u64
    test("atomic: u64_load_store", test_u64_load_store)
    test("atomic: u64_exchange", test_u64_exchange)
    test("atomic: u64_fetch_add_sub", test_u64_fetch_add_sub)
    test("atomic: u64_bitwise_and_or_xor", test_u64_bitwise_and_or_xor)
    test("atomic: u64_compare_exchange_strong_success", test_u64_compare_exchange_strong_success)
    test("atomic: u64_compare_exchange_strong_fail", test_u64_compare_exchange_strong_fail)
    test("atomic: u64_compare_exchange_weak_success", test_u64_compare_exchange_weak_success)
    test("atomic: u64_compare_exchange_weak_fail", test_u64_compare_exchange_weak_fail)
    test("atomic: u64_overflow_wrap", test_u64_overflow_wrap)

    // u32
    test("atomic: u32_load_store", test_u32_load_store)
    test("atomic: u32_exchange", test_u32_exchange)
    test("atomic: u32_fetch_add_sub", test_u32_fetch_add_sub)
    test("atomic: u32_bitwise_and_or_xor", test_u32_bitwise_and_or_xor)
    test("atomic: u32_compare_exchange_strong_success", test_u32_compare_exchange_strong_success)
    test("atomic: u32_compare_exchange_strong_fail", test_u32_compare_exchange_strong_fail)
    test("atomic: u32_compare_exchange_weak_success", test_u32_compare_exchange_weak_success)
    test("atomic: u32_compare_exchange_weak_fail", test_u32_compare_exchange_weak_fail)
    test("atomic: u32_overflow_wrap", test_u32_overflow_wrap)

    // u16
    test("atomic: u16_load_store", test_u16_load_store)
    test("atomic: u16_exchange", test_u16_exchange)
    test("atomic: u16_fetch_add_sub", test_u16_fetch_add_sub)
    test("atomic: u16_bitwise_and_or_xor", test_u16_bitwise_and_or_xor)
    test("atomic: u16_compare_exchange_strong_success", test_u16_compare_exchange_strong_success)
    test("atomic: u16_compare_exchange_strong_fail", test_u16_compare_exchange_strong_fail)
    test("atomic: u16_compare_exchange_weak_success", test_u16_compare_exchange_weak_success)
    test("atomic: u16_compare_exchange_weak_fail", test_u16_compare_exchange_weak_fail)
    test("atomic: u16_overflow_wrap", test_u16_overflow_wrap)

    // u8
    test("atomic: u8_load_store", test_u8_load_store)
    test("atomic: u8_exchange", test_u8_exchange)
    test("atomic: u8_fetch_add_sub", test_u8_fetch_add_sub)
    test("atomic: u8_bitwise_and_or_xor", test_u8_bitwise_and_or_xor)
    test("atomic: u8_compare_exchange_strong_success", test_u8_compare_exchange_strong_success)
    test("atomic: u8_compare_exchange_strong_fail", test_u8_compare_exchange_strong_fail)
    test("atomic: u8_compare_exchange_weak_success", test_u8_compare_exchange_weak_success)
    test("atomic: u8_compare_exchange_weak_fail", test_u8_compare_exchange_weak_fail)
    test("atomic: u8_overflow_wrap", test_u8_overflow_wrap)
}
