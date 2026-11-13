public struct AtomicU32 {

    internal var value: u32

    public func new(v: u32) : AtomicU32 {
        return AtomicU32 { value : v }
    }

    public comptime func load(&self, order: memory_order = memory_order.seq_cst) : u32 {
        return atomic_load_u32(&raw value, order)
    }

    public comptime func store(&self, v: %maybe_runtime<u32>, order: memory_order = memory_order.seq_cst) {
        atomic_store_u32(&raw mut value, v, order)
    }

    public comptime func exchange(&self, v: %maybe_runtime<u32>, order: memory_order = memory_order.seq_cst) : u32 {
        return atomic_exchange_u32(&raw mut value, v, order)
    }

    public comptime func compare_exchange_weak(&self, expected: %runtime<*mut u32>, v: %maybe_runtime<u32>,
        success_order: memory_order = memory_order.seq_cst,
        failure_order: memory_order = memory_order.seq_cst
    ) : bool {
        return atomic_compare_exchange_weak_u32(&raw mut value, expected, v, success_order, failure_order) != 0
    }

    public comptime func compare_exchange_strong(&self, expected: %runtime<*mut u32>, v: %maybe_runtime<u32>,
        success_order: memory_order = memory_order.seq_cst,
        failure_order: memory_order = memory_order.seq_cst
    ) : bool {
        return atomic_compare_exchange_strong_u32(&raw mut value, expected, v, success_order, failure_order) != 0
    }

    public comptime func fetch_add(&self, v: %maybe_runtime<u32>, order: memory_order = memory_order.seq_cst) : u32 {
        return atomic_fetch_add_u32(&raw mut value, v, order)
    }

    public comptime func fetch_sub(&self, v: %maybe_runtime<u32>, order: memory_order = memory_order.seq_cst) : u32 {
        return atomic_fetch_sub_u32(&raw mut value, v, order)
    }

    public comptime func fetch_and(&self, v: %maybe_runtime<u32>, order: memory_order = memory_order.seq_cst) : u32 {
        return atomic_fetch_and_u32(&raw mut value, v, order)
    }

    public comptime func fetch_or(&self, v: %maybe_runtime<u32>, order: memory_order = memory_order.seq_cst) : u32 {
        return atomic_fetch_or_u32(&raw mut value, v, order)
    }

    public comptime func fetch_xor(&self, v: %maybe_runtime<u32>, order: memory_order = memory_order.seq_cst) : u32 {
        return atomic_fetch_xor_u32(&raw mut value, v, order)
    }

}
