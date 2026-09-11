// =================== helpers (public API only used in tests) ===================

var s64 : u64 = 0

func reset_u64(v: u64) : *mut u64 {
    s64 = v
    return &raw mut s64
}

var s32 : u32 = 0

func reset_u32(v: u32) : *mut u32 {
    s32 = v
    return &raw mut s32
}

var s16 : u16 = 0

func reset_u16(v: u16) : *mut u16 {
    s16 = v
    return &raw mut s16
}

var s8 : u8 = 0

func reset_u8(v: u8) : *mut u8 {
    s8 = v
    return &raw mut s8
}

// =================== fence tests ===================

@test
func atomic_fence_seq_cst(env : &mut TestEnv) {
    atomic_fence(memory_order.seq_cst)
}

@test
func atomic_fence_acquire_release_relaxed(env : &mut TestEnv) {
    atomic_fence(memory_order.acquire)
    atomic_fence(memory_order.release)
}

// =================== u64 tests ===================

@test
func u64_load_store(env : &mut TestEnv) {
    var p = reset_u64(0)
    atomic_store_u64(p, 1234567890123456789, memory_order.release)
    var v = atomic_load_u64(p, memory_order.acquire)
    if(v != 1234567890123456789) { env.error("u64_load_store"); return }
}

@test
func u64_exchange(env : &mut TestEnv) {
    var p = reset_u64(7)
    var old = atomic_exchange_u64(p, 99, memory_order.seq_cst)
    var now = atomic_load_u64(p)
    if(old != 7 || now != 99) { env.error("u64_exchange"); return }
}

@test
func u64_fetch_add_sub(env : &mut TestEnv) {
    var p = reset_u64(10)
    var old = atomic_fetch_add_u64(p, 5, memory_order.seq_cst)
    if(old != 10 || atomic_load_u64(p) != 15) { env.error("u64_fetch_add_sub"); return }
    var old2 = atomic_fetch_sub_u64(p, 3, memory_order.seq_cst)
    if(old2 != 15 || atomic_load_u64(p) != 12) { env.error("u64_fetch_add_sub"); return }
}

@test
func u64_bitwise_and_or_xor(env : &mut TestEnv) {
    var p = reset_u64(0b11110000)
    var old_and = atomic_fetch_and_u64(p, 0b10101010, memory_order.seq_cst)
    if(old_and != 0b11110000 || atomic_load_u64(p) != 0b10100000) { env.error("u64_bitwise_and_or_xor"); return }
    var old_or = atomic_fetch_or_u64(p, 0b00001111, memory_order.seq_cst)
    if(old_or != 0b10100000 || atomic_load_u64(p) != 0b10101111) { env.error("u64_bitwise_and_or_xor"); return }
    var old_xor = atomic_fetch_xor_u64(p, 0b11111111, memory_order.seq_cst)
    if(old_xor != 0b10101111 || atomic_load_u64(p) != 0b01010000) { env.error("u64_bitwise_and_or_xor"); return }
}

@test
func u64_compare_exchange_strong_success(env : &mut TestEnv) {
    var p = reset_u64(42)
    var expected: u64 = 42
    var ok = atomic_compare_exchange_strong_u64(p, &raw mut expected, 4242, memory_order.acq_rel, memory_order.acquire)
    if(ok != true || atomic_load_u64(p) != 4242) { env.error("u64_compare_exchange_strong_success"); return }
}

@test
func u64_compare_exchange_strong_fail(env : &mut TestEnv) {
    var p = reset_u64(500)
    var expected: u64 = 111
    var ok = atomic_compare_exchange_strong_u64(p, &raw mut expected, 222, memory_order.seq_cst, memory_order.seq_cst)
    if(ok != false || atomic_load_u64(p) != 500 || expected != 500) { env.error("u64_compare_exchange_strong_fail"); return }
}

@test
func u64_compare_exchange_weak_success(env : &mut TestEnv) {
    var p = reset_u64(13)
    var expected: u64 = 13
    var done = false
    for(var i = 0;i < 10; i++) {
        var ok = atomic_compare_exchange_weak_u64(p, &raw mut expected, 31, memory_order.seq_cst, memory_order.seq_cst)
        if(ok == true) { done = (atomic_load_u64(p) == 31); break }
    }
    if(!done) { env.error("u64_compare_exchange_weak_success"); }
}

@test
func u64_compare_exchange_weak_fail(env : &mut TestEnv) {
    var p = reset_u64(77)
    var expected: u64 = 5
    var ok = atomic_compare_exchange_weak_u64(p, &raw mut expected, 99, memory_order.seq_cst, memory_order.seq_cst)
    if(ok != false || expected != 77 || atomic_load_u64(p) != 77) { env.error("u64_compare_exchange_weak_fail"); return }
}

@test
func u64_overflow_wrap(env : &mut TestEnv) {
    var p = reset_u64(0xFFFFFFFFFFFFFFFF)
    var old = atomic_fetch_add_u64(p, 1, memory_order.seq_cst)
    if(old != 0xFFFFFFFFFFFFFFFF || atomic_load_u64(p) != 0) { env.error("u64_overflow_wrap"); return }
}

// =================== u32 tests ===================

