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
