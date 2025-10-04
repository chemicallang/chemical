if(intrinsics::get_backend_name() == "C") {
    comptime {
        intrinsics::emit("#" + "ifdef _WIN32\n#" + "include \"" + intrinsics::get_libs_dir() + "/c/atomic/win/atomic_compat.h\"\n#" + "else\n#" + "include \"" + intrinsics::get_libs_dir() + "/c/atomic/nix/atomic.h\"\n#" + "endif")
    }
}

private comptime func signal_arg_call(name : *char, arg : any) : any {
    return intrinsics::wrap(intrinsics::multiple(intrinsics::raw(name), intrinsics::raw("("), arg, intrinsics::raw(")")))
}

private comptime func two_arg_call(name : *char, arg1 : any, arg2 : any) : any {
    return intrinsics::wrap(intrinsics::multiple(intrinsics::raw(name),intrinsics::raw("("),arg1,intrinsics::raw(", "),arg2,intrinsics::raw(")")))
}

private comptime func three_arg_call(name : *char, arg1 : any, arg2 : any, arg3 : any) : any {
    return intrinsics::wrap(intrinsics::multiple(intrinsics::raw(name),intrinsics::raw("("),arg1,intrinsics::raw(", "),arg2,intrinsics::raw(", "),arg3,intrinsics::raw(")")))
}

private comptime func four_arg_call(name : *char, arg1 : any, arg2 : any, arg3 : any, arg4 : any) : any {
    return intrinsics::wrap(intrinsics::multiple(intrinsics::raw(name),intrinsics::raw("("),arg1,intrinsics::raw(", "),arg2,intrinsics::raw(", "),arg3,intrinsics::raw(", "),arg4,intrinsics::raw(")")))
}

private comptime func five_arg_call(name : *char, arg1 : any, arg2 : any, arg3 : any, arg4 : any, arg5 : any) : any {
    return intrinsics::wrap(intrinsics::multiple(intrinsics::raw(name),intrinsics::raw("("),arg1,intrinsics::raw(", "),arg2,intrinsics::raw(", "),arg3,intrinsics::raw(", "),arg4,intrinsics::raw(", "),arg5,intrinsics::raw(")")))
}

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
        llvm_atomic_memory_order.sequentially_consistent => { return 7; }
    }
}

comptime func convert_mem_order_to_llvm(order : memory_order) : llvm_atomic_memory_order {
    switch(order) {
        memory_order.relaxed => { return llvm_atomic_memory_order.monotonic; }
        memory_order.consume => { return llvm_atomic_memory_order.acquire; }
        memory_order.acquire => { return llvm_atomic_memory_order.acquire; }
        memory_order.release => { return llvm_atomic_memory_order.release; }
        memory_order.acq_rel => { return llvm_atomic_memory_order.acquire_release; }
        memory_order.seq_cst => { return llvm_atomic_memory_order.sequentially_consistent; }
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
        llvm_atomic_sync_scope.system => { return 0; }
        llvm_atomic_sync_scope.single_thread => { return 1; }
    }
}

enum llvm_atomic_op : int {
    Xchg = 0,
    /// *p = old + v
    Add = 1,
    /// *p = old - v
    Sub = 2,
    /// *p = old & v
    And = 3,
    /// *p = ~(old & v)
    Nand = 4,
    /// *p = old | v
    Or = 5,
    /// *p = old ^ v
    Xor = 6,
    /// *p = old >signed v ? old : v
    Max = 7,
    /// *p = old <signed v ? old : v
    Min = 8,
    /// *p = old >unsigned v ? old : v
    UMax = 9,
    /// *p = old <unsigned v ? old : v
    UMin = 10,
    /// *p = old + v
    FAdd = 11,
    /// *p = old - v
    FSub = 12,
    /// *p = maxnum(old, v)
    /// \p maxnum matches the behavior of \p llvm.maxnum.*.
    FMax = 13,
    /// *p = minnum(old, v)
    /// \p minnum matches the behavior of \p llvm.minnum.*.
    FMin = 14,
    /// Increment one up to a maximum value.
    /// *p = (old u>= v) ? 0 : (old + 1)
    UIncWrap = 15,
    /// Decrement one until a minimum value or zero.
    /// *p = ((old == 0) || (old u> v)) ? v : (old - 1)
    UDecWrap = 16,
};