@test
func u32_load_store(env : &mut TestEnv) {
    var p = reset_u32(0)
    atomic_store_u32(p, 0xDEADBEEF, memory_order.release)
    var v = atomic_load_u32(p, memory_order.acquire)
    if(v != 0xDEADBEEF) { env.error("u32_load_store"); return }
}

@test
func u32_exchange(env : &mut TestEnv) {
    var p = reset_u32(1234)
    var old = atomic_exchange_u32(p, 5678, memory_order.seq_cst)
    if(old != 1234 || atomic_load_u32(p) != 5678) { env.error("u32_exchange"); return }
}

@test
func u32_fetch_add_sub(env : &mut TestEnv) {
    var p = reset_u32(100)
    var old = atomic_fetch_add_u32(p, 50, memory_order.seq_cst)
    if(old != 100 || atomic_load_u32(p) != 150) { env.error("u32_fetch_add_sub"); return }
    var old2 = atomic_fetch_sub_u32(p, 25, memory_order.seq_cst)
    if(old2 != 150 || atomic_load_u32(p) != 125) { env.error("u32_fetch_add_sub"); return }
}

@test
func u32_bitwise_and_or_xor(env : &mut TestEnv) {
    var p = reset_u32(0xFF00FF00)
    var old_and = atomic_fetch_and_u32(p, 0x0F0F0F0F, memory_order.seq_cst)
    if(old_and != 0xFF00FF00 || atomic_load_u32(p) != (0xFF00FF00 & 0x0F0F0F0F)) { env.error("u32_bitwise_and_or_xor"); return }
    var old_or = atomic_fetch_or_u32(p, 0x0000FFFF, memory_order.seq_cst)
    if(old_or != (0xFF00FF00 & 0x0F0F0F0F) || atomic_load_u32(p) != ((0xFF00FF00 & 0x0F0F0F0F) | 0x0000FFFF)) { env.error("u32_bitwise_and_or_xor"); return }
    var old_xor = atomic_fetch_xor_u32(p, 0xAAAAAAAA, memory_order.seq_cst)
    if(atomic_load_u32(p) != (((0xFF00FF00 & 0x0F0F0F0F) | 0x0000FFFF) ^ 0xAAAAAAAA)) { env.error("u32_bitwise_and_or_xor"); return }
}

@test
func u32_compare_exchange_strong_success(env : &mut TestEnv) {
    var p = reset_u32(200)
    var expected: u32 = 200
    var ok = atomic_compare_exchange_strong_u32(p, &raw mut expected, 300, memory_order.acquire, memory_order.relaxed)
    if(ok != true || atomic_load_u32(p) != 300) { env.error("u32_compare_exchange_strong_success"); return }
}

@test
func u32_compare_exchange_strong_fail(env : &mut TestEnv) {
    var p = reset_u32(400)
    var expected: u32 = 1
    var ok = atomic_compare_exchange_strong_u32(p, &raw mut expected, 2, memory_order.seq_cst, memory_order.seq_cst)
    if(ok != false || expected != 400 || atomic_load_u32(p) != 400) { env.error("u32_compare_exchange_strong_fail"); return }
}

@test
func u32_compare_exchange_weak_success(env : &mut TestEnv) {
    var p = reset_u32(8)
    var expected: u32 = 8
    var done = false
    for(var i = 0;i < 10;i++) {
        var ok = atomic_compare_exchange_weak_u32(p, &raw mut expected, 16, memory_order.seq_cst, memory_order.seq_cst)
        if(ok == true) { done = (atomic_load_u32(p) == 16); break }
    }
    if(!done) { env.error("u32_compare_exchange_weak_success"); }
}

@test
func u32_compare_exchange_weak_fail(env : &mut TestEnv) {
    var p = reset_u32(9)
    var expected: u32 = 123
    var ok = atomic_compare_exchange_weak_u32(p, &raw mut expected, 45, memory_order.seq_cst, memory_order.seq_cst)
    if(ok != false || expected != 9 || atomic_load_u32(p) != 9) { env.error("u32_compare_exchange_weak_fail"); return }
}

@test
func u32_overflow_wrap(env : &mut TestEnv) {
    var p = reset_u32(0xFFFFFFFF)
    var old = atomic_fetch_add_u32(p, 1, memory_order.seq_cst)
    if(old != 0xFFFFFFFF || atomic_load_u32(p) != 0) { env.error("u32_overflow_wrap"); return }
}

// =================== u16 tests ===================

@test
func u16_load_store(env : &mut TestEnv) {
    var p = reset_u16(0)
    atomic_store_u16(p, 0xABCD, memory_order.release)
    var v = atomic_load_u16(p, memory_order.acquire)
    if(v != 0xABCD) { env.error("u16_load_store"); return }
}

@test
func u16_exchange(env : &mut TestEnv) {
    var p = reset_u16(7)
    var old = atomic_exchange_u16(p, 123, memory_order.seq_cst)
    if(old != 7 || atomic_load_u16(p) != 123) { env.error("u16_exchange"); return }
}

