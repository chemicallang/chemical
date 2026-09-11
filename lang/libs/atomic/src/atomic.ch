// ========================
// Enums and helper types
// ========================

public enum memory_order : int {
    relaxed,
    consume,
    acquire,
    release,
    acq_rel,
    seq_cst
};

enum llvm_atomic_memory_order : int {
    not_atomic = 0,
    unordered = 1,
    monotonic = 2,
    acquire = 4,
    release = 5,
    acquire_release = 6,
    sequentially_consistent = 7,
};

comptime func mem_order_to_int(order : llvm_atomic_memory_order) : int {
    switch(order) {
        llvm_atomic_memory_order.not_atomic => { return 0; }
        llvm_atomic_memory_order.unordered => { return 1; }
        llvm_atomic_memory_order.monotonic => { return 2; }
        llvm_atomic_memory_order.acquire => { return 4; }
        llvm_atomic_memory_order.release => { return 5; }
        llvm_atomic_memory_order.acquire_release => { return 6; }
        llvm_atomic_memory_order.sequentially_consistent, default => { return 7; }
    }
}

comptime func convert_mem_order_to_llvm(order : memory_order) : llvm_atomic_memory_order {
    switch(order) {
        memory_order.relaxed => { return llvm_atomic_memory_order.monotonic; }
        memory_order.consume => { return llvm_atomic_memory_order.acquire; }
        memory_order.acquire => { return llvm_atomic_memory_order.acquire; }
        memory_order.release => { return llvm_atomic_memory_order.release; }
        memory_order.acq_rel => { return llvm_atomic_memory_order.acquire_release; }
        memory_order.seq_cst, default => { return llvm_atomic_memory_order.sequentially_consistent; }
    }
}

comptime func llvm_mem_order(order : memory_order) : int {
    return mem_order_to_int(convert_mem_order_to_llvm(order))
}

enum llvm_atomic_sync_scope {
    system = 0,
    single_thread = 1
}

comptime func scope_to_int(scope : llvm_atomic_sync_scope) : int {
    switch(scope) {
        llvm_atomic_sync_scope.system, default => { return 0; }
        llvm_atomic_sync_scope.single_thread => { return 1; }
    }
}

enum llvm_atomic_op : int {
    Xchg = 0,
    Add = 1,
    Sub = 2,
    And = 3,
    Nand = 4,
    Or = 5,
    Xor = 6,
    Max = 7,
    Min = 8,
    UMax = 9,
    UMin = 10,
    FAdd = 11,
    FSub = 12,
    FMax = 13,
    FMin = 14,
    UIncWrap = 15,
    UDecWrap = 16,
};

comptime func llvm_atomic_op_to_int(op : llvm_atomic_op) : int {
    switch(op) {
        llvm_atomic_op.Xchg => { return 0; }
        llvm_atomic_op.Add, default => { return 1; }
        llvm_atomic_op.Sub => { return 2; }
        llvm_atomic_op.And => { return 3; }
        llvm_atomic_op.Nand => { return 4; }
        llvm_atomic_op.Or => { return 5; }
        llvm_atomic_op.Xor => { return 6; }
        llvm_atomic_op.Max => { return 7; }
        llvm_atomic_op.Min => { return 8; }
        llvm_atomic_op.UMax => { return 9; }
        llvm_atomic_op.UMin => { return 10; }
        llvm_atomic_op.FAdd => { return 11; }
        llvm_atomic_op.FSub => { return 12; }
        llvm_atomic_op.FMax => { return 13; }
        llvm_atomic_op.FMin => { return 14; }
        llvm_atomic_op.UIncWrap => { return 15; }
        llvm_atomic_op.UDecWrap => { return 16; }
    }
}

// ---- Capability dispatch wrapper ----
comptime func has_atomic_builtins() : bool {
    return intrinsics::supports("atomic")
}

// ========================
// Inline-asm CAS primitives (C backend only, x86_64)
// These are @retained regular functions with inline asm.
// They are called from comptime funcs via %runtime_value(...).
// ========================

@retained func __chx__cas_u64(ptr : *mut u64, expected : *mut u64, desired : u64) : bool {
    comptime if(def.x86_64 || def.i386) {
        var success : bool = false
        asm("lock cmpxchgq %3, %2\n\tsete %1" : "=a"(*expected), "=q"(success), "+m"(*ptr) : "r"(desired), "0"(*expected) : "cc", "memory")
        return success
    } else {
        return false
    }
}

@retained func __chx__cas_u32(ptr : *mut u32, expected : *mut u32, desired : u32) : bool {
    comptime if(def.x86_64 || def.i386) {
        var success : bool = false
        asm("lock cmpxchgl %3, %2\n\tsete %1" : "=a"(*expected), "=q"(success), "+m"(*ptr) : "r"(desired), "0"(*expected) : "cc", "memory")
        return success
    } else {
        return false
    }
}

@retained func __chx__cas_u16(ptr : *mut u16, expected : *mut u16, desired : u16) : bool {
    comptime if(def.x86_64 || def.i386) {
        var success : bool = false
        asm("lock cmpxchgw %3, %2\n\tsete %1" : "=a"(*expected), "=q"(success), "+m"(*ptr) : "r"(desired), "0"(*expected) : "cc", "memory")
        return success
    } else {
        return false
    }
}

@retained func __chx__cas_u8(ptr : *mut u8, expected : *mut u8, desired : u8) : bool {
    comptime if(def.x86_64 || def.i386) {
        var success : bool = false
        asm("lock cmpxchgb %3, %2\n\tsete %1" : "=a"(*expected), "=q"(success), "+m"(*ptr) : "r"(desired), "0"(*expected) : "cc", "memory")
        return success
    } else {
        return false
    }
}

// ========================
// CAS-loop helper functions
// ========================

@retained func __chx__exchange_u64(ptr : *mut u64, val : u64) : u64 {
    var old_val : u64 = 0
    asm("xchgq %0, %1" : "=r"(old_val), "+m"(*ptr) : "0"(val) : "memory")
    return old_val
}

@retained func __chx__fetch_add_u64(ptr : *mut u64, val : u64) : u64 {
    var old_val : u64 = 0
    asm("lock xaddq %0, %1" : "=r"(old_val), "+m"(*ptr) : "0"(val) : "cc", "memory")
    return old_val
}

@retained func __chx__fetch_sub_u64(ptr : *mut u64, val : u64) : u64 {
    var neg : u64 = 0 - val
    var old_val : u64 = 0
    asm("lock xaddq %0, %1" : "=r"(old_val), "+m"(*ptr) : "0"(neg) : "cc", "memory")
    return old_val
}

@retained func __chx__fetch_and_u64(ptr : *mut u64, val : u64) : u64 {
    var old_val : u64 = 0
    asm("movq %1, %0" : "=r"(old_val) : "m"(*ptr) : "memory")
    while(true) {
        var new_val : u64 = old_val & val
        var success : bool = false
        asm("lock cmpxchgq %3, %2\n\tsete %1" : "=a"(old_val), "=q"(success), "+m"(*ptr) : "r"(new_val), "0"(old_val) : "cc", "memory")
        if(success) { return old_val }
    }
    return 0
}

@retained func __chx__fetch_or_u64(ptr : *mut u64, val : u64) : u64 {
    var old_val : u64 = 0
    asm("movq %1, %0" : "=r"(old_val) : "m"(*ptr) : "memory")
    while(true) {
        var new_val : u64 = old_val | val
        var success : bool = false
        asm("lock cmpxchgq %3, %2\n\tsete %1" : "=a"(old_val), "=q"(success), "+m"(*ptr) : "r"(new_val), "0"(old_val) : "cc", "memory")
        if(success) { return old_val }
    }
    return 0
}

@retained func __chx__fetch_xor_u64(ptr : *mut u64, val : u64) : u64 {
    var old_val : u64 = 0
    asm("movq %1, %0" : "=r"(old_val) : "m"(*ptr) : "memory")
    while(true) {
        var new_val : u64 = old_val ^ val
        var success : bool = false
        asm("lock cmpxchgq %3, %2\n\tsete %1" : "=a"(old_val), "=q"(success), "+m"(*ptr) : "r"(new_val), "0"(old_val) : "cc", "memory")
        if(success) { return old_val }
    }
    return 0
}

// --- u32 CAS-loop helpers ---

@retained func __chx__exchange_u32(ptr : *mut u32, val : u32) : u32 {
    var old_val : u32 = 0
    asm("xchgl %0, %1" : "=r"(old_val), "+m"(*ptr) : "0"(val) : "memory")
    return old_val
}

@retained func __chx__fetch_add_u32(ptr : *mut u32, val : u32) : u32 {
    var old_val : u32 = 0
    asm("lock xaddl %0, %1" : "=r"(old_val), "+m"(*ptr) : "0"(val) : "cc", "memory")
    return old_val
}

@retained func __chx__fetch_sub_u32(ptr : *mut u32, val : u32) : u32 {
    var neg : u32 = 0 - val
    var old_val : u32 = 0
    asm("lock xaddl %0, %1" : "=r"(old_val), "+m"(*ptr) : "0"(neg) : "cc", "memory")
    return old_val
}

@retained func __chx__fetch_and_u32(ptr : *mut u32, val : u32) : u32 {
    var old_val : u32 = 0
    asm("movl %1, %0" : "=r"(old_val) : "m"(*ptr) : "memory")
    while(true) {
        var new_val : u32 = old_val & val
        var success : bool = false
        asm("lock cmpxchgl %3, %2\n\tsete %1" : "=a"(old_val), "=q"(success), "+m"(*ptr) : "r"(new_val), "0"(old_val) : "cc", "memory")
        if(success) { return old_val }
    }
    return 0
}

@retained func __chx__fetch_or_u32(ptr : *mut u32, val : u32) : u32 {
    var old_val : u32 = 0
    asm("movl %1, %0" : "=r"(old_val) : "m"(*ptr) : "memory")
    while(true) {
        var new_val : u32 = old_val | val
        var success : bool = false
        asm("lock cmpxchgl %3, %2\n\tsete %1" : "=a"(old_val), "=q"(success), "+m"(*ptr) : "r"(new_val), "0"(old_val) : "cc", "memory")
        if(success) { return old_val }
    }
    return 0
}

@retained func __chx__fetch_xor_u32(ptr : *mut u32, val : u32) : u32 {
    var old_val : u32 = 0
    asm("movl %1, %0" : "=r"(old_val) : "m"(*ptr) : "memory")
    while(true) {
        var new_val : u32 = old_val ^ val
        var success : bool = false
        asm("lock cmpxchgl %3, %2\n\tsete %1" : "=a"(old_val), "=q"(success), "+m"(*ptr) : "r"(new_val), "0"(old_val) : "cc", "memory")
        if(success) { return old_val }
    }
    return 0
}

// --- u16 CAS-loop helpers ---

@retained func __chx__exchange_u16(ptr : *mut u16, val : u16) : u16 {
    var old_val : u16 = 0
    asm("xchgw %0, %1" : "=r"(old_val), "+m"(*ptr) : "0"(val) : "memory")
    return old_val
}

@retained func __chx__fetch_add_u16(ptr : *mut u16, val : u16) : u16 {
    var old_val : u16 = 0
    asm("lock xaddw %0, %1" : "=r"(old_val), "+m"(*ptr) : "0"(val) : "cc", "memory")
    return old_val
}