comptime func llvm_atomic_op_to_int(op : llvm_atomic_op) : int {
    switch(op) {
        llvm_atomic_op.Xchg => { return 0; }
        llvm_atomic_op.Add => { return 1; }
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

// ------- u64 functions ----------

public comptime func atomic_load_u64(x : *u64, order : memory_order = memory_order.seq_cst) : u64 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned long long atomic_load_u64(unsigned long long* x)
        return two_arg_call("atomic_load_u64_explicit", x, order) as u64
    } else {
        return intrinsics::llvm::atomic_load(x, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u64
    }
}

public comptime func atomic_store_u64(x : *mut u64, y : u64, order : memory_order = memory_order.seq_cst) {
    if(intrinsics::get_backend_name() == "C") {
        // void atomic_store_u64(unsigned long long* x, unsigned long long y)
        return three_arg_call("atomic_store_u64_explicit", x, y, order) as void
    } else {
        return intrinsics::llvm::atomic_store(x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system))
    }
}

public comptime func atomic_compare_exchange_weak_u64(x : *mut u64, expected : *mut u64, y : u64, success_order : memory_order = memory_order.seq_cst, failure_order : memory_order = memory_order.seq_cst) : int {
    if(intrinsics::get_backend_name() == "C") {
        // int atomic_compare_exchange_weak_u64(unsigned long long* x, unsigned long long* expected, unsigned long long y)
        return five_arg_call("atomic_compare_exchange_weak_u64_explicit", x, expected, y, success_order, failure_order) as int
    } else {
        return intrinsics::llvm::atomic_cmp_exch_weak(x, expected, y, llvm_mem_order(success_order), llvm_mem_order(failure_order), scope_to_int(llvm_atomic_sync_scope.system)) as int
    }
}

public comptime func atomic_compare_exchange_strong_u64(x : *mut u64, expected : *mut u64, y : u64, success_order : memory_order = memory_order.seq_cst, failure_order : memory_order = memory_order.seq_cst) : int {
    if(intrinsics::get_backend_name() == "C") {
        // int atomic_compare_exchange_strong_u64(unsigned long long* x, unsigned long long* expected, unsigned long long y)
        return five_arg_call("atomic_compare_exchange_strong_u64_explicit", x, expected, y, success_order, failure_order) as int
    } else {
        return intrinsics::llvm::atomic_cmp_exch_strong(x, expected, y, llvm_mem_order(success_order), llvm_mem_order(failure_order), scope_to_int(llvm_atomic_sync_scope.system)) as int
    }
}

public comptime func atomic_exchange_u64(x : *mut u64, y : u64, order : memory_order = memory_order.seq_cst) : u64 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned long long atomic_exchange_u64(unsigned long long* x, unsigned long long y)
        return three_arg_call("atomic_exchange_u64_explicit", x, y, order) as u64
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Xchg), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u64
    }
}

public comptime func atomic_fetch_add_u64(x : *mut u64, y : u64, order : memory_order = memory_order.seq_cst) : u64 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned long long atomic_fetch_add_u64(unsigned long long* x, unsigned long long y)
        return three_arg_call("atomic_fetch_add_u64_explicit", x, y, order) as u64
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Add), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u64
    }
}

public comptime func atomic_fetch_sub_u64(x : *mut u64, y : u64, order : memory_order = memory_order.seq_cst) : u64 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned long long atomic_fetch_sub_u64(unsigned long long* x, unsigned long long y)
        return three_arg_call("atomic_fetch_sub_u64_explicit", x, y, order) as u64
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Sub), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u64
    }
}

public comptime func atomic_fetch_and_u64(x : *mut u64, y : u64, order : memory_order = memory_order.seq_cst) : u64 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned long long atomic_fetch_and_u64(unsigned long long* x, unsigned long long y)
        return three_arg_call("atomic_fetch_and_u64_explicit", x, y, order) as u64
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.And), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u64
    }
}

public comptime func atomic_fetch_or_u64(x : *mut u64, y : u64, order : memory_order = memory_order.seq_cst) : u64 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned long long atomic_fetch_or_u64(unsigned long long* x, unsigned long long y)
        return three_arg_call("atomic_fetch_or_u64_explicit", x, y, order) as u64
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Or), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u64
    }
}

public comptime func atomic_fetch_xor_u64(x : *mut u64, y : u64, order : memory_order = memory_order.seq_cst) : u64 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned long long atomic_fetch_xor_u64(unsigned long long* x, unsigned long long y)
        return three_arg_call("atomic_fetch_xor_u64_explicit", x, y, order) as u64
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Xor), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u64
    }
}