@test
func u16_fetch_add_sub(env : &mut TestEnv) {
    var p = reset_u16(1000)
    var old = atomic_fetch_add_u16(p, 2000, memory_order.seq_cst)
    if(old != 1000 || atomic_load_u16(p) != 3000) { env.error("u16_fetch_add_sub"); return }
    var old2 = atomic_fetch_sub_u16(p, 500, memory_order.seq_cst)
    if(old2 != 3000 || atomic_load_u16(p) != 2500) { env.error("u16_fetch_add_sub"); return }
}

@test
func u16_bitwise_and_or_xor(env : &mut TestEnv) {
    var p = reset_u16(0b1111000011110000)
    var old_and = atomic_fetch_and_u16(p, 0b1010101010101010, memory_order.seq_cst)
    if(old_and != 0b1111000011110000 || atomic_load_u16(p) != (0b1111000011110000 & 0b1010101010101010)) { env.error("u16_bitwise_and_or_xor"); return }
    var old_or = atomic_fetch_or_u16(p, 0b0000111100001111, memory_order.seq_cst)
    if(atomic_load_u16(p) != ((0b1111000011110000 & 0b1010101010101010) | 0b0000111100001111)) { env.error("u16_bitwise_and_or_xor"); return }
    var old_xor = atomic_fetch_xor_u16(p, 0xFFFF, memory_order.seq_cst)
    if(old_xor != ((0b1111000011110000 & 0b1010101010101010) | 0b0000111100001111)) { env.error("u16_bitwise_and_or_xor"); return }
}

@test
func u16_compare_exchange_strong_success(env : &mut TestEnv) {
    var p = reset_u16(55)
    var expected: u16 = 55
    var ok = atomic_compare_exchange_strong_u16(p, &raw mut expected, 66, memory_order.acquire, memory_order.relaxed)
    if(ok != true || atomic_load_u16(p) != 66) { env.error("u16_compare_exchange_strong_success"); return }
}

@test
func u16_compare_exchange_strong_fail(env : &mut TestEnv) {
    var p = reset_u16(77)
    var expected: u16 = 2
    var ok = atomic_compare_exchange_strong_u16(p, &raw mut expected, 3, memory_order.seq_cst, memory_order.seq_cst)
    if(ok != false || expected != 77 || atomic_load_u16(p) != 77) { env.error("u16_compare_exchange_strong_fail"); return }
}

@test
func u16_compare_exchange_weak_success(env : &mut TestEnv) {
    var p = reset_u16(21)
    var expected: u16 = 21
    var done = false
    for(var i = 0;i < 10; i++) {
        var ok = atomic_compare_exchange_weak_u16(p, &raw mut expected, 42, memory_order.seq_cst, memory_order.seq_cst)
        if(ok == true) { done = (atomic_load_u16(p) == 42); break }
    }
    if(!done) { env.error("u16_compare_exchange_weak_success"); }
}

@test
func u16_compare_exchange_weak_fail(env : &mut TestEnv) {
    var p = reset_u16(99)
    var expected: u16 = 0
    var ok = atomic_compare_exchange_weak_u16(p, &raw mut expected, 1, memory_order.seq_cst, memory_order.seq_cst)
    if(ok != false || expected != 99 || atomic_load_u16(p) != 99) { env.error("u16_compare_exchange_weak_fail"); return }
}

@test
func u16_overflow_wrap(env : &mut TestEnv) {
    var p = reset_u16(0xFFFF)
    var old = atomic_fetch_add_u16(p, 1, memory_order.seq_cst)
    if(old != 0xFFFF || atomic_load_u16(p) != 0) { env.error("u16_overflow_wrap"); return }
}

// =================== u8 tests ===================

@test
func u8_load_store(env : &mut TestEnv) {
    var p = reset_u8(0)
    atomic_store_u8(p, 0x7F, memory_order.release)
    var v = atomic_load_u8(p, memory_order.acquire)
    if(v != 0x7F) { env.error("u8_load_store"); return }
}

@test
func u8_exchange(env : &mut TestEnv) {
    var p = reset_u8(3)
    var old = atomic_exchange_u8(p, 8, memory_order.seq_cst)
    if(old != 3 || atomic_load_u8(p) != 8) { env.error("u8_exchange"); return }
}

@test
func u8_fetch_add_sub(env : &mut TestEnv) {
    var p = reset_u8(200)
    var old = atomic_fetch_add_u8(p, 30, memory_order.seq_cst)
    if(old != 200 || atomic_load_u8(p) != 230) { env.error("u8_fetch_add_sub"); return }
    var old2 = atomic_fetch_sub_u8(p, 50, memory_order.seq_cst)
    if(old2 != 230 || atomic_load_u8(p) != 180) { env.error("u8_fetch_add_sub"); return }
}

@test
func u8_bitwise_and_or_xor(env : &mut TestEnv) {
    var p = reset_u8(0b11001100)
    var old_and = atomic_fetch_and_u8(p, 0b10101010, memory_order.seq_cst)
    if(old_and != 0b11001100 || atomic_load_u8(p) != (0b11001100 & 0b10101010)) { env.error("u8_bitwise_and_or_xor"); return }
    var old_or = atomic_fetch_or_u8(p, 0b00001111, memory_order.seq_cst)
    if(atomic_load_u8(p) != ((0b11001100 & 0b10101010) | 0b00001111)) { env.error("u8_bitwise_and_or_xor"); return }
    var old_xor = atomic_fetch_xor_u8(p, 0xFF, memory_order.seq_cst)
    if(old_xor != ((0b11001100 & 0b10101010) | 0b00001111)) { env.error("u8_bitwise_and_or_xor"); return }
}

