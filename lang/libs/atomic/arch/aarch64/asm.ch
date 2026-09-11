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
