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