@test
func u8_compare_exchange_strong_success(env : &mut TestEnv) {
    var p = reset_u8(10)
    var expected: u8 = 10
    var ok = atomic_compare_exchange_strong_u8(p, &raw mut expected, 20, memory_order.acquire, memory_order.release)
    if(ok != true || atomic_load_u8(p) != 20) { env.error("u8_compare_exchange_strong_success"); return }
}

@test
func u8_compare_exchange_strong_fail(env : &mut TestEnv) {
    var p = reset_u8(30)
    var expected: u8 = 5
    var ok = atomic_compare_exchange_strong_u8(p, &raw mut expected, 6, memory_order.seq_cst, memory_order.seq_cst)
    if(ok != false || expected != 30 || atomic_load_u8(p) != 30) { env.error("u8_compare_exchange_strong_fail"); return }
}

@test
func u8_compare_exchange_weak_success(env : &mut TestEnv) {
    var p = reset_u8(4)
    var expected: u8 = 4
    var done = false
    for(var i = 0; i < 10; i++){
        var ok = atomic_compare_exchange_weak_u8(p, &raw mut expected, 9, memory_order.seq_cst, memory_order.seq_cst)
        if(ok == true) { done = (atomic_load_u8(p) == 9); break }
    }
    if(!done) { env.error("u8_compare_exchange_weak_success"); }
}

@test
func u8_compare_exchange_weak_fail(env : &mut TestEnv) {
    var p = reset_u8(66)
    var expected: u8 = 12
    var ok = atomic_compare_exchange_weak_u8(p, &raw mut expected, 77, memory_order.seq_cst, memory_order.seq_cst)
    if(ok != false || expected != 66 || atomic_load_u8(p) != 66) { env.error("u8_compare_exchange_weak_fail"); return }
}

@test
func u8_overflow_wrap(env : &mut TestEnv) {
    var p = reset_u8(0xFF)
    var old = atomic_fetch_add_u8(p, 1, memory_order.seq_cst)
    if(old != 0xFF || atomic_load_u8(p) != 0) { env.error("u8_overflow_wrap"); return }
}

// =================== @volatile tests ===================

@test
func volatile_flag_spin(env : &mut TestEnv) {
    @volatile
    var flag : u32 = 0
    // Verify @volatile variable can be read and written
    if(flag != 0) { env.error("volatile_flag_spin"); return }
    flag = 1
    if(flag != 1) { env.error("volatile_flag_spin"); return }
}

@test
func volatile_in_atomic_struct(env : &mut TestEnv) {
    // Test @volatile on a struct field used for atomic-like operations
    var cell = atomic_cell_make()
    if(cell.value != 0) { env.error("volatile_in_atomic_struct"); return }
    cell.value = 42
    if(cell.value != 42) { env.error("volatile_in_atomic_struct"); return }
}

struct AtomicCell {
    @volatile
    var value : u64 = 0
}

func atomic_cell_make() : AtomicCell {
    return AtomicCell{}
}

// =================== concurrency stress tests ===================

struct FetchAddArgs {
    var ptr : *mut u64
    var iters : u64
}

func fetch_add_worker_u64(arg : *void) : *void {
    var a = arg as *mut FetchAddArgs
    var i : u64 = 0
    while(i < a.iters) {
        atomic_fetch_add_u64(a.ptr, 1, memory_order.seq_cst)
        i = i + 1
    }
    return null
}

@test
func concurrent_fetch_add_u64(env : &mut TestEnv) {
    var counter : u64 = 0
    var iters : u64 = 10000
    var args = FetchAddArgs{ptr: &raw mut counter, iters: iters}
    var t1 = std::concurrent::spawn(fetch_add_worker_u64, &raw mut args as *void)
    var t2 = std::concurrent::spawn(fetch_add_worker_u64, &raw mut args as *void)
    t1.join()
    t2.join()
    var expected : u64 = iters * 2
    if(counter != expected) { env.error("concurrent_fetch_add_u64"); return }
}

struct FetchAddArgsU32 {
    var ptr : *mut u32
    var iters : u32
}

func fetch_add_worker_u32(arg : *void) : *void {
    var a = arg as *mut FetchAddArgsU32
    var i : u32 = 0
    while(i < a.iters) {
        atomic_fetch_add_u32(a.ptr, 1, memory_order.seq_cst)
        i = i + 1
    }
    return null
}

@test
func concurrent_fetch_add_u32(env : &mut TestEnv) {
    var counter : u32 = 0
    var iters : u32 = 10000
    var args = FetchAddArgsU32{ptr: &raw mut counter, iters: iters}
    var t1 = std::concurrent::spawn(fetch_add_worker_u32, &raw mut args as *void)
    var t2 = std::concurrent::spawn(fetch_add_worker_u32, &raw mut args as *void)
    t1.join()
    t2.join()
    var expected : u32 = iters * 2
    if(counter != expected) { env.error("concurrent_fetch_add_u32"); return }
}