@retained func __chx__fetch_sub_u16(ptr : *mut u16, val : u16) : u16 {
    var neg : u16 = 0 - val
    var old_val : u16 = 0
    asm("lock xaddw %0, %1" : "=r"(old_val), "+m"(*ptr) : "0"(neg) : "cc", "memory")
    return old_val
}

@retained func __chx__fetch_and_u16(ptr : *mut u16, val : u16) : u16 {
    var old_val : u16 = 0
    asm("movw %1, %0" : "=r"(old_val) : "m"(*ptr) : "memory")
    while(true) {
        var new_val : u16 = old_val & val
        var success : bool = false
        asm("lock cmpxchgw %3, %2\n\tsete %1" : "=a"(old_val), "=q"(success), "+m"(*ptr) : "r"(new_val), "0"(old_val) : "cc", "memory")
        if(success) { return old_val }
    }
    return 0
}

@retained func __chx__fetch_or_u16(ptr : *mut u16, val : u16) : u16 {
    var old_val : u16 = 0
    asm("movw %1, %0" : "=r"(old_val) : "m"(*ptr) : "memory")
    while(true) {
        var new_val : u16 = old_val | val
        var success : bool = false
        asm("lock cmpxchgw %3, %2\n\tsete %1" : "=a"(old_val), "=q"(success), "+m"(*ptr) : "r"(new_val), "0"(old_val) : "cc", "memory")
        if(success) { return old_val }
    }
    return 0
}

@retained func __chx__fetch_xor_u16(ptr : *mut u16, val : u16) : u16 {
    var old_val : u16 = 0
    asm("movw %1, %0" : "=r"(old_val) : "m"(*ptr) : "memory")
    while(true) {
        var new_val : u16 = old_val ^ val
        var success : bool = false
        asm("lock cmpxchgw %3, %2\n\tsete %1" : "=a"(old_val), "=q"(success), "+m"(*ptr) : "r"(new_val), "0"(old_val) : "cc", "memory")
        if(success) { return old_val }
    }
    return 0
}

// --- u8 CAS-loop helpers ---

@retained func __chx__exchange_u8(ptr : *mut u8, val : u8) : u8 {
    var old_val : u8 = 0
    asm("xchgb %0, %1" : "=r"(old_val), "+m"(*ptr) : "0"(val) : "memory")
    return old_val
}

@retained func __chx__fetch_add_u8(ptr : *mut u8, val : u8) : u8 {
    var old_val : u8 = 0
    asm("lock xaddb %0, %1" : "=r"(old_val), "+m"(*ptr) : "0"(val) : "cc", "memory")
    return old_val
}

@retained func __chx__fetch_sub_u8(ptr : *mut u8, val : u8) : u8 {
    var neg : u8 = 0 - val
    var old_val : u8 = 0
    asm("lock xaddb %0, %1" : "=r"(old_val), "+m"(*ptr) : "0"(neg) : "cc", "memory")
    return old_val
}

@retained func __chx__fetch_and_u8(ptr : *mut u8, val : u8) : u8 {
    var old_val : u8 = 0
    asm("movb %1, %0" : "=r"(old_val) : "m"(*ptr) : "memory")
    while(true) {
        var new_val : u8 = old_val & val
        var success : bool = false
        asm("lock cmpxchgb %3, %2\n\tsete %1" : "=a"(old_val), "=q"(success), "+m"(*ptr) : "r"(new_val), "0"(old_val) : "cc", "memory")
        if(success) { return old_val }
    }
    return 0
}

@retained func __chx__fetch_or_u8(ptr : *mut u8, val : u8) : u8 {
    var old_val : u8 = 0
    asm("movb %1, %0" : "=r"(old_val) : "m"(*ptr) : "memory")
    while(true) {
        var new_val : u8 = old_val | val
        var success : bool = false
        asm("lock cmpxchgb %3, %2\n\tsete %1" : "=a"(old_val), "=q"(success), "+m"(*ptr) : "r"(new_val), "0"(old_val) : "cc", "memory")
        if(success) { return old_val }
    }
    return 0
}

@retained func __chx__fetch_xor_u8(ptr : *mut u8, val : u8) : u8 {
    var old_val : u8 = 0
    asm("movb %1, %0" : "=r"(old_val) : "m"(*ptr) : "memory")
    while(true) {
        var new_val : u8 = old_val ^ val
        var success : bool = false
        asm("lock cmpxchgb %3, %2\n\tsete %1" : "=a"(old_val), "=q"(success), "+m"(*ptr) : "r"(new_val), "0"(old_val) : "cc", "memory")
        if(success) { return old_val }
    }
    return 0
}

// ========================
// Inline-asm store helpers
// ========================

@retained func __chx__store_u64(ptr : *mut u64, val : u64) : u64 {
    asm("movq %1, %0" : "=m"(*ptr) : "r"(val) : "memory")
    return val
}

@retained func __chx__store_u32(ptr : *mut u32, val : u32) : u32 {
    asm("movl %1, %0" : "=m"(*ptr) : "r"(val) : "memory")
    return val
}

@retained func __chx__store_u16(ptr : *mut u16, val : u16) : u16 {
    asm("movw %1, %0" : "=m"(*ptr) : "r"(val) : "memory")
    return val
}

@retained func __chx__store_u8(ptr : *mut u8, val : u8) : u8 {
    asm("movb %1, %0" : "=m"(*ptr) : "r"(val) : "memory")
    return val
}

// ========================
// Inline-asm load helpers
// ========================

@retained func __chx__load_u64(ptr : *u64) : u64 {
    var val : u64 = 0
    asm("movq %1, %0" : "=r"(val) : "m"(*ptr) : "memory")
    return val
}

@retained func __chx__load_u32(ptr : *u32) : u32 {
    var val : u32 = 0
    asm("movl %1, %0" : "=r"(val) : "m"(*ptr) : "memory")
    return val
}

@retained func __chx__load_u16(ptr : *u16) : u16 {
    var val : u16 = 0
    asm("movw %1, %0" : "=r"(val) : "m"(*ptr) : "memory")
    return val
}

@retained func __chx__load_u8(ptr : *u8) : u8 {
    var val : u8 = 0
    asm("movb %1, %0" : "=r"(val) : "m"(*ptr) : "memory")
    return val
}

// ========================
// AArch64 inline-asm primitives
// ========================

@retained func __chx__cas_u64_aarch64(ptr : *mut u64, expected : *mut u64, desired : u64) : bool {
    comptime if(def.aarch64) {
        var success : u32 = 0
        asm("dmb ish\n\t1: ldxr %0, %2\n\tcmp %0, %4\n\tb.ne 2f\n\tstxr %w1, %3, %2\n\tcbnz %w1, 1b\n\t2:\n\tdmb ish"
            : "=&r"(*expected), "=&r"(success), "+Q"(*ptr)
            : "r"(desired), "r"(*expected)
            : "cc", "memory")
        return success == 0
    } else {
        return false
    }
}

@retained func __chx__cas_u32_aarch64(ptr : *mut u32, expected : *mut u32, desired : u32) : bool {
    comptime if(def.aarch64) {
        var success : u32 = 0
        asm("dmb ish\n\t1: ldxr %w0, %2\n\tcmp %w0, %w4\n\tb.ne 2f\n\tstxr %w1, %w3, %2\n\tcbnz %w1, 1b\n\t2:\n\tdmb ish"
            : "=&r"(*expected), "=&r"(success), "+Q"(*ptr)
            : "r"(desired), "r"(*expected)
            : "cc", "memory")
        return success == 0
    } else {
        return false
    }
}

@retained func __chx__cas_u16_aarch64(ptr : *mut u16, expected : *mut u16, desired : u16) : bool {
    comptime if(def.aarch64) {
        var expected_val : u16 = *expected
        var old_word : u32 = 0
        var new_word : u32 = 0
        var success : u32 = 0
        var byte_off : u64 = (ptr as u64) & 3
        var shift : u32 = byte_off * 8
        var mask : u32 = 0xFFFF << shift
        var aligned_ptr : *mut u32 = (ptr as *mut u32) - byte_off
        while(true) {
            asm("dmb ish\n\t1: ldxr %0, %2\n\tcmp %0, %4\n\tb.ne 2f\n\tstxr %w1, %3, %2\n\tcbnz %w1, 1b\n\t2:\n\tdmb ish"
                : "=&r"(old_word), "=&r"(success), "+Q"(*aligned_ptr)
                : "r"(new_word), "r"(old_word)
                : "cc", "memory")
            var old_val : u32 = (old_word >> shift) & 0xFFFF
            if(old_val != (expected_val as u32)) {
                *expected = old_val as u16
                return false
            }
            new_word = (old_word & ~mask) | ((desired as u32) << shift)
            if(success == 0) { return true }
        }
        return false
    } else {
        return false
    }
}

@retained func __chx__cas_u8_aarch64(ptr : *mut u8, expected : *mut u8, desired : u8) : bool {
    comptime if(def.aarch64) {
        var expected_val : u8 = *expected
        var old_word : u32 = 0
        var new_word : u32 = 0
        var success : u32 = 0
        var byte_off : u64 = (ptr as u64) & 3
        var shift : u32 = byte_off * 8
        var mask : u32 = 0xFF << shift
        var aligned_ptr : *mut u32 = (ptr as *mut u32) - byte_off
        while(true) {
            asm("dmb ish\n\t1: ldxr %0, %2\n\tcmp %0, %4\n\tb.ne 2f\n\tstxr %w1, %3, %2\n\tcbnz %w1, 1b\n\t2:\n\tdmb ish"
                : "=&r"(old_word), "=&r"(success), "+Q"(*aligned_ptr)
                : "r"(new_word), "r"(old_word)
                : "cc", "memory")
            var old_val : u32 = (old_word >> shift) & 0xFF
            if(old_val != (expected_val as u32)) {
                *expected = old_val as u8
                return false
            }
            new_word = (old_word & ~mask) | ((desired as u32) << shift)
            if(success == 0) { return true }
        }
        return false
    } else {
        return false
    }
}

@retained func __chx__exchange_u64_aarch64(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.aarch64) {
        var old_val : u64 = 0
        var success : u32 = 0
        asm("dmb ish\n\t1: ldxr %0, %2\n\tstxr %w1, %3, %2\n\tcbnz %w1, 1b\n\tdmb ish"
            : "=&r"(old_val), "=&r"(success), "+Q"(*ptr)
            : "r"(val)
            : "cc", "memory")
        return old_val
    } else {
        return 0
    }
}

@retained func __chx__exchange_u32_aarch64(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.aarch64) {
        var old_val : u32 = 0
        var success : u32 = 0
        asm("dmb ish\n\t1: ldxr %w0, %2\n\tstxr %w1, %w3, %2\n\tcbnz %w1, 1b\n\tdmb ish"
            : "=&r"(old_val), "=&r"(success), "+Q"(*ptr)
            : "r"(val)
            : "cc", "memory")
        return old_val
    } else {
        return 0
    }
}

