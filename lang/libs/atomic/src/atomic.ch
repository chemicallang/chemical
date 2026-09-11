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
        return false
    }
    return true
}

public func atomic_flag_clear(flag : *mut atomic_flag) {
    atomic_store_u32(flag as *mut u32, 0, memory_order.seq_cst)
}
