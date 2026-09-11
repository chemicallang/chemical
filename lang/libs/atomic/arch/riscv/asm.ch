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