@retained func __chx__exchange_u16_aarch64(ptr : *mut u16, val : u16) : u16 {
    comptime if(def.aarch64) {
        var old_val : u16 = 0
        var cas_expected : u16 = *ptr
        while(true) {
            if(__chx__cas_u16_aarch64(ptr, &raw mut cas_expected, val)) {
                return cas_expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__exchange_u8_aarch64(ptr : *mut u8, val : u8) : u8 {
    comptime if(def.aarch64) {
        var old_val : u8 = 0
        var cas_expected : u8 = *ptr
        while(true) {
            if(__chx__cas_u8_aarch64(ptr, &raw mut cas_expected, val)) {
                return cas_expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_add_u64_aarch64(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.aarch64) {
        var old_val : u64 = 0
        var success : u32 = 0
        var new_val : u64 = 0
        asm("dmb ish\n\t1: ldxr %0, %3\n\tadd %2, %0, %4\n\tstxr %w1, %2, %3\n\tcbnz %w1, 1b\n\tdmb ish"
            : "=&r"(old_val), "=&r"(success), "=&r"(new_val), "+Q"(*ptr)
            : "r"(val)
            : "cc", "memory")
        return old_val
    } else {
        return 0
    }
}

@retained func __chx__fetch_add_u32_aarch64(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.aarch64) {
        var old_val : u32 = 0
        var success : u32 = 0
        var new_val : u32 = 0
        asm("dmb ish\n\t1: ldxr %w0, %3\n\tadd %w2, %w0, %w4\n\tstxr %w1, %w2, %3\n\tcbnz %w1, 1b\n\tdmb ish"
            : "=&r"(old_val), "=&r"(success), "=&r"(new_val), "+Q"(*ptr)
            : "r"(val)
            : "cc", "memory")
        return old_val
    } else {
        return 0
    }
}

@retained func __chx__fetch_sub_u64_aarch64(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.aarch64) {
        var old_val : u64 = 0
        var success : u32 = 0
        var new_val : u64 = 0
        asm("dmb ish\n\t1: ldxr %0, %3\n\tsub %2, %0, %4\n\tstxr %w1, %2, %3\n\tcbnz %w1, 1b\n\tdmb ish"
            : "=&r"(old_val), "=&r"(success), "=&r"(new_val), "+Q"(*ptr)
            : "r"(val)
            : "cc", "memory")
        return old_val
    } else {
        return 0
    }
}

@retained func __chx__fetch_sub_u32_aarch64(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.aarch64) {
        var old_val : u32 = 0
        var success : u32 = 0
        var new_val : u32 = 0
        asm("dmb ish\n\t1: ldxr %w0, %3\n\tsub %w2, %w0, %w4\n\tstxr %w1, %w2, %3\n\tcbnz %w1, 1b\n\tdmb ish"
            : "=&r"(old_val), "=&r"(success), "=&r"(new_val), "+Q"(*ptr)
            : "r"(val)
            : "cc", "memory")
        return old_val
    } else {
        return 0
    }
}

@retained func __chx__fetch_and_u64_aarch64(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.aarch64) {
        var old_val : u64 = 0
        var success : u32 = 0
        var new_val : u64 = 0
        asm("dmb ish\n\t1: ldxr %0, %3\n\tand %2, %0, %4\n\tstxr %w1, %2, %3\n\tcbnz %w1, 1b\n\tdmb ish"
            : "=&r"(old_val), "=&r"(success), "=&r"(new_val), "+Q"(*ptr)
            : "r"(val)
            : "cc", "memory")
        return old_val
    } else {
        return 0
    }
}

@retained func __chx__fetch_and_u32_aarch64(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.aarch64) {
        var old_val : u32 = 0
        var success : u32 = 0
        var new_val : u32 = 0
        asm("dmb ish\n\t1: ldxr %w0, %3\n\tand %w2, %w0, %w4\n\tstxr %w1, %w2, %3\n\tcbnz %w1, 1b\n\tdmb ish"
            : "=&r"(old_val), "=&r"(success), "=&r"(new_val), "+Q"(*ptr)
            : "r"(val)
            : "cc", "memory")
        return old_val
    } else {
        return 0
    }
}

@retained func __chx__fetch_or_u64_aarch64(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.aarch64) {
        var old_val : u64 = 0
        var success : u32 = 0
        var new_val : u64 = 0
        asm("dmb ish\n\t1: ldxr %0, %3\n\torr %2, %0, %4\n\tstxr %w1, %2, %3\n\tcbnz %w1, 1b\n\tdmb ish"
            : "=&r"(old_val), "=&r"(success), "=&r"(new_val), "+Q"(*ptr)
            : "r"(val)
            : "cc", "memory")
        return old_val
    } else {
        return 0
    }
}

@retained func __chx__fetch_or_u32_aarch64(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.aarch64) {
        var old_val : u32 = 0
        var success : u32 = 0
        var new_val : u32 = 0
        asm("dmb ish\n\t1: ldxr %w0, %3\n\torr %w2, %w0, %w4\n\tstxr %w1, %w2, %3\n\tcbnz %w1, 1b\n\tdmb ish"
            : "=&r"(old_val), "=&r"(success), "=&r"(new_val), "+Q"(*ptr)
            : "r"(val)
            : "cc", "memory")
        return old_val
    } else {
        return 0
    }
}

@retained func __chx__fetch_xor_u64_aarch64(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.aarch64) {
        var old_val : u64 = 0
        var success : u32 = 0
        var new_val : u64 = 0
        asm("dmb ish\n\t1: ldxr %0, %3\n\teor %2, %0, %4\n\tstxr %w1, %2, %3\n\tcbnz %w1, 1b\n\tdmb ish"
            : "=&r"(old_val), "=&r"(success), "=&r"(new_val), "+Q"(*ptr)
            : "r"(val)
            : "cc", "memory")
        return old_val
    } else {
        return 0
    }
}

@retained func __chx__fetch_xor_u32_aarch64(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.aarch64) {
        var old_val : u32 = 0
        var success : u32 = 0
        var new_val : u32 = 0
        asm("dmb ish\n\t1: ldxr %w0, %3\n\teor %w2, %w0, %w4\n\tstxr %w1, %w2, %3\n\tcbnz %w1, 1b\n\tdmb ish"
            : "=&r"(old_val), "=&r"(success), "=&r"(new_val), "+Q"(*ptr)
            : "r"(val)
            : "cc", "memory")
        return old_val
    } else {
        return 0
    }
}

@retained func __chx__store_u64_aarch64(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.aarch64) {
        asm("dmb ish\n\tstlr %1, %0\n\tdmb ish"
            : "=Q"(*ptr) : "r"(val) : "memory")
        return val
    } else {
        return val
    }
}

@retained func __chx__store_u32_aarch64(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.aarch64) {
        asm("dmb ish\n\tstlr %w1, %0\n\tdmb ish"
            : "=Q"(*ptr) : "r"(val) : "memory")
        return val
    } else {
        return val
    }
}

@retained func __chx__store_u16_aarch64(ptr : *mut u16, val : u16) : u16 {
    comptime if(def.aarch64) {
        var dummy : u16 = val
        var cas_expected : u16 = *ptr
        while(true) {
            if(__chx__cas_u16_aarch64(ptr, &raw mut cas_expected, val)) {
                return val
            }
        }
        return val
    } else {
        return val
    }
}

@retained func __chx__store_u8_aarch64(ptr : *mut u8, val : u8) : u8 {
    comptime if(def.aarch64) {
        var dummy : u8 = val
        var cas_expected : u8 = *ptr
        while(true) {
            if(__chx__cas_u8_aarch64(ptr, &raw mut cas_expected, val)) {
                return val
            }
        }
        return val
    } else {
        return val
    }
}

@retained func __chx__load_u64_aarch64(ptr : *u64) : u64 {
    comptime if(def.aarch64) {
        var val : u64 = 0
        asm("dmb ish\n\tldar %0, %1\n\tdmb ish"
            : "=r"(val) : "Q"(*ptr) : "memory")
        return val
    } else {
        return 0
    }
}

@retained func __chx__load_u32_aarch64(ptr : *u32) : u32 {
    comptime if(def.aarch64) {
        var val : u32 = 0
        asm("dmb ish\n\tldar %w0, %1\n\tdmb ish"
            : "=r"(val) : "Q"(*ptr) : "memory")
        return val
    } else {
        return 0
    }
}