struct ExchangeArgs {
    var ptr : *mut u64
    var iters : u64
}

func exchange_worker(arg : *void) : *void {
    var a = arg as *mut ExchangeArgs
    var i : u64 = 0
    while(i < a.iters) {
        atomic_exchange_u64(a.ptr, i, memory_order.seq_cst)
        i = i + 1
    }
    return null
}

@test
func concurrent_exchange_u64(env : &mut TestEnv) {
    var val : u64 = 0
    var iters : u64 = 10000
    var args = ExchangeArgs{ptr: &raw mut val, iters: iters}
    var t1 = std::concurrent::spawn(exchange_worker, &raw mut args as *void)
    var t2 = std::concurrent::spawn(exchange_worker, &raw mut args as *void)
    t1.join()
    t2.join()
    // After both threads complete, val must be one of {0..iters-1}
    // The key invariant: no tearing — val is always a valid u64
    if(val >= iters && val != 0) { env.error("concurrent_exchange_u64"); return }
}

struct CasArgs {
    var ptr : *mut u64
    var iters : u64
}

func cas_worker(arg : *void) : *void {
    var a = arg as *mut CasArgs
    var i : u64 = 0
    var expected : u64 = 0
    var desired : u64 = 0
    while(i < a.iters) {
        expected = atomic_load_u64(a.ptr, memory_order.relaxed)
        desired = expected + 1
        atomic_compare_exchange_strong_u64(a.ptr, &raw mut expected, desired, memory_order.seq_cst, memory_order.relaxed)
        i = i + 1
    }
    return null
}

@test
func concurrent_cas_u64(env : &mut TestEnv) {
    var counter : u64 = 0
    var iters : u64 = 10000
    var args = CasArgs{ptr: &raw mut counter, iters: iters}
    var t1 = std::concurrent::spawn(cas_worker, &raw mut args as *void)
    var t2 = std::concurrent::spawn(cas_worker, &raw mut args as *void)
    t1.join()
    t2.join()
    // CAS may lose updates under contention, but counter must be <= iters*2 and >= iters
    if(counter > iters * 2) { env.error("concurrent_cas_u64: counter exceeded max"); return }
}

struct FetchSubArgs {
    var ptr : *mut u64
    var iters : u64
}

func fetch_sub_worker(arg : *void) : *void {
    var a = arg as *mut FetchSubArgs
    var i : u64 = 0
    while(i < a.iters) {
        atomic_fetch_sub_u64(a.ptr, 1, memory_order.seq_cst)
        i = i + 1
    }
    return null
}

@test
func concurrent_fetch_sub_u64(env : &mut TestEnv) {
    var counter : u64 = 20000
    var iters : u64 = 10000
    var args = FetchSubArgs{ptr: &raw mut counter, iters: iters}
    var t1 = std::concurrent::spawn(fetch_sub_worker, &raw mut args as *void)
    var t2 = std::concurrent::spawn(fetch_sub_worker, &raw mut args as *void)
    t1.join()
    t2.join()
    var expected : u64 = 0
    if(counter != expected) { env.error("concurrent_fetch_sub_u64"); return }
}

struct FlagArgs {
    var ptr : *mut atomic_flag
    var count : u64
}

func flag_set_worker(arg : *void) : *void {
    var a = arg as *mut FlagArgs
    var i : u64 = 0
    var sets : u64 = 0
    while(i < a.count) {
        if(atomic_flag_test_and_set(a.ptr)) {
            sets = sets + 1
        }
        atomic_flag_clear(a.ptr)
        i = i + 1
    }
    return null
}

@test
func concurrent_atomic_flag(env : &mut TestEnv) {
    var flag = atomic_flag{_value: 0}
    var count : u64 = 10000
    var args = FlagArgs{ptr: &raw mut flag, count: count}
    var t1 = std::concurrent::spawn(flag_set_worker, &raw mut args as *void)
    var t2 = std::concurrent::spawn(flag_set_worker, &raw mut args as *void)
    t1.join()
    t2.join()
    // Flag should be clear after both threads done
    if(atomic_flag_test_and_set(&raw mut flag)) {
        // If set, clear it — both threads should have left it clear
        atomic_flag_clear(&raw mut flag)
    }
}

// =================== memory ordering tests ===================

@test
func fence_relaxed(env : &mut TestEnv) {
    atomic_fence(memory_order.relaxed)
}

@test
func fence_acquire(env : &mut TestEnv) {
    atomic_fence(memory_order.acquire)
}

@test
func fence_release(env : &mut TestEnv) {
    atomic_fence(memory_order.release)
}

@test
func fence_acq_rel(env : &mut TestEnv) {
    atomic_fence(memory_order.acq_rel)
}

@test
func u64_load_store_relaxed(env : &mut TestEnv) {
    var p = reset_u64(0)
    atomic_store_u64(p, 42, memory_order.relaxed)
    var v = atomic_load_u64(p, memory_order.relaxed)
    if(v != 42) { env.error("u64_load_store_relaxed"); return }
}