// ------- u32 functions ----------

public comptime func atomic_load_u32(x : *u32, order : memory_order = memory_order.seq_cst) : u32 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned atomic_load_u32(unsigned* x)
        return two_arg_call("atomic_load_u32_explicit", x, order) as u32
    } else {
        return intrinsics::llvm::atomic_load(x, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u32
    }
}

public comptime func atomic_store_u32(x : *mut u32, y : u32, order : memory_order = memory_order.seq_cst) {
    if(intrinsics::get_backend_name() == "C") {
        // void atomic_store_u32(unsigned* x, unsigned y)
        return three_arg_call("atomic_store_u32_explicit", x, y, order) as void
    } else {
        return intrinsics::llvm::atomic_store(x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system))
    }
}

public comptime func atomic_compare_exchange_weak_u32(x : *mut u32, expected : *mut u32, y : u32, success_order : memory_order = memory_order.seq_cst, failure_order : memory_order = memory_order.seq_cst) : int {
    if(intrinsics::get_backend_name() == "C") {
        // int atomic_compare_exchange_weak_u32(unsigned* x, unsigned* expected, unsigned y)
        return five_arg_call("atomic_compare_exchange_weak_u32_explicit", x, expected, y, success_order, failure_order) as int
    } else {
        return intrinsics::llvm::atomic_cmp_exch_weak(x, expected, y, llvm_mem_order(success_order), llvm_mem_order(failure_order), scope_to_int(llvm_atomic_sync_scope.system)) as int
    }
}

public comptime func atomic_compare_exchange_strong_u32(x : *mut u32, expected : *mut u32, y : u32, success_order : memory_order = memory_order.seq_cst, failure_order : memory_order = memory_order.seq_cst) : int {
    if(intrinsics::get_backend_name() == "C") {
        // int atomic_compare_exchange_strong_u32(unsigned* x, unsigned* expected, unsigned y)
        return five_arg_call("atomic_compare_exchange_strong_u32_explicit", x, expected, y, success_order, failure_order) as int
    } else {
        return intrinsics::llvm::atomic_cmp_exch_strong(x, expected, y, llvm_mem_order(success_order), llvm_mem_order(failure_order), scope_to_int(llvm_atomic_sync_scope.system)) as int
    }
}

public comptime func atomic_exchange_u32(x : *mut u32, y : u32, order : memory_order = memory_order.seq_cst) : u32 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned atomic_exchange_u32(unsigned* x, unsigned y)
        return three_arg_call("atomic_exchange_u32_explicit", x, y, order) as u32
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Xchg), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u32
    }
}

public comptime func atomic_fetch_add_u32(x : *mut u32, y : u32, order : memory_order = memory_order.seq_cst) : u32 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned atomic_fetch_add_u32(unsigned* x, unsigned y)
        return three_arg_call("atomic_fetch_add_u32_explicit", x, y, order) as u32
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Add), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u32
    }
}

public comptime func atomic_fetch_sub_u32(x : *mut u32, y : u32, order : memory_order = memory_order.seq_cst) : u32 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned atomic_fetch_sub_u32(unsigned* x, unsigned y)
        return three_arg_call("atomic_fetch_sub_u32_explicit", x, y, order) as u32
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Sub), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u32
    }
}

public comptime func atomic_fetch_and_u32(x : *mut u32, y : u32, order : memory_order = memory_order.seq_cst) : u32 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned atomic_fetch_and_u32(unsigned* x, unsigned y)
        return three_arg_call("atomic_fetch_and_u32_explicit", x, y, order) as u32
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.And), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u32
    }
}

public comptime func atomic_fetch_or_u32(x : *mut u32, y : u32, order : memory_order = memory_order.seq_cst) : u32 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned atomic_fetch_or_u32(unsigned* x, unsigned y)
        return three_arg_call("atomic_fetch_or_u32_explicit", x, y, order) as u32
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Or), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u32
    }
}

public comptime func atomic_fetch_xor_u32(x : *mut u32, y : u32, order : memory_order = memory_order.seq_cst) : u32 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned atomic_fetch_xor_u32(unsigned* x, unsigned y)
        return three_arg_call("atomic_fetch_xor_u32_explicit", x, y, order) as u32
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Xor), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u32
    }
}

// ------- u16 functions ----------