@retained func __chx__load_u16_aarch64(ptr : *u16) : u16 {
    comptime if(def.aarch64) {
        var expected : u16 = *ptr
        while(true) {
            if(__chx__cas_u16_aarch64(ptr as *mut u16, &raw mut expected, expected)) {
                return expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__load_u8_aarch64(ptr : *u8) : u8 {
    comptime if(def.aarch64) {
        var expected : u8 = *ptr
        while(true) {
            if(__chx__cas_u8_aarch64(ptr as *mut u8, &raw mut expected, expected)) {
                return expected
            }
        }
        return 0
    } else {
        return 0
    }
}

// ========================
// RISC-V inline-asm primitives (lr/sc)
// ========================

@retained func __chx__cas_u64_riscv(ptr : *mut u64, expected : *mut u64, desired : u64) : bool {
    comptime if(def.riscv64) {
        var success : u64 = 0
        asm("1: lr.d %0, %3\n\tbne %0, %4, 2f\n\tsc.d %1, %5, %3\n\tbnez %1, 1b\n\t2:"
            : "=&r"(*expected), "=&r"(success), "+A"(*ptr)
            : "r"(*expected), "r"(desired)
            : "cc", "memory")
        return success == 0
    } else {
        return false
    }
}

@retained func __chx__cas_u32_riscv(ptr : *mut u32, expected : *mut u32, desired : u32) : bool {
    comptime if(def.riscv32 || def.riscv64) {
        var success : u64 = 0
        asm("1: lr.w %0, %3\n\tbne %0, %4, 2f\n\tsc.w %1, %5, %3\n\tbnez %1, 1b\n\t2:"
            : "=&r"(*expected), "=&r"(success), "+A"(*ptr)
            : "r"(*expected), "r"(desired)
            : "cc", "memory")
        return success == 0
    } else {
        return false
    }
}

@retained func __chx__exchange_u64_riscv(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.riscv64) {
        var old_val : u64 = 0
        while(true) {
            var cas_expected : u64 = old_val
            if(__chx__cas_u64_riscv(ptr, &raw mut cas_expected, val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__exchange_u32_riscv(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.riscv32 || def.riscv64) {
        var old_val : u32 = 0
        while(true) {
            var cas_expected : u32 = old_val
            if(__chx__cas_u32_riscv(ptr, &raw mut cas_expected, val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_add_u64_riscv(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.riscv64) {
        var old_val : u64 = *ptr
        while(true) {
            var cas_expected : u64 = old_val
            if(__chx__cas_u64_riscv(ptr, &raw mut cas_expected, old_val + val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_add_u32_riscv(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.riscv32 || def.riscv64) {
        var old_val : u32 = *ptr
        while(true) {
            var cas_expected : u32 = old_val
            if(__chx__cas_u32_riscv(ptr, &raw mut cas_expected, old_val + val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_sub_u64_riscv(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.riscv64) {
        var old_val : u64 = *ptr
        while(true) {
            var cas_expected : u64 = old_val
            if(__chx__cas_u64_riscv(ptr, &raw mut cas_expected, old_val - val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_sub_u32_riscv(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.riscv32 || def.riscv64) {
        var old_val : u32 = *ptr
        while(true) {
            var cas_expected : u32 = old_val
            if(__chx__cas_u32_riscv(ptr, &raw mut cas_expected, old_val - val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_and_u64_riscv(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.riscv64) {
        var old_val : u64 = *ptr
        while(true) {
            var cas_expected : u64 = old_val
            if(__chx__cas_u64_riscv(ptr, &raw mut cas_expected, old_val & val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_and_u32_riscv(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.riscv32 || def.riscv64) {
        var old_val : u32 = *ptr
        while(true) {
            var cas_expected : u32 = old_val
            if(__chx__cas_u32_riscv(ptr, &raw mut cas_expected, old_val & val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_or_u64_riscv(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.riscv64) {
        var old_val : u64 = *ptr
        while(true) {
            var cas_expected : u64 = old_val
            if(__chx__cas_u64_riscv(ptr, &raw mut cas_expected, old_val | val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_or_u32_riscv(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.riscv32 || def.riscv64) {
        var old_val : u32 = *ptr
        while(true) {
            var cas_expected : u32 = old_val
            if(__chx__cas_u32_riscv(ptr, &raw mut cas_expected, old_val | val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_xor_u64_riscv(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.riscv64) {
        var old_val : u64 = *ptr
        while(true) {
            var cas_expected : u64 = old_val
            if(__chx__cas_u64_riscv(ptr, &raw mut cas_expected, old_val ^ val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_xor_u32_riscv(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.riscv32 || def.riscv64) {
        var old_val : u32 = *ptr
        while(true) {
            var cas_expected : u32 = old_val
            if(__chx__cas_u32_riscv(ptr, &raw mut cas_expected, old_val ^ val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__store_u64_riscv(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.riscv64) {
        asm("fence rw, rw\n\tamoswap.d.rl %0, %1, %2"
            : "=r"(val), "+A"(*ptr) : "0"(val) : "memory")
        return val
    } else {
        return val
    }
}

@retained func __chx__store_u32_riscv(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.riscv32 || def.riscv64) {
        asm("fence rw, rw\n\tamoswap.w.rl %0, %1, %2"
            : "=r"(val), "+A"(*ptr) : "0"(val) : "memory")
        return val
    } else {
        return val
    }
}

@retained func __chx__load_u64_riscv(ptr : *u64) : u64 {
    comptime if(def.riscv64) {
        var val : u64 = 0
        asm("amoswap.aq %0, %1, %2"
            : "=r"(val), "+A"(*ptr) : "0"(val) : "memory")
        return val
    } else {
        return 0
    }
}

@retained func __chx__load_u32_riscv(ptr : *u32) : u32 {
    comptime if(def.riscv32 || def.riscv64) {
        var val : u32 = 0
        asm("amoswap.aq %0, %1, %2"
            : "=r"(val), "+A"(*ptr) : "0"(val) : "memory")
        return val
    } else {
        return 0
    }
}

@retained func __chx__cas_u16_riscv(ptr : *mut u16, expected : *mut u16, desired : u16) : bool {
    comptime if(def.riscv64) {
        var expected_val : u16 = *expected
        var old_word : u32 = 0
        var new_word : u32 = 0
        var success : u64 = 0
        var byte_off : u64 = (ptr as u64) & 3
        var shift : u32 = byte_off * 8
        var mask : u32 = 0xFFFF << shift
        var aligned_ptr : *mut u32 = (ptr as *mut u32) - byte_off
        while(true) {
            asm("1: lr.w %0, %3\n\tbne %0, %5, 2f\n\tand %4, %0, %6\n\tor %4, %4, %7\n\tsc.w %1, %4, %3\n\tbnez %1, 1b\n\t2:"
                : "=&r"(old_word), "=&r"(success), "+A"(*aligned_ptr)
                : "r"(new_word), "r"(success), "r"(old_word)
                : "cc", "memory")
            var old_val : u32 = (old_word >> shift) & 0xFFFF
            if(old_val != (expected_val as u32)) {
                *expected = old_val as u16
                return false
            }
            new_word = (old_word & ~mask) | ((desired as u32) << shift)
            if(success == 0) { return true }
        }
        return false
    } else {
        return false
    }
}

@retained func __chx__cas_u8_riscv(ptr : *mut u8, expected : *mut u8, desired : u8) : bool {
    comptime if(def.riscv64) {
        var expected_val : u8 = *expected
        var old_word : u32 = 0
        var new_word : u32 = 0
        var success : u64 = 0
        var byte_off : u64 = (ptr as u64) & 3
        var shift : u32 = byte_off * 8
        var mask : u32 = 0xFF << shift
        var aligned_ptr : *mut u32 = (ptr as *mut u32) - byte_off
        while(true) {
            asm("1: lr.w %0, %3\n\tbne %0, %5, 2f\n\tand %4, %0, %6\n\tor %4, %4, %7\n\tsc.w %1, %4, %3\n\tbnez %1, 1b\n\t2:"
                : "=&r"(old_word), "=&r"(success), "+A"(*aligned_ptr)
                : "r"(new_word), "r"(success), "r"(old_word)
                : "cc", "memory")
            var old_val : u32 = (old_word >> shift) & 0xFF
            if(old_val != (expected_val as u32)) {
                *expected = old_val as u8
                return false
            }
            new_word = (old_word & ~mask) | ((desired as u32) << shift)
            if(success == 0) { return true }
        }
        return false
    } else {
        return false
    }
}

@retained func __chx__exchange_u16_riscv(ptr : *mut u16, val : u16) : u16 {
    comptime if(def.riscv64) {
        var cas_expected : u16 = *ptr
        while(true) {
            if(__chx__cas_u16_riscv(ptr, &raw mut cas_expected, val)) {
                return cas_expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__exchange_u8_riscv(ptr : *mut u8, val : u8) : u8 {
    comptime if(def.riscv64) {
        var cas_expected : u8 = *ptr
        while(true) {
            if(__chx__cas_u8_riscv(ptr, &raw mut cas_expected, val)) {
                return cas_expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__load_u16_riscv(ptr : *u16) : u16 {
    comptime if(def.riscv64) {
        var expected : u16 = *ptr
        while(true) {
            if(__chx__cas_u16_riscv(ptr as *mut u16, &raw mut expected, expected)) {
                return expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__load_u8_riscv(ptr : *u8) : u8 {
    comptime if(def.riscv64) {
        var expected : u8 = *ptr
        while(true) {
            if(__chx__cas_u8_riscv(ptr as *mut u8, &raw mut expected, expected)) {
                return expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__store_u16_riscv(ptr : *mut u16, val : u16) : u16 {
    comptime if(def.riscv64) {
        var cas_expected : u16 = *ptr
        while(true) {
            if(__chx__cas_u16_riscv(ptr, &raw mut cas_expected, val)) {
                return val
            }
        }
        return val
    } else {
        return val
    }
}

@retained func __chx__store_u8_riscv(ptr : *mut u8, val : u8) : u8 {
    comptime if(def.riscv64) {
        var cas_expected : u8 = *ptr
        while(true) {
            if(__chx__cas_u8_riscv(ptr, &raw mut cas_expected, val)) {
                return val
            }
        }
        return val
    } else {
        return val
    }
}

@retained func __chx__fetch_add_u16_riscv(ptr : *mut u16, val : u16) : u16 {
    comptime if(def.riscv64) {
        var expected : u16 = *ptr
        while(true) {
            var new_val : u16 = expected + val
            if(__chx__cas_u16_riscv(ptr, &raw mut expected, new_val)) {
                return expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_add_u8_riscv(ptr : *mut u8, val : u8) : u8 {
    comptime if(def.riscv64) {
        var expected : u8 = *ptr
        while(true) {
            var new_val : u8 = expected + val
            if(__chx__cas_u8_riscv(ptr, &raw mut expected, new_val)) {
                return expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_sub_u16_riscv(ptr : *mut u16, val : u16) : u16 {
    comptime if(def.riscv64) {
        var expected : u16 = *ptr
        while(true) {
            var new_val : u16 = expected - val
            if(__chx__cas_u16_riscv(ptr, &raw mut expected, new_val)) {
                return expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_sub_u8_riscv(ptr : *mut u8, val : u8) : u8 {
    comptime if(def.riscv64) {
        var expected : u8 = *ptr
        while(true) {
            var new_val : u8 = expected - val
            if(__chx__cas_u8_riscv(ptr, &raw mut expected, new_val)) {
                return expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_and_u16_riscv(ptr : *mut u16, val : u16) : u16 {
    comptime if(def.riscv64) {
        var expected : u16 = *ptr
        while(true) {
            var new_val : u16 = expected & val
            if(__chx__cas_u16_riscv(ptr, &raw mut expected, new_val)) {
                return expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_and_u8_riscv(ptr : *mut u8, val : u8) : u8 {
    comptime if(def.riscv64) {
        var expected : u8 = *ptr
        while(true) {
            var new_val : u8 = expected & val
            if(__chx__cas_u8_riscv(ptr, &raw mut expected, new_val)) {
                return expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_or_u16_riscv(ptr : *mut u16, val : u16) : u16 {
    comptime if(def.riscv64) {
        var expected : u16 = *ptr
        while(true) {
            var new_val : u16 = expected | val
            if(__chx__cas_u16_riscv(ptr, &raw mut expected, new_val)) {
                return expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_or_u8_riscv(ptr : *mut u8, val : u8) : u8 {
    comptime if(def.riscv64) {
        var expected : u8 = *ptr
        while(true) {
            var new_val : u8 = expected | val
            if(__chx__cas_u8_riscv(ptr, &raw mut expected, new_val)) {
                return expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_xor_u16_riscv(ptr : *mut u16, val : u16) : u16 {
    comptime if(def.riscv64) {
        var expected : u16 = *ptr
        while(true) {
            var new_val : u16 = expected ^ val
            if(__chx__cas_u16_riscv(ptr, &raw mut expected, new_val)) {
                return expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_xor_u8_riscv(ptr : *mut u8, val : u8) : u8 {
    comptime if(def.riscv64) {
        var expected : u8 = *ptr
        while(true) {
            var new_val : u8 = expected ^ val
            if(__chx__cas_u8_riscv(ptr, &raw mut expected, new_val)) {
                return expected
            }
        }
        return 0
    } else {
        return 0
    }
}

// ========================
// Arm32 inline-asm primitives (ldrex/strex)
// ========================

@retained func __chx__cas_u64_arm(ptr : *mut u64, expected : *mut u64, desired : u64) : bool {
    comptime if(def.arm) {
        var success : u32 = 0
        asm("dmb ish\n\t1: ldrexd %0, %H0, %2\n\tcmp %0, %4\n\tcmpne %H0, %H4\n\tbne 2f\n\tstrexd %1, %5, %H5, %2\n\tcmp %1, #0\n\tbne 1b\n\t2:\n\tdmb ish"
            : "=&r"(*expected), "=&r"(success), "+Q"(*ptr)
            : "r"(*expected), "r"(desired)
            : "cc", "memory")
        return success == 0
    } else {
        return false
    }
}

@retained func __chx__cas_u32_arm(ptr : *mut u32, expected : *mut u32, desired : u32) : bool {
    comptime if(def.arm) {
        var success : u32 = 0
        asm("dmb ish\n\t1: ldrex %0, %2\n\tcmp %0, %4\n\tbne 2f\n\tstrex %1, %3, %2\n\tcmp %1, #0\n\tbne 1b\n\t2:\n\tdmb ish"
            : "=&r"(*expected), "=&r"(success), "+Q"(*ptr)
            : "r"(desired), "r"(*expected)
            : "cc", "memory")
        return success == 0
    } else {
        return false
    }
}

@retained func __chx__cas_u16_arm(ptr : *mut u16, expected : *mut u16, desired : u16) : bool {
    comptime if(def.arm) {
        var expected_val : u16 = *expected
        var old_word : u32 = 0
        var new_word : u32 = 0
        var success : u32 = 0
        var byte_off : u32 = (ptr as u32) & 3
        var shift : u32 = byte_off * 8
        var mask : u32 = 0xFFFF << shift
        var aligned_ptr : *mut u32 = (ptr as *mut u32) - byte_off
        while(true) {
            asm("dmb ish\n\t1: ldrex %0, %3\n\tcmp %0, %5\n\tbne 2f\n\tstrex %1, %4, %3\n\tcmp %1, #0\n\tbne 1b\n\t2:\n\tdmb ish"
                : "=&r"(old_word), "=&r"(success), "+Q"(*aligned_ptr)
                : "r"(new_word), "r"(old_word)
                : "cc", "memory")
            var old_val : u32 = (old_word >> shift) & 0xFFFF
            if(old_val != (expected_val as u32)) {
                *expected = old_val as u16
                return false
            }
            new_word = (old_word & ~mask) | ((desired as u32) << shift)
            if(success == 0) { return true }
        }
        return false
    } else {
        return false
    }
}

@retained func __chx__cas_u8_arm(ptr : *mut u8, expected : *mut u8, desired : u8) : bool {
    comptime if(def.arm) {
        var expected_val : u8 = *expected
        var old_word : u32 = 0
        var new_word : u32 = 0
        var success : u32 = 0
        var byte_off : u32 = (ptr as u32) & 3
        var shift : u32 = byte_off * 8
        var mask : u32 = 0xFF << shift
        var aligned_ptr : *mut u32 = (ptr as *mut u32) - byte_off
        while(true) {
            asm("dmb ish\n\t1: ldrex %0, %3\n\tcmp %0, %5\n\tbne 2f\n\tstrex %1, %4, %3\n\tcmp %1, #0\n\tbne 1b\n\t2:\n\tdmb ish"
                : "=&r"(old_word), "=&r"(success), "+Q"(*aligned_ptr)
                : "r"(new_word), "r"(old_word)
                : "cc", "memory")
            var old_val : u32 = (old_word >> shift) & 0xFF
            if(old_val != (expected_val as u32)) {
                *expected = old_val as u8
                return false
            }
            new_word = (old_word & ~mask) | ((desired as u32) << shift)
            if(success == 0) { return true }
        }
        return false
    } else {
        return false
    }
}

@retained func __chx__exchange_u64_arm(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.arm) {
        var old_val : u64 = 0
        while(true) {
            var cas_expected : u64 = old_val
            if(__chx__cas_u64_arm(ptr, &raw mut cas_expected, val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__exchange_u32_arm(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.arm) {
        var old_val : u32 = 0
        while(true) {
            var cas_expected : u32 = old_val
            if(__chx__cas_u32_arm(ptr, &raw mut cas_expected, val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__exchange_u16_arm(ptr : *mut u16, val : u16) : u16 {
    comptime if(def.arm) {
        var cas_expected : u16 = *ptr
        while(true) {
            if(__chx__cas_u16_arm(ptr, &raw mut cas_expected, val)) {
                return cas_expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__exchange_u8_arm(ptr : *mut u8, val : u8) : u8 {
    comptime if(def.arm) {
        var cas_expected : u8 = *ptr
        while(true) {
            if(__chx__cas_u8_arm(ptr, &raw mut cas_expected, val)) {
                return cas_expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_add_u64_arm(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.arm) {
        var old_val : u64 = *ptr
        while(true) {
            var cas_expected : u64 = old_val
            if(__chx__cas_u64_arm(ptr, &raw mut cas_expected, old_val + val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_add_u32_arm(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.arm) {
        var old_val : u32 = *ptr
        while(true) {
            var cas_expected : u32 = old_val
            if(__chx__cas_u32_arm(ptr, &raw mut cas_expected, old_val + val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_sub_u64_arm(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.arm) {
        var old_val : u64 = *ptr
        while(true) {
            var cas_expected : u64 = old_val
            if(__chx__cas_u64_arm(ptr, &raw mut cas_expected, old_val - val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_sub_u32_arm(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.arm) {
        var old_val : u32 = *ptr
        while(true) {
            var cas_expected : u32 = old_val
            if(__chx__cas_u32_arm(ptr, &raw mut cas_expected, old_val - val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_and_u64_arm(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.arm) {
        var old_val : u64 = *ptr
        while(true) {
            var cas_expected : u64 = old_val
            if(__chx__cas_u64_arm(ptr, &raw mut cas_expected, old_val & val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_and_u32_arm(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.arm) {
        var old_val : u32 = *ptr
        while(true) {
            var cas_expected : u32 = old_val
            if(__chx__cas_u32_arm(ptr, &raw mut cas_expected, old_val & val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_or_u64_arm(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.arm) {
        var old_val : u64 = *ptr
        while(true) {
            var cas_expected : u64 = old_val
            if(__chx__cas_u64_arm(ptr, &raw mut cas_expected, old_val | val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_or_u32_arm(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.arm) {
        var old_val : u32 = *ptr
        while(true) {
            var cas_expected : u32 = old_val
            if(__chx__cas_u32_arm(ptr, &raw mut cas_expected, old_val | val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_xor_u64_arm(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.arm) {
        var old_val : u64 = *ptr
        while(true) {
            var cas_expected : u64 = old_val
            if(__chx__cas_u64_arm(ptr, &raw mut cas_expected, old_val ^ val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_xor_u32_arm(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.arm) {
        var old_val : u32 = *ptr
        while(true) {
            var cas_expected : u32 = old_val
            if(__chx__cas_u32_arm(ptr, &raw mut cas_expected, old_val ^ val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__store_u64_arm(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.arm) {
        var cas_expected : u64 = *ptr
        while(true) {
            if(__chx__cas_u64_arm(ptr, &raw mut cas_expected, val)) {
                return val
            }
        }
        return val
    } else {
        return val
    }
}

@retained func __chx__store_u32_arm(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.arm) {
        var cas_expected : u32 = *ptr
        while(true) {
            if(__chx__cas_u32_arm(ptr, &raw mut cas_expected, val)) {
                return val
            }
        }
        return val
    } else {
        return val
    }
}

@retained func __chx__store_u16_arm(ptr : *mut u16, val : u16) : u16 {
    comptime if(def.arm) {
        var cas_expected : u16 = *ptr
        while(true) {
            if(__chx__cas_u16_arm(ptr, &raw mut cas_expected, val)) {
                return val
            }
        }
        return val
    } else {
        return val
    }
}

@retained func __chx__store_u8_arm(ptr : *mut u8, val : u8) : u8 {
    comptime if(def.arm) {
        var cas_expected : u8 = *ptr
        while(true) {
            if(__chx__cas_u8_arm(ptr, &raw mut cas_expected, val)) {
                return val
            }
        }
        return val
    } else {
        return val
    }
}

@retained func __chx__load_u64_arm(ptr : *u64) : u64 {
    comptime if(def.arm) {
        var expected : u64 = *ptr
        while(true) {
            if(__chx__cas_u64_arm(ptr as *mut u64, &raw mut expected, expected)) {
                return expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__load_u32_arm(ptr : *u32) : u32 {
    comptime if(def.arm) {
        var expected : u32 = *ptr
        while(true) {
            if(__chx__cas_u32_arm(ptr as *mut u32, &raw mut expected, expected)) {
                return expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__load_u16_arm(ptr : *u16) : u16 {
    comptime if(def.arm) {
        var expected : u16 = *ptr
        while(true) {
            if(__chx__cas_u16_arm(ptr as *mut u16, &raw mut expected, expected)) {
                return expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__load_u8_arm(ptr : *u8) : u8 {
    comptime if(def.arm) {
        var expected : u8 = *ptr
        while(true) {
            if(__chx__cas_u8_arm(ptr as *mut u8, &raw mut expected, expected)) {
                return expected
            }
        }
        return 0
    } else {
        return 0
    }
}

// ========================
// PowerPC inline-asm primitives (lwarx/stwcx)
// ========================

@retained func __chx__cas_u64_ppc(ptr : *mut u64, expected : *mut u64, desired : u64) : bool {
    comptime if(def.powerpc64) {
        var success : u32 = 0
        asm("1: ldarx %0, 0, %3\n\tcmpd %0, %4\n\tbne 2f\n\tstdcx. %5, 0, %3\n\tbne- 1b\n\t2:\n\tmfcr %1\n\trwlnm %1, %1, 0, 1, 0"
            : "=&r"(*expected), "=&r"(success), "+m"(*ptr)
            : "r"(expected), "r"(desired)
            : "cc", "memory")
        return (success & 0x20000000) != 0
    } else {
        return false
    }
}

@retained func __chx__cas_u32_ppc(ptr : *mut u32, expected : *mut u32, desired : u32) : bool {
    comptime if(def.powerpc || def.powerpc64) {
        var success : u32 = 0
        asm("sync\n\t1: lwarx %0, 0, %3\n\tcmpw %0, %4\n\tbne 2f\n\tstwcx. %5, 0, %3\n\tbne- 1b\n\t2:\n\tmfcr %1\n\trwlnm %1, %1, 0, 1, 0\n\tisync"
            : "=&r"(*expected), "=&r"(success), "+m"(*ptr)
            : "r"(expected), "r"(desired)
            : "cc", "memory")
        return (success & 0x20000000) != 0
    } else {
        return false
    }
}

@retained func __chx__exchange_u64_ppc(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.powerpc64) {
        var old_val : u64 = 0
        while(true) {
            var cas_expected : u64 = old_val
            if(__chx__cas_u64_ppc(ptr, &raw mut cas_expected, val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__exchange_u32_ppc(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.powerpc || def.powerpc64) {
        var old_val : u32 = 0
        while(true) {
            var cas_expected : u32 = old_val
            if(__chx__cas_u32_ppc(ptr, &raw mut cas_expected, val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_add_u64_ppc(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.powerpc64) {
        var old_val : u64 = *ptr
        while(true) {
            var cas_expected : u64 = old_val
            if(__chx__cas_u64_ppc(ptr, &raw mut cas_expected, old_val + val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_add_u32_ppc(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.powerpc || def.powerpc64) {
        var old_val : u32 = *ptr
        while(true) {
            var cas_expected : u32 = old_val
            if(__chx__cas_u32_ppc(ptr, &raw mut cas_expected, old_val + val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_sub_u64_ppc(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.powerpc64) {
        var old_val : u64 = *ptr
        while(true) {
            var cas_expected : u64 = old_val
            if(__chx__cas_u64_ppc(ptr, &raw mut cas_expected, old_val - val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_sub_u32_ppc(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.powerpc || def.powerpc64) {
        var old_val : u32 = *ptr
        while(true) {
            var cas_expected : u32 = old_val
            if(__chx__cas_u32_ppc(ptr, &raw mut cas_expected, old_val - val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_and_u64_ppc(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.powerpc64) {
        var old_val : u64 = *ptr
        while(true) {
            var cas_expected : u64 = old_val
            if(__chx__cas_u64_ppc(ptr, &raw mut cas_expected, old_val & val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_and_u32_ppc(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.powerpc || def.powerpc64) {
        var old_val : u32 = *ptr
        while(true) {
            var cas_expected : u32 = old_val
            if(__chx__cas_u32_ppc(ptr, &raw mut cas_expected, old_val & val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_or_u64_ppc(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.powerpc64) {
        var old_val : u64 = *ptr
        while(true) {
            var cas_expected : u64 = old_val
            if(__chx__cas_u64_ppc(ptr, &raw mut cas_expected, old_val | val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_or_u32_ppc(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.powerpc || def.powerpc64) {
        var old_val : u32 = *ptr
        while(true) {
            var cas_expected : u32 = old_val
            if(__chx__cas_u32_ppc(ptr, &raw mut cas_expected, old_val | val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_xor_u64_ppc(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.powerpc64) {
        var old_val : u64 = *ptr
        while(true) {
            var cas_expected : u64 = old_val
            if(__chx__cas_u64_ppc(ptr, &raw mut cas_expected, old_val ^ val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__fetch_xor_u32_ppc(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.powerpc || def.powerpc64) {
        var old_val : u32 = *ptr
        while(true) {
            var cas_expected : u32 = old_val
            if(__chx__cas_u32_ppc(ptr, &raw mut cas_expected, old_val ^ val)) {
                return old_val
            }
            old_val = cas_expected
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__store_u64_ppc(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.powerpc64) {
        var cas_expected : u64 = *ptr
        while(true) {
            if(__chx__cas_u64_ppc(ptr, &raw mut cas_expected, val)) {
                return val
            }
        }
        return val
    } else {
        return val
    }
}

@retained func __chx__store_u32_ppc(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.powerpc || def.powerpc64) {
        var cas_expected : u32 = *ptr
        while(true) {
            if(__chx__cas_u32_ppc(ptr, &raw mut cas_expected, val)) {
                return val
            }
        }
        return val
    } else {
        return val
    }
}

@retained func __chx__load_u64_ppc(ptr : *u64) : u64 {
    comptime if(def.powerpc64) {
        var expected : u64 = *ptr
        while(true) {
            if(__chx__cas_u64_ppc(ptr as *mut u64, &raw mut expected, expected)) {
                return expected
            }
        }
        return 0
    } else {
        return 0
    }
}

@retained func __chx__load_u32_ppc(ptr : *u32) : u32 {
    comptime if(def.powerpc || def.powerpc64) {
        var expected : u32 = *ptr
        while(true) {
            if(__chx__cas_u32_ppc(ptr as *mut u32, &raw mut expected, expected)) {
                return expected
            }
        }
        return 0
    } else {
        return 0
    }
}

// ========================
// Architecture dispatch functions
// ========================

@retained func __chx__cas_u64_dispatch(ptr : *mut u64, expected : *mut u64, desired : u64) : bool {
    comptime if(def.x86_64 || def.i386) {
        return __chx__cas_u64(ptr, expected, desired)
    } else comptime if(def.aarch64) {
        return __chx__cas_u64_aarch64(ptr, expected, desired)
    } else comptime if(def.arm) {
        return __chx__cas_u64_arm(ptr, expected, desired)
    } else comptime if(def.riscv64) {
        return __chx__cas_u64_riscv(ptr, expected, desired)
    } else comptime if(def.powerpc64) {
        return __chx__cas_u64_ppc(ptr, expected, desired)
    } else {
        return false
    }
}

@retained func __chx__cas_u32_dispatch(ptr : *mut u32, expected : *mut u32, desired : u32) : bool {
    comptime if(def.x86_64 || def.i386) {
        return __chx__cas_u32(ptr, expected, desired)
    } else comptime if(def.aarch64) {
        return __chx__cas_u32_aarch64(ptr, expected, desired)
    } else comptime if(def.arm) {
        return __chx__cas_u32_arm(ptr, expected, desired)
    } else comptime if(def.riscv32 || def.riscv64) {
        return __chx__cas_u32_riscv(ptr, expected, desired)
    } else comptime if(def.powerpc || def.powerpc64) {
        return __chx__cas_u32_ppc(ptr, expected, desired)
    } else {
        return false
    }
}

@retained func __chx__cas_u16_dispatch(ptr : *mut u16, expected : *mut u16, desired : u16) : bool {
    comptime if(def.x86_64 || def.i386) {
        return __chx__cas_u16(ptr, expected, desired)
    } else comptime if(def.aarch64) {
        return __chx__cas_u16_aarch64(ptr, expected, desired)
    } else comptime if(def.arm) {
        return __chx__cas_u16_arm(ptr, expected, desired)
    } else comptime if(def.riscv64) {
        return __chx__cas_u16_riscv(ptr, expected, desired)
    } else {
        return false
    }
}

@retained func __chx__cas_u8_dispatch(ptr : *mut u8, expected : *mut u8, desired : u8) : bool {
    comptime if(def.x86_64 || def.i386) {
        return __chx__cas_u8(ptr, expected, desired)
    } else comptime if(def.aarch64) {
        return __chx__cas_u8_aarch64(ptr, expected, desired)
    } else comptime if(def.arm) {
        return __chx__cas_u8_arm(ptr, expected, desired)
    } else comptime if(def.riscv64) {
        return __chx__cas_u8_riscv(ptr, expected, desired)
    } else {
        return false
    }
}

@retained func __chx__exchange_u64_dispatch(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__exchange_u64(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__exchange_u64_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__exchange_u64_arm(ptr, val)
    } else comptime if(def.riscv64) {
        return __chx__exchange_u64_riscv(ptr, val)
    } else comptime if(def.powerpc64) {
        return __chx__exchange_u64_ppc(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__exchange_u32_dispatch(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__exchange_u32(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__exchange_u32_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__exchange_u32_arm(ptr, val)
    } else comptime if(def.riscv32 || def.riscv64) {
        return __chx__exchange_u32_riscv(ptr, val)
    } else comptime if(def.powerpc || def.powerpc64) {
        return __chx__exchange_u32_ppc(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__exchange_u16_dispatch(ptr : *mut u16, val : u16) : u16 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__exchange_u16(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__exchange_u16_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__exchange_u16_arm(ptr, val)
    } else comptime if(def.riscv64) {
        return __chx__exchange_u16_riscv(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__exchange_u8_dispatch(ptr : *mut u8, val : u8) : u8 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__exchange_u8(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__exchange_u8_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__exchange_u8_arm(ptr, val)
    } else comptime if(def.riscv64) {
        return __chx__exchange_u8_riscv(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__fetch_add_u64_dispatch(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__fetch_add_u64(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__fetch_add_u64_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__fetch_add_u64_arm(ptr, val)
    } else comptime if(def.riscv64) {
        return __chx__fetch_add_u64_riscv(ptr, val)
    } else comptime if(def.powerpc64) {
        return __chx__fetch_add_u64_ppc(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__fetch_add_u32_dispatch(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__fetch_add_u32(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__fetch_add_u32_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__fetch_add_u32_arm(ptr, val)
    } else comptime if(def.riscv32 || def.riscv64) {
        return __chx__fetch_add_u32_riscv(ptr, val)
    } else comptime if(def.powerpc || def.powerpc64) {
        return __chx__fetch_add_u32_ppc(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__fetch_add_u16_dispatch(ptr : *mut u16, val : u16) : u16 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__fetch_add_u16(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__fetch_add_u16_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__fetch_add_u16_arm(ptr, val)
    } else comptime if(def.riscv64) {
        return __chx__fetch_add_u16_riscv(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__fetch_add_u8_dispatch(ptr : *mut u8, val : u8) : u8 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__fetch_add_u8(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__fetch_add_u8_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__fetch_add_u8_arm(ptr, val)
    } else comptime if(def.riscv64) {
        return __chx__fetch_add_u8_riscv(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__fetch_sub_u64_dispatch(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__fetch_sub_u64(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__fetch_sub_u64_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__fetch_sub_u64_arm(ptr, val)
    } else comptime if(def.riscv64) {
        return __chx__fetch_sub_u64_riscv(ptr, val)
    } else comptime if(def.powerpc64) {
        return __chx__fetch_sub_u64_ppc(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__fetch_sub_u32_dispatch(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__fetch_sub_u32(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__fetch_sub_u32_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__fetch_sub_u32_arm(ptr, val)
    } else comptime if(def.riscv32 || def.riscv64) {
        return __chx__fetch_sub_u32_riscv(ptr, val)
    } else comptime if(def.powerpc || def.powerpc64) {
        return __chx__fetch_sub_u32_ppc(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__fetch_sub_u16_dispatch(ptr : *mut u16, val : u16) : u16 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__fetch_sub_u16(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__fetch_sub_u16_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__fetch_sub_u16_arm(ptr, val)
    } else comptime if(def.riscv64) {
        return __chx__fetch_sub_u16_riscv(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__fetch_sub_u8_dispatch(ptr : *mut u8, val : u8) : u8 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__fetch_sub_u8(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__fetch_sub_u8_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__fetch_sub_u8_arm(ptr, val)
    } else comptime if(def.riscv64) {
        return __chx__fetch_sub_u8_riscv(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__fetch_and_u64_dispatch(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__fetch_and_u64(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__fetch_and_u64_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__fetch_and_u64_arm(ptr, val)
    } else comptime if(def.riscv64) {
        return __chx__fetch_and_u64_riscv(ptr, val)
    } else comptime if(def.powerpc64) {
        return __chx__fetch_and_u64_ppc(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__fetch_and_u32_dispatch(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__fetch_and_u32(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__fetch_and_u32_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__fetch_and_u32_arm(ptr, val)
    } else comptime if(def.riscv32 || def.riscv64) {
        return __chx__fetch_and_u32_riscv(ptr, val)
    } else comptime if(def.powerpc || def.powerpc64) {
        return __chx__fetch_and_u32_ppc(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__fetch_and_u16_dispatch(ptr : *mut u16, val : u16) : u16 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__fetch_and_u16(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__fetch_and_u16_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__fetch_and_u16_arm(ptr, val)
    } else comptime if(def.riscv64) {
        return __chx__fetch_and_u16_riscv(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__fetch_and_u8_dispatch(ptr : *mut u8, val : u8) : u8 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__fetch_and_u8(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__fetch_and_u8_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__fetch_and_u8_arm(ptr, val)
    } else comptime if(def.riscv64) {
        return __chx__fetch_and_u8_riscv(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__fetch_or_u64_dispatch(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__fetch_or_u64(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__fetch_or_u64_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__fetch_or_u64_arm(ptr, val)
    } else comptime if(def.riscv64) {
        return __chx__fetch_or_u64_riscv(ptr, val)
    } else comptime if(def.powerpc64) {
        return __chx__fetch_or_u64_ppc(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__fetch_or_u32_dispatch(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__fetch_or_u32(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__fetch_or_u32_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__fetch_or_u32_arm(ptr, val)
    } else comptime if(def.riscv32 || def.riscv64) {
        return __chx__fetch_or_u32_riscv(ptr, val)
    } else comptime if(def.powerpc || def.powerpc64) {
        return __chx__fetch_or_u32_ppc(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__fetch_or_u16_dispatch(ptr : *mut u16, val : u16) : u16 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__fetch_or_u16(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__fetch_or_u16_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__fetch_or_u16_arm(ptr, val)
    } else comptime if(def.riscv64) {
        return __chx__fetch_or_u16_riscv(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__fetch_or_u8_dispatch(ptr : *mut u8, val : u8) : u8 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__fetch_or_u8(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__fetch_or_u8_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__fetch_or_u8_arm(ptr, val)
    } else comptime if(def.riscv64) {
        return __chx__fetch_or_u8_riscv(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__fetch_xor_u64_dispatch(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__fetch_xor_u64(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__fetch_xor_u64_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__fetch_xor_u64_arm(ptr, val)
    } else comptime if(def.riscv64) {
        return __chx__fetch_xor_u64_riscv(ptr, val)
    } else comptime if(def.powerpc64) {
        return __chx__fetch_xor_u64_ppc(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__fetch_xor_u32_dispatch(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__fetch_xor_u32(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__fetch_xor_u32_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__fetch_xor_u32_arm(ptr, val)
    } else comptime if(def.riscv32 || def.riscv64) {
        return __chx__fetch_xor_u32_riscv(ptr, val)
    } else comptime if(def.powerpc || def.powerpc64) {
        return __chx__fetch_xor_u32_ppc(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__fetch_xor_u16_dispatch(ptr : *mut u16, val : u16) : u16 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__fetch_xor_u16(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__fetch_xor_u16_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__fetch_xor_u16_arm(ptr, val)
    } else comptime if(def.riscv64) {
        return __chx__fetch_xor_u16_riscv(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__fetch_xor_u8_dispatch(ptr : *mut u8, val : u8) : u8 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__fetch_xor_u8(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__fetch_xor_u8_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__fetch_xor_u8_arm(ptr, val)
    } else comptime if(def.riscv64) {
        return __chx__fetch_xor_u8_riscv(ptr, val)
    } else {
        return 0
    }
}

@retained func __chx__store_u64_dispatch(ptr : *mut u64, val : u64) : u64 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__store_u64(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__store_u64_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__store_u64_arm(ptr, val)
    } else comptime if(def.riscv64) {
        return __chx__store_u64_riscv(ptr, val)
    } else comptime if(def.powerpc64) {
        return __chx__store_u64_ppc(ptr, val)
    } else {
        return val
    }
}

@retained func __chx__store_u32_dispatch(ptr : *mut u32, val : u32) : u32 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__store_u32(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__store_u32_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__store_u32_arm(ptr, val)
    } else comptime if(def.riscv32 || def.riscv64) {
        return __chx__store_u32_riscv(ptr, val)
    } else comptime if(def.powerpc || def.powerpc64) {
        return __chx__store_u32_ppc(ptr, val)
    } else {
        return val
    }
}

@retained func __chx__store_u16_dispatch(ptr : *mut u16, val : u16) : u16 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__store_u16(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__store_u16_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__store_u16_arm(ptr, val)
    } else comptime if(def.riscv64) {
        return __chx__store_u16_riscv(ptr, val)
    } else {
        return val
    }
}

@retained func __chx__store_u8_dispatch(ptr : *mut u8, val : u8) : u8 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__store_u8(ptr, val)
    } else comptime if(def.aarch64) {
        return __chx__store_u8_aarch64(ptr, val)
    } else comptime if(def.arm) {
        return __chx__store_u8_arm(ptr, val)
    } else comptime if(def.riscv64) {
        return __chx__store_u8_riscv(ptr, val)
    } else {
        return val
    }
}

@retained func __chx__load_u64_dispatch(ptr : *u64) : u64 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__load_u64(ptr)
    } else comptime if(def.aarch64) {
        return __chx__load_u64_aarch64(ptr)
    } else comptime if(def.arm) {
        return __chx__load_u64_arm(ptr)
    } else comptime if(def.riscv64) {
        return __chx__load_u64_riscv(ptr)
    } else comptime if(def.powerpc64) {
        return __chx__load_u64_ppc(ptr)
    } else {
        return 0
    }
}

@retained func __chx__load_u32_dispatch(ptr : *u32) : u32 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__load_u32(ptr)
    } else comptime if(def.aarch64) {
        return __chx__load_u32_aarch64(ptr)
    } else comptime if(def.arm) {
        return __chx__load_u32_arm(ptr)
    } else comptime if(def.riscv32 || def.riscv64) {
        return __chx__load_u32_riscv(ptr)
    } else comptime if(def.powerpc || def.powerpc64) {
        return __chx__load_u32_ppc(ptr)
    } else {
        return 0
    }
}

@retained func __chx__load_u16_dispatch(ptr : *u16) : u16 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__load_u16(ptr)
    } else comptime if(def.aarch64) {
        return __chx__load_u16_aarch64(ptr)
    } else comptime if(def.arm) {
        return __chx__load_u16_arm(ptr)
    } else comptime if(def.riscv64) {
        return __chx__load_u16_riscv(ptr)
    } else {
        return 0
    }
}

@retained func __chx__load_u8_dispatch(ptr : *u8) : u8 {
    comptime if(def.x86_64 || def.i386) {
        return __chx__load_u8(ptr)
    } else comptime if(def.aarch64) {
        return __chx__load_u8_aarch64(ptr)
    } else comptime if(def.arm) {
        return __chx__load_u8_arm(ptr)
    } else comptime if(def.riscv64) {
        return __chx__load_u8_riscv(ptr)
    } else {
        return 0
    }
}

// ========================
// fence
// ========================

@retained func __chx__fence(order : int) {
    comptime if(def.x86_64 || def.i386) {
        if(order == 7) {
            asm("mfence" ::: "memory")
        } else if(order >= 4) {
            asm("" ::: "memory")
        }
    } else comptime if(def.aarch64 || def.arm) {
        if(order == 7) {
            asm("dmb ish" ::: "memory")
        } else if(order >= 4) {
            asm("" ::: "memory")
        }
    } else comptime if(def.riscv64 || def.riscv32) {
        if(order >= 4) {
            asm("fence rw, rw" ::: "memory")
        }
    } else comptime if(def.powerpc || def.powerpc64) {
        if(order == 7) {
            asm("sync" ::: "memory")
        } else if(order >= 4) {
            asm("" ::: "memory")
        }
    }
}

@retained func __chx__signal_fence(order : int) {
    if(order >= 4) {
        asm("" ::: "memory")
    }
}

public comptime func atomic_fence(order : memory_order = memory_order.seq_cst) {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            __chx__fence(llvm_mem_order(order))
        } else {
            intrinsics::llvm::atomic_fence(llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system))
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
    }
}

// ========================
// u64 functions
// ========================

public comptime func atomic_load_u64(x : %runtime<*u64>, order : memory_order = memory_order.seq_cst) : u64 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__load_u64_dispatch(x)) as u64
        } else {
            return intrinsics::llvm::atomic_load(x, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u64
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_load(x, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u64
    }
}

public comptime func atomic_store_u64(x : %runtime<*mut u64>, y : %maybe_runtime<u64>, order : memory_order = memory_order.seq_cst) : int {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__store_u64_dispatch(x, y)) as int
        } else {
            intrinsics::llvm::atomic_store(x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system))
            return 0
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        intrinsics::llvm::atomic_store(x, y, 0, scope_to_int(llvm_atomic_sync_scope.system))
        return 0
    }
}

public comptime func atomic_compare_exchange_weak_u64(x : %runtime<*mut u64>, expected : %runtime<*mut u64>, y : %maybe_runtime<u64>, success_order : memory_order = memory_order.seq_cst, failure_order : memory_order = memory_order.seq_cst) : bool {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__cas_u64_dispatch(x, expected, y)) as bool
        } else {
            return intrinsics::llvm::atomic_cmp_exch_weak(x, expected, y, llvm_mem_order(success_order), llvm_mem_order(failure_order), scope_to_int(llvm_atomic_sync_scope.system))
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(success_order))
        return intrinsics::llvm::atomic_cmp_exch_weak(x, expected, y, 0, 0, scope_to_int(llvm_atomic_sync_scope.system))
    }
}

public comptime func atomic_compare_exchange_strong_u64(x : %runtime<*mut u64>, expected : %runtime<*mut u64>, y : %maybe_runtime<u64>, success_order : memory_order = memory_order.seq_cst, failure_order : memory_order = memory_order.seq_cst) : bool {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__cas_u64_dispatch(x, expected, y)) as bool
        } else {
            return intrinsics::llvm::atomic_cmp_exch_strong(x, expected, y, llvm_mem_order(success_order), llvm_mem_order(failure_order), scope_to_int(llvm_atomic_sync_scope.system))
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(success_order))
        return intrinsics::llvm::atomic_cmp_exch_strong(x, expected, y, 0, 0, scope_to_int(llvm_atomic_sync_scope.system))
    }
}

public comptime func atomic_exchange_u64(x : %runtime<*mut u64>, y : %maybe_runtime<u64>, order : memory_order = memory_order.seq_cst) : u64 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__exchange_u64_dispatch(x, y)) as u64
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Xchg), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u64
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(0, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u64
    }
}

public comptime func atomic_fetch_add_u64(x : %runtime<*mut u64>, y : %maybe_runtime<u64>, order : memory_order = memory_order.seq_cst) : u64 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__fetch_add_u64_dispatch(x, y)) as u64
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Add), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u64
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(1, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u64
    }
}

public comptime func atomic_fetch_sub_u64(x : %runtime<*mut u64>, y : %maybe_runtime<u64>, order : memory_order = memory_order.seq_cst) : u64 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__fetch_sub_u64_dispatch(x, y)) as u64
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Sub), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u64
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(2, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u64
    }
}

public comptime func atomic_fetch_and_u64(x : %runtime<*mut u64>, y : %maybe_runtime<u64>, order : memory_order = memory_order.seq_cst) : u64 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__fetch_and_u64_dispatch(x, y)) as u64
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.And), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u64
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(3, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u64
    }
}

public comptime func atomic_fetch_or_u64(x : %runtime<*mut u64>, y : %maybe_runtime<u64>, order : memory_order = memory_order.seq_cst) : u64 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__fetch_or_u64_dispatch(x, y)) as u64
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Or), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u64
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(5, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u64
    }
}

public comptime func atomic_fetch_xor_u64(x : %runtime<*mut u64>, y : %maybe_runtime<u64>, order : memory_order = memory_order.seq_cst) : u64 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__fetch_xor_u64_dispatch(x, y)) as u64
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Xor), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u64
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(6, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u64
    }
}

// ========================
// u32 functions
// ========================

public comptime func atomic_load_u32(x : %runtime<*u32>, order : memory_order = memory_order.seq_cst) : u32 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__load_u32_dispatch(x)) as u32
        } else {
            return intrinsics::llvm::atomic_load(x, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u32
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_load(x, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u32
    }
}

public comptime func atomic_store_u32(x : %runtime<*mut u32>, y : %maybe_runtime<u32>, order : memory_order = memory_order.seq_cst) : int {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__store_u32_dispatch(x, y)) as int
        } else {
            intrinsics::llvm::atomic_store(x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system))
            return 0
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        intrinsics::llvm::atomic_store(x, y, 0, scope_to_int(llvm_atomic_sync_scope.system))
        return 0
    }
}

public comptime func atomic_compare_exchange_weak_u32(x : %runtime<*mut u32>, expected : %runtime<*mut u32>, y : %maybe_runtime<u32>, success_order : memory_order = memory_order.seq_cst, failure_order : memory_order = memory_order.seq_cst) : bool {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__cas_u32_dispatch(x, expected, y)) as bool
        } else {
            return intrinsics::llvm::atomic_cmp_exch_weak(x, expected, y, llvm_mem_order(success_order), llvm_mem_order(failure_order), scope_to_int(llvm_atomic_sync_scope.system))
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(success_order))
        return intrinsics::llvm::atomic_cmp_exch_weak(x, expected, y, 0, 0, scope_to_int(llvm_atomic_sync_scope.system))
    }
}

public comptime func atomic_compare_exchange_strong_u32(x : %runtime<*mut u32>, expected : %runtime<*mut u32>, y : %maybe_runtime<u32>, success_order : memory_order = memory_order.seq_cst, failure_order : memory_order = memory_order.seq_cst) : bool {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__cas_u32_dispatch(x, expected, y)) as bool
        } else {
            return intrinsics::llvm::atomic_cmp_exch_strong(x, expected, y, llvm_mem_order(success_order), llvm_mem_order(failure_order), scope_to_int(llvm_atomic_sync_scope.system))
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(success_order))
        return intrinsics::llvm::atomic_cmp_exch_strong(x, expected, y, 0, 0, scope_to_int(llvm_atomic_sync_scope.system))
    }
}

public comptime func atomic_exchange_u32(x : %runtime<*mut u32>, y : %maybe_runtime<u32>, order : memory_order = memory_order.seq_cst) : u32 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__exchange_u32_dispatch(x, y)) as u32
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Xchg), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u32
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(0, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u32
    }
}

public comptime func atomic_fetch_add_u32(x : %runtime<*mut u32>, y : %maybe_runtime<u32>, order : memory_order = memory_order.seq_cst) : u32 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__fetch_add_u32_dispatch(x, y)) as u32
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Add), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u32
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(1, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u32
    }
}

public comptime func atomic_fetch_sub_u32(x : %runtime<*mut u32>, y : %maybe_runtime<u32>, order : memory_order = memory_order.seq_cst) : u32 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__fetch_sub_u32_dispatch(x, y)) as u32
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Sub), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u32
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(2, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u32
    }
}

public comptime func atomic_fetch_and_u32(x : %runtime<*mut u32>, y : %maybe_runtime<u32>, order : memory_order = memory_order.seq_cst) : u32 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__fetch_and_u32_dispatch(x, y)) as u32
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.And), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u32
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(3, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u32
    }
}

public comptime func atomic_fetch_or_u32(x : %runtime<*mut u32>, y : %maybe_runtime<u32>, order : memory_order = memory_order.seq_cst) : u32 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__fetch_or_u32_dispatch(x, y)) as u32
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Or), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u32
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(5, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u32
    }
}

public comptime func atomic_fetch_xor_u32(x : %runtime<*mut u32>, y : %maybe_runtime<u32>, order : memory_order = memory_order.seq_cst) : u32 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__fetch_xor_u32_dispatch(x, y)) as u32
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Xor), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u32
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(6, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u32
    }
}

// ========================
// u16 functions
// ========================

public comptime func atomic_load_u16(x : %runtime<*u16>, order : memory_order = memory_order.seq_cst) : u16 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__load_u16_dispatch(x)) as u16
        } else {
            return intrinsics::llvm::atomic_load(x, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u16
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_load(x, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u16
    }
}

public comptime func atomic_store_u16(x : %runtime<*mut u16>, y : %maybe_runtime<u16>, order : memory_order = memory_order.seq_cst) : int {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__store_u16_dispatch(x, y)) as int
        } else {
            intrinsics::llvm::atomic_store(x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system))
            return 0
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        intrinsics::llvm::atomic_store(x, y, 0, scope_to_int(llvm_atomic_sync_scope.system))
        return 0
    }
}

public comptime func atomic_compare_exchange_weak_u16(x : %runtime<*mut u16>, expected : %runtime<*mut u16>, y : %maybe_runtime<u16>, success_order : memory_order = memory_order.seq_cst, failure_order : memory_order = memory_order.seq_cst) : bool {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__cas_u16_dispatch(x, expected, y)) as bool
        } else {
            return intrinsics::llvm::atomic_cmp_exch_weak(x, expected, y, llvm_mem_order(success_order), llvm_mem_order(failure_order), scope_to_int(llvm_atomic_sync_scope.system))
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(success_order))
        return intrinsics::llvm::atomic_cmp_exch_weak(x, expected, y, 0, 0, scope_to_int(llvm_atomic_sync_scope.system))
    }
}

public comptime func atomic_compare_exchange_strong_u16(x : %runtime<*mut u16>, expected : %runtime<*mut u16>, y : %maybe_runtime<u16>, success_order : memory_order = memory_order.seq_cst, failure_order : memory_order = memory_order.seq_cst) : bool {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__cas_u16_dispatch(x, expected, y)) as bool
        } else {
            return intrinsics::llvm::atomic_cmp_exch_strong(x, expected, y, llvm_mem_order(success_order), llvm_mem_order(failure_order), scope_to_int(llvm_atomic_sync_scope.system))
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(success_order))
        return intrinsics::llvm::atomic_cmp_exch_strong(x, expected, y, 0, 0, scope_to_int(llvm_atomic_sync_scope.system))
    }
}

public comptime func atomic_exchange_u16(x : %runtime<*mut u16>, y : %maybe_runtime<u16>, order : memory_order = memory_order.seq_cst) : u16 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__exchange_u16_dispatch(x, y)) as u16
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Xchg), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u16
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(0, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u16
    }
}

public comptime func atomic_fetch_add_u16(x : %runtime<*mut u16>, y : %maybe_runtime<u16>, order : memory_order = memory_order.seq_cst) : u16 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__fetch_add_u16_dispatch(x, y)) as u16
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Add), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u16
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(1, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u16
    }
}

public comptime func atomic_fetch_sub_u16(x : %runtime<*mut u16>, y : %maybe_runtime<u16>, order : memory_order = memory_order.seq_cst) : u16 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__fetch_sub_u16_dispatch(x, y)) as u16
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Sub), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u16
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(2, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u16
    }
}

public comptime func atomic_fetch_and_u16(x : %runtime<*mut u16>, y : %maybe_runtime<u16>, order : memory_order = memory_order.seq_cst) : u16 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__fetch_and_u16_dispatch(x, y)) as u16
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.And), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u16
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(3, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u16
    }
}

public comptime func atomic_fetch_or_u16(x : %runtime<*mut u16>, y : %maybe_runtime<u16>, order : memory_order = memory_order.seq_cst) : u16 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__fetch_or_u16_dispatch(x, y)) as u16
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Or), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u16
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(5, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u16
    }
}

public comptime func atomic_fetch_xor_u16(x : %runtime<*mut u16>, y : %maybe_runtime<u16>, order : memory_order = memory_order.seq_cst) : u16 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__fetch_xor_u16_dispatch(x, y)) as u16
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Xor), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u16
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(6, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u16
    }
}

// ========================
// u8 functions
// ========================

public comptime func atomic_load_u8(x : %runtime<*u8>, order : memory_order = memory_order.seq_cst) : u8 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__load_u8_dispatch(x)) as u8
        } else {
            return intrinsics::llvm::atomic_load(x, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u8
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_load(x, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u8
    }
}

public comptime func atomic_store_u8(x : %runtime<*mut u8>, y : %maybe_runtime<u8>, order : memory_order = memory_order.seq_cst) : int {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__store_u8_dispatch(x, y)) as int
        } else {
            intrinsics::llvm::atomic_store(x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system))
            return 0
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        intrinsics::llvm::atomic_store(x, y, 0, scope_to_int(llvm_atomic_sync_scope.system))
        return 0
    }
}

public comptime func atomic_compare_exchange_weak_u8(x : %runtime<*mut u8>, expected : %runtime<*mut u8>, y : %maybe_runtime<u8>, success_order : memory_order = memory_order.seq_cst, failure_order : memory_order = memory_order.seq_cst) : bool {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__cas_u8_dispatch(x, expected, y)) as bool
        } else {
            return intrinsics::llvm::atomic_cmp_exch_weak(x, expected, y, llvm_mem_order(success_order), llvm_mem_order(failure_order), scope_to_int(llvm_atomic_sync_scope.system))
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(success_order))
        return intrinsics::llvm::atomic_cmp_exch_weak(x, expected, y, 0, 0, scope_to_int(llvm_atomic_sync_scope.system))
    }
}

public comptime func atomic_compare_exchange_strong_u8(x : %runtime<*mut u8>, expected : %runtime<*mut u8>, y : %maybe_runtime<u8>, success_order : memory_order = memory_order.seq_cst, failure_order : memory_order = memory_order.seq_cst) : bool {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__cas_u8_dispatch(x, expected, y)) as bool
        } else {
            return intrinsics::llvm::atomic_cmp_exch_strong(x, expected, y, llvm_mem_order(success_order), llvm_mem_order(failure_order), scope_to_int(llvm_atomic_sync_scope.system))
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(success_order))
        return intrinsics::llvm::atomic_cmp_exch_strong(x, expected, y, 0, 0, scope_to_int(llvm_atomic_sync_scope.system))
    }
}

public comptime func atomic_exchange_u8(x : %runtime<*mut u8>, y : %maybe_runtime<u8>, order : memory_order = memory_order.seq_cst) : u8 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__exchange_u8_dispatch(x, y)) as u8
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Xchg), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u8
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(0, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u8
    }
}

public comptime func atomic_fetch_add_u8(x : %runtime<*mut u8>, y : %maybe_runtime<u8>, order : memory_order = memory_order.seq_cst) : u8 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__fetch_add_u8_dispatch(x, y)) as u8
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Add), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u8
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(1, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u8
    }
}

public comptime func atomic_fetch_sub_u8(x : %runtime<*mut u8>, y : %maybe_runtime<u8>, order : memory_order = memory_order.seq_cst) : u8 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__fetch_sub_u8_dispatch(x, y)) as u8
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Sub), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u8
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(2, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u8
    }
}

public comptime func atomic_fetch_and_u8(x : %runtime<*mut u8>, y : %maybe_runtime<u8>, order : memory_order = memory_order.seq_cst) : u8 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__fetch_and_u8_dispatch(x, y)) as u8
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.And), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u8
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(3, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u8
    }
}

public comptime func atomic_fetch_or_u8(x : %runtime<*mut u8>, y : %maybe_runtime<u8>, order : memory_order = memory_order.seq_cst) : u8 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__fetch_or_u8_dispatch(x, y)) as u8
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Or), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u8
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(5, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u8
    }
}

public comptime func atomic_fetch_xor_u8(x : %runtime<*mut u8>, y : %maybe_runtime<u8>, order : memory_order = memory_order.seq_cst) : u8 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__fetch_xor_u8_dispatch(x, y)) as u8
        } else {
            return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Xor), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u8
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_op(6, x, y, 0, scope_to_int(llvm_atomic_sync_scope.system)) as u8
    }
}

// ========================
// atomic_flag
// ========================

@direct_init
public struct atomic_flag {
    var _value : u32 = 0
}

@retained func __chx__flag_test_and_set_u32(ptr : *mut u32) : bool {
    comptime if(def.x86_64 || def.i386) {
        var old_val : u32 = 1
        asm("xchgl %0, %1" : "=r"(old_val), "+m"(*ptr) : "0"(old_val) : "memory")
        return old_val == 0
    } else {
        var expected : u32 = 0
        while(true) {
            if(__chx__cas_u32_dispatch(ptr, &raw mut expected, 1)) {
                return true
            }
            if(expected != 0) { return false }
        }
        return false
    }
}

@retained func __chx__flag_clear_u32(ptr : *mut u32) : int {
    __chx__store_u32_dispatch(ptr, 0)
    __chx__fence(7)
    return 0
}

public func atomic_flag_test_and_set(flag : *mut atomic_flag) : bool {
    var expected : u32 = 0
    var val_ptr = flag as *mut u32
    if(atomic_compare_exchange_strong_u32(val_ptr, &raw mut expected, 1, memory_order.seq_cst, memory_order.seq_cst)) {
        return true
    }
    return false
}

public func atomic_flag_clear(flag : *mut atomic_flag) {
    atomic_store_u32(flag as *mut u32, 0, memory_order.seq_cst)
}