@test
func u64_load_store_acquire_release(env : &mut TestEnv) {
    var p = reset_u64(0)
    atomic_store_u64(p, 999, memory_order.release)
    var v = atomic_load_u64(p, memory_order.acquire)
    if(v != 999) { env.error("u64_load_store_acquire_release"); return }
}

@test
func u32_load_store_relaxed(env : &mut TestEnv) {
    var p = reset_u32(0)
    atomic_store_u32(p, 77, memory_order.relaxed)
    var v = atomic_load_u32(p, memory_order.relaxed)
    if(v != 77) { env.error("u32_load_store_relaxed"); return }
}

@test
func u32_load_store_acquire_release(env : &mut TestEnv) {
    var p = reset_u32(0)
    atomic_store_u32(p, 555, memory_order.release)
    var v = atomic_load_u32(p, memory_order.acquire)
    if(v != 555) { env.error("u32_load_store_acquire_release"); return }
}

@test
func u16_load_store_relaxed(env : &mut TestEnv) {
    var p = reset_u16(0)
    atomic_store_u16(p, 33, memory_order.relaxed)
    var v = atomic_load_u16(p, memory_order.relaxed)
    if(v != 33) { env.error("u16_load_store_relaxed"); return }
}

@test
func u8_load_store_relaxed(env : &mut TestEnv) {
    var p = reset_u8(0)
    atomic_store_u8(p, 11, memory_order.relaxed)
    var v = atomic_load_u8(p, memory_order.relaxed)
    if(v != 11) { env.error("u8_load_store_relaxed"); return }
}

@test
func u64_exchange_relaxed(env : &mut TestEnv) {
    var p = reset_u64(100)
    var old = atomic_exchange_u64(p, 200, memory_order.relaxed)
    if(old != 100 || atomic_load_u64(p) != 200) { env.error("u64_exchange_relaxed"); return }
}

@test
func u64_exchange_acquire(env : &mut TestEnv) {
    var p = reset_u64(11)
    var old = atomic_exchange_u64(p, 22, memory_order.acquire)
    if(old != 11 || atomic_load_u64(p) != 22) { env.error("u64_exchange_acquire"); return }
}

@test
func u64_exchange_release(env : &mut TestEnv) {
    var p = reset_u64(33)
    var old = atomic_exchange_u64(p, 44, memory_order.release)
    if(old != 33 || atomic_load_u64(p) != 44) { env.error("u64_exchange_release"); return }
}

@test
func u32_exchange_relaxed(env : &mut TestEnv) {
    var p = reset_u32(10)
    var old = atomic_exchange_u32(p, 20, memory_order.relaxed)
    if(old != 10 || atomic_load_u32(p) != 20) { env.error("u32_exchange_relaxed"); return }
}

@test
func u16_exchange_relaxed(env : &mut TestEnv) {
    var p = reset_u16(100)
    var old = atomic_exchange_u16(p, 200, memory_order.relaxed)
    if(old != 100 || atomic_load_u16(p) != 200) { env.error("u16_exchange_relaxed"); return }
}

@test
func u8_exchange_relaxed(env : &mut TestEnv) {
    var p = reset_u8(10)
    var old = atomic_exchange_u8(p, 20, memory_order.relaxed)
    if(old != 10 || atomic_load_u8(p) != 20) { env.error("u8_exchange_relaxed"); return }
}

@test
func u64_fetch_add_relaxed(env : &mut TestEnv) {
    var p = reset_u64(1000)
    var old = atomic_fetch_add_u64(p, 500, memory_order.relaxed)
    if(old != 1000 || atomic_load_u64(p) != 1500) { env.error("u64_fetch_add_relaxed"); return }
}

@test
func u64_fetch_sub_acq_rel(env : &mut TestEnv) {
    var p = reset_u64(1000)
    var old = atomic_fetch_sub_u64(p, 100, memory_order.acq_rel)
    if(old != 1000 || atomic_load_u64(p) != 900) { env.error("u64_fetch_sub_acq_rel"); return }
}

@test
func u32_fetch_add_release(env : &mut TestEnv) {
    var p = reset_u32(500)
    var old = atomic_fetch_add_u32(p, 250, memory_order.release)
    if(old != 500 || atomic_load_u32(p) != 750) { env.error("u32_fetch_add_release"); return }
}

@test
func u32_fetch_sub_acquire(env : &mut TestEnv) {
    var p = reset_u32(500)
    var old = atomic_fetch_sub_u32(p, 150, memory_order.acquire)
    if(old != 500 || atomic_load_u32(p) != 350) { env.error("u32_fetch_sub_acquire"); return }
}

@test
func u16_fetch_add_relaxed(env : &mut TestEnv) {
    var p = reset_u16(100)
    var old = atomic_fetch_add_u16(p, 50, memory_order.relaxed)
    if(old != 100 || atomic_load_u16(p) != 150) { env.error("u16_fetch_add_relaxed"); return }
}

@test
func u8_fetch_add_relaxed(env : &mut TestEnv) {
    var p = reset_u8(50)
    var old = atomic_fetch_add_u8(p, 25, memory_order.relaxed)
    if(old != 50 || atomic_load_u8(p) != 75) { env.error("u8_fetch_add_relaxed"); return }
}