public comptime func atomic_load_u16(x : *u16, order : memory_order = memory_order.seq_cst) : u16 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned short atomic_load_u16(unsigned short* x)
        return two_arg_call("atomic_load_u16_explicit", x, order) as u16
    } else {
        return intrinsics::llvm::atomic_load(x, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u16
    }
}

public comptime func atomic_store_u16(x : *mut u16, y : u16, order : memory_order = memory_order.seq_cst) {
    if(intrinsics::get_backend_name() == "C") {
        // void atomic_store_u16(void* x, unsigned short y)
        return three_arg_call("atomic_store_u16_explicit", x, y, order) as void
    } else {
        return intrinsics::llvm::atomic_store(x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system))
    }
}

public comptime func atomic_compare_exchange_weak_u16(x : *mut u16, expected : *mut u16, y : u16, success_order : memory_order = memory_order.seq_cst, failure_order : memory_order = memory_order.seq_cst) : int {
    if(intrinsics::get_backend_name() == "C") {
        // int atomic_compare_exchange_weak_u16(void* x, unsigned short* expected, unsigned short y)
        return five_arg_call("atomic_compare_exchange_weak_u16_explicit", x, expected, y, success_order, failure_order) as int
    } else {
        return intrinsics::llvm::atomic_cmp_exch_weak(x, expected, y, llvm_mem_order(success_order), llvm_mem_order(failure_order), scope_to_int(llvm_atomic_sync_scope.system)) as int
    }
}

public comptime func atomic_compare_exchange_strong_u16(x : *mut u16, expected : *mut u16, y : u16, success_order : memory_order = memory_order.seq_cst, failure_order : memory_order = memory_order.seq_cst) : int {
    if(intrinsics::get_backend_name() == "C") {
        // int atomic_compare_exchange_strong_u16(unsigned short* x, unsigned short* expected, unsigned short y)
        return five_arg_call("atomic_compare_exchange_strong_u16_explicit", x, expected, y, success_order, failure_order) as int
    } else {
        return intrinsics::llvm::atomic_cmp_exch_strong(x, expected, y, llvm_mem_order(success_order), llvm_mem_order(failure_order), scope_to_int(llvm_atomic_sync_scope.system)) as int
    }
}

public comptime func atomic_exchange_u16(x : *mut u16, y : u16, order : memory_order = memory_order.seq_cst) : u16 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned short atomic_exchange_u16(unsigned short* x, unsigned short y)
        return three_arg_call("atomic_exchange_u16_explicit", x, y, order) as u16
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Xchg), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u16
    }
}

public comptime func atomic_fetch_add_u16(x : *mut u16, y : u16, order : memory_order = memory_order.seq_cst) : u16 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned short atomic_fetch_add_u16(unsigned short* x, unsigned short y)
        return three_arg_call("atomic_fetch_add_u16_explicit", x, y, order) as u16
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Add), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u16
    }
}

public comptime func atomic_fetch_sub_u16(x : *mut u16, y : u16, order : memory_order = memory_order.seq_cst) : u16 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned short atomic_fetch_sub_u16(unsigned short* x, unsigned short y)
        return three_arg_call("atomic_fetch_sub_u16_explicit", x, y, order) as u16
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Sub), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u16
    }
}

public comptime func atomic_fetch_and_u16(x : *mut u16, y : u16, order : memory_order = memory_order.seq_cst) : u16 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned short atomic_fetch_and_u16(unsigned short* x, unsigned short y)
        return three_arg_call("atomic_fetch_and_u16_explicit", x, y, order) as u16
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.And), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u16
    }
}

public comptime func atomic_fetch_or_u16(x : *mut u16, y : u16, order : memory_order = memory_order.seq_cst) : u16 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned short atomic_fetch_or_u16(unsigned short* x, unsigned short y)
        return three_arg_call("atomic_fetch_or_u16_explicit", x, y, order) as u16
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Or), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u16
    }
}

public comptime func atomic_fetch_xor_u16(x : *mut u16, y : u16, order : memory_order = memory_order.seq_cst) : u16 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned short atomic_fetch_xor_u16(unsigned short* x, unsigned short y)
        return three_arg_call("atomic_fetch_xor_u16_explicit", x, y, order) as u16
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Xor), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u16
    }
}


// ------- u8 functions ----------