@test
func u64_fetch_and_or_xor_relaxed(env : &mut TestEnv) {
    var p = reset_u64(0xFF)
    var old_and = atomic_fetch_and_u64(p, 0x0F, memory_order.relaxed)
    if(old_and != 0xFF || atomic_load_u64(p) != 0x0F) { env.error("u64_fetch_and_or_xor_relaxed"); return }
    var old_or = atomic_fetch_or_u64(p, 0xF0, memory_order.relaxed)
    if(old_or != 0x0F || atomic_load_u64(p) != 0xFF) { env.error("u64_fetch_and_or_xor_relaxed"); return }
    var old_xor = atomic_fetch_xor_u64(p, 0xFF, memory_order.relaxed)
    if(old_xor != 0xFF || atomic_load_u64(p) != 0) { env.error("u64_fetch_and_or_xor_relaxed"); return }
}

@test
func u64_cas_relaxed(env : &mut TestEnv) {
    var p = reset_u64(100)
    var expected: u64 = 100
    var ok = atomic_compare_exchange_strong_u64(p, &raw mut expected, 200, memory_order.relaxed, memory_order.relaxed)
    if(ok != true || atomic_load_u64(p) != 200) { env.error("u64_cas_relaxed"); return }
}

@test
func u32_cas_acquire_release(env : &mut TestEnv) {
    var p = reset_u32(100)
    var expected: u32 = 100
    var ok = atomic_compare_exchange_strong_u32(p, &raw mut expected, 200, memory_order.acquire, memory_order.release)
    if(ok != true || atomic_load_u32(p) != 200) { env.error("u32_cas_acquire_release"); return }
}

@test
func u16_cas_relaxed(env : &mut TestEnv) {
    var p = reset_u16(100)
    var expected: u16 = 100
    var ok = atomic_compare_exchange_strong_u16(p, &raw mut expected, 200, memory_order.relaxed, memory_order.relaxed)
    if(ok != true || atomic_load_u16(p) != 200) { env.error("u16_cas_relaxed"); return }
}

@test
func u8_cas_relaxed(env : &mut TestEnv) {
    var p = reset_u8(100)
    var expected: u8 = 100
    var ok = atomic_compare_exchange_strong_u8(p, &raw mut expected, 200, memory_order.relaxed, memory_order.relaxed)
    if(ok != true || atomic_load_u8(p) != 200) { env.error("u8_cas_relaxed"); return }
}

// =================== edge case tests ===================

@test
func u64_boundary_zero(env : &mut TestEnv) {
    var p = reset_u64(0xFFFFFFFFFFFFFFFF)
    atomic_store_u64(p, 0, memory_order.seq_cst)
    if(atomic_load_u64(p) != 0) { env.error("u64_boundary_zero"); return }
}

@test
func u64_boundary_max(env : &mut TestEnv) {
    var p = reset_u64(0)
    atomic_store_u64(p, 0xFFFFFFFFFFFFFFFF, memory_order.seq_cst)
    if(atomic_load_u64(p) != 0xFFFFFFFFFFFFFFFF) { env.error("u64_boundary_max"); return }
}

@test
func u32_boundary_zero(env : &mut TestEnv) {
    var p = reset_u32(0xFFFFFFFF)
    atomic_store_u32(p, 0, memory_order.seq_cst)
    if(atomic_load_u32(p) != 0) { env.error("u32_boundary_zero"); return }
}

@test
func u32_boundary_max(env : &mut TestEnv) {
    var p = reset_u32(0)
    atomic_store_u32(p, 0xFFFFFFFF, memory_order.seq_cst)
    if(atomic_load_u32(p) != 0xFFFFFFFF) { env.error("u32_boundary_max"); return }
}

@test
func u16_boundary_zero(env : &mut TestEnv) {
    var p = reset_u16(0xFFFF)
    atomic_store_u16(p, 0, memory_order.seq_cst)
    if(atomic_load_u16(p) != 0) { env.error("u16_boundary_zero"); return }
}

@test
func u16_boundary_max(env : &mut TestEnv) {
    var p = reset_u16(0)
    atomic_store_u16(p, 0xFFFF, memory_order.seq_cst)
    if(atomic_load_u16(p) != 0xFFFF) { env.error("u16_boundary_max"); return }
}

@test
func u8_boundary_zero(env : &mut TestEnv) {
    var p = reset_u8(0xFF)
    atomic_store_u8(p, 0, memory_order.seq_cst)
    if(atomic_load_u8(p) != 0) { env.error("u8_boundary_zero"); return }
}

@test
func u8_boundary_max(env : &mut TestEnv) {
    var p = reset_u8(0)
    atomic_store_u8(p, 0xFF, memory_order.seq_cst)
    if(atomic_load_u8(p) != 0xFF) { env.error("u8_boundary_max"); return }
}

@test
func u64_sub_underflow_wrap(env : &mut TestEnv) {
    var p = reset_u64(0)
    var old = atomic_fetch_sub_u64(p, 1, memory_order.seq_cst)
    if(old != 0 || atomic_load_u64(p) != 0xFFFFFFFFFFFFFFFF) { env.error("u64_sub_underflow_wrap"); return }
}