public comptime func atomic_load_u8(x : *u8, order : memory_order = memory_order.seq_cst) : u8 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned char atomic_load_byte(unsigned char* x);
        return two_arg_call("atomic_load_byte_explicit", x, order) as u8
    } else {
        return intrinsics::llvm::atomic_load(x, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u8
    }
}

public comptime func atomic_store_u8(x : *mut u8, y : u8, order : memory_order = memory_order.seq_cst) {
    if(intrinsics::get_backend_name() == "C") {
        // void atomic_store_byte(unsigned char* x, unsigned char y);
        return three_arg_call("atomic_store_byte_explicit", x, y, order) as void
    } else {
        return intrinsics::llvm::atomic_store(x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system))
    }
}

public comptime func atomic_compare_exchange_weak_u8(x : *mut u8, expected : *mut u8, y : u8, success_order : memory_order = memory_order.seq_cst, failure_order : memory_order = memory_order.seq_cst) : int {
    if(intrinsics::get_backend_name() == "C") {
        // int atomic_compare_exchange_weak_byte(unsigned char* x, unsigned char* expected, unsigned char y);
        return five_arg_call("atomic_compare_exchange_weak_byte_explicit", x, expected, y, success_order, failure_order) as int
    } else {
        return intrinsics::llvm::atomic_cmp_exch_weak(x, expected, y, llvm_mem_order(success_order), llvm_mem_order(failure_order), scope_to_int(llvm_atomic_sync_scope.system)) as int
    }
}

public comptime func atomic_compare_exchange_strong_u8(x : *mut u8, expected : *mut u8, y : u8, success_order : memory_order = memory_order.seq_cst, failure_order : memory_order = memory_order.seq_cst) : int {
    if(intrinsics::get_backend_name() == "C") {
        // int atomic_compare_exchange_strong_byte(unsigned char* x, unsigned char* expected, unsigned char y);
        return five_arg_call("atomic_compare_exchange_strong_byte_explicit", x, expected, y, success_order, failure_order) as int
    } else {
        return intrinsics::llvm::atomic_cmp_exch_strong(x, expected, y, llvm_mem_order(success_order), llvm_mem_order(failure_order), scope_to_int(llvm_atomic_sync_scope.system)) as int
    }
}

public comptime func atomic_exchange_u8(x : *mut u8, y : u8, order : memory_order = memory_order.seq_cst) : u8 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned char atomic_exchange_byte(unsigned char* x, unsigned char y);
        return three_arg_call("atomic_exchange_byte_explicit", x, y, order) as u8
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Xchg), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u8
    }
}

public comptime func atomic_fetch_add_u8(x : *mut u8, y : u8, order : memory_order = memory_order.seq_cst) : u8 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned char atomic_fetch_add_byte(unsigned char* x, unsigned char y);
        return three_arg_call("atomic_fetch_add_byte_explicit", x, y, order) as u8
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Add), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u8
    }
}

public comptime func atomic_fetch_sub_u8(x : *mut u8, y : u8, order : memory_order = memory_order.seq_cst) : u8 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned char atomic_fetch_sub_byte(unsigned char* x, unsigned char y);
        return three_arg_call("atomic_fetch_sub_byte_explicit", x, y, order) as u8
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Sub), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u8
    }
}

public comptime func atomic_fetch_and_u8(x : *mut u8, y : u8, order : memory_order = memory_order.seq_cst) : u8 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned char atomic_fetch_and_byte(unsigned char* x, unsigned char y);
        return three_arg_call("atomic_fetch_and_byte_explicit", x, y, order) as u8
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.And), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u8
    }
}

public comptime func atomic_fetch_or_u8(x : *mut u8, y : u8, order : memory_order = memory_order.seq_cst) : u8 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned char atomic_fetch_or_byte(unsigned char* x, unsigned char y);
        return three_arg_call("atomic_fetch_or_byte_explicit", x, y, order) as u8
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Or), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u8
    }
}

public comptime func atomic_fetch_xor_u8(x : *mut u8, y : u8, order : memory_order = memory_order.seq_cst) : u8 {
    if(intrinsics::get_backend_name() == "C") {
        // unsigned char atomic_fetch_xor_byte(unsigned char* x, unsigned char y);
        return three_arg_call("atomic_fetch_xor_byte_explicit", x, y, order) as u8
    } else {
        return intrinsics::llvm::atomic_op(llvm_atomic_op_to_int(llvm_atomic_op.Xor), x, y, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u8
    }
}