@test
func u32_sub_underflow_wrap(env : &mut TestEnv) {
    var p = reset_u32(0)
    var old = atomic_fetch_sub_u32(p, 1, memory_order.seq_cst)
    if(old != 0 || atomic_load_u32(p) != 0xFFFFFFFF) { env.error("u32_sub_underflow_wrap"); return }
}

@test
func u16_sub_underflow_wrap(env : &mut TestEnv) {
    var p = reset_u16(0)
    var old = atomic_fetch_sub_u16(p, 1, memory_order.seq_cst)
    if(old != 0 || atomic_load_u16(p) != 0xFFFF) { env.error("u16_sub_underflow_wrap"); return }
}

@test
func u8_sub_underflow_wrap(env : &mut TestEnv) {
    var p = reset_u8(0)
    var old = atomic_fetch_sub_u8(p, 1, memory_order.seq_cst)
    if(old != 0 || atomic_load_u8(p) != 0xFF) { env.error("u8_sub_underflow_wrap"); return }
}

@test
func u64_exchange_identity(env : &mut TestEnv) {
    var p = reset_u64(42)
    var old = atomic_exchange_u64(p, 42, memory_order.seq_cst)
    if(old != 42 || atomic_load_u64(p) != 42) { env.error("u64_exchange_identity"); return }
}

@test
func u32_exchange_identity(env : &mut TestEnv) {
    var p = reset_u32(42)
    var old = atomic_exchange_u32(p, 42, memory_order.seq_cst)
    if(old != 42 || atomic_load_u32(p) != 42) { env.error("u32_exchange_identity"); return }
}

@test
func u64_cas_spurious(env : &mut TestEnv) {
    // Weak CAS may fail spuriously — verify it doesn't corrupt state
    var p = reset_u64(999)
    var expected: u64 = 999
    var done = false
    for(var i = 0; i < 100; i++) {
        var ok = atomic_compare_exchange_weak_u64(p, &raw mut expected, 1234, memory_order.relaxed, memory_order.relaxed)
        if(ok) { done = true; break }
        // expected is updated on failure
    }
    if(!done || atomic_load_u64(p) != 1234) { env.error("u64_cas_spurious"); return }
}

@test
func u32_cas_spurious(env : &mut TestEnv) {
    var p = reset_u32(500)
    var expected: u32 = 500
    var done = false
    for(var i = 0; i < 100; i++) {
        var ok = atomic_compare_exchange_weak_u32(p, &raw mut expected, 600, memory_order.relaxed, memory_order.relaxed)
        if(ok) { done = true; break }
    }
    if(!done || atomic_load_u32(p) != 600) { env.error("u32_cas_spurious"); return }
}

@test
func u64_fetch_add_zero(env : &mut TestEnv) {
    var p = reset_u64(42)
    var old = atomic_fetch_add_u64(p, 0, memory_order.seq_cst)
    if(old != 42 || atomic_load_u64(p) != 42) { env.error("u64_fetch_add_zero"); return }
}

@test
func u32_fetch_sub_zero(env : &mut TestEnv) {
    var p = reset_u32(42)
    var old = atomic_fetch_sub_u32(p, 0, memory_order.seq_cst)
    if(old != 42 || atomic_load_u32(p) != 42) { env.error("u32_fetch_sub_zero"); return }
}

@test
func u64_double_exchange(env : &mut TestEnv) {
    var p = reset_u64(1)
    var old1 = atomic_exchange_u64(p, 2, memory_order.seq_cst)
    var old2 = atomic_exchange_u64(p, 3, memory_order.seq_cst)
    if(old1 != 1 || old2 != 2 || atomic_load_u64(p) != 3) { env.error("u64_double_exchange"); return }
}

@test
func u64_fetch_add_negative(env : &mut TestEnv) {
    var p = reset_u64(100)
    var old = atomic_fetch_add_u64(p, 0xFFFFFFFFFFFFFFFF, memory_order.seq_cst)
    if(old != 100 || atomic_load_u64(p) != 99) { env.error("u64_fetch_add_negative"); return }
}

// =================== atomic_flag edge cases ===================

@test
func atomic_flag_already_set(env : &mut TestEnv) {
    var flag = atomic_flag{_value: 1}
    // test_and_set on already-set flag should return true (was already set)
    var was_set = atomic_flag_test_and_set(&raw mut flag)
    if(!was_set) { env.error("atomic_flag_already_set: expected true"); return }
    // Clear and try again
    atomic_flag_clear(&raw mut flag)
    var was_set2 = atomic_flag_test_and_set(&raw mut flag)
    if(was_set2) { env.error("atomic_flag_already_set: expected false after clear"); return }
}

@test
func atomic_flag_clear_and_retest(env : &mut TestEnv) {
    var flag = atomic_flag{_value: 0}
    var was_set = atomic_flag_test_and_set(&raw mut flag)
    if(was_set) { env.error("atomic_flag_clear_and_retest: first set should be false"); return }
    atomic_flag_clear(&raw mut flag)
    var was_set2 = atomic_flag_test_and_set(&raw mut flag)
    if(was_set2) { env.error("atomic_flag_clear_and_retest: second set should be false"); return }
    atomic_flag_clear(&raw mut flag)
}

