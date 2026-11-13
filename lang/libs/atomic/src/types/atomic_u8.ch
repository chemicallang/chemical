public struct AtomicU8 {

    internal var value: u8

    public func new(v: u8) : AtomicU8 {
        return AtomicU8 { value : v }
    }

    public comptime func load(&self, order: memory_order = memory_order.seq_cst) : u8 {
        return atomic_load_u8(&raw value, order)
    }

    public comptime func store(&self, v: %maybe_runtime<u8>, order: memory_order = memory_order.seq_cst) {
        atomic_store_u8(&raw mut value, v, order)
    }

    public comptime func exchange(&self, v: %maybe_runtime<u8>, order: memory_order = memory_order.seq_cst) : u8 {
        return atomic_exchange_u8(&raw mut value, v, order)
    }

    public comptime func compare_exchange_weak(&self, expected: %runtime<*mut u8>, v: %maybe_runtime<u8>,
        success_order: memory_order = memory_order.seq_cst,
        failure_order: memory_order = memory_order.seq_cst
    ) : bool {
        return atomic_compare_exchange_weak_u8(&raw mut value, expected, v, success_order, failure_order) != 0
    }

    public comptime func compare_exchange_strong(&self, expected: %runtime<*mut u8>, v: %maybe_runtime<u8>,
        success_order: memory_order = memory_order.seq_cst,
        failure_order: memory_order = memory_order.seq_cst
    ) : bool {
        return atomic_compare_exchange_strong_u8(&raw mut value, expected, v, success_order, failure_order) != 0
    }

    public comptime func fetch_add(&self, v: %maybe_runtime<u8>, order: memory_order = memory_order.seq_cst) : u8 {
        return atomic_fetch_add_u8(&raw mut value, v, order)
    }

    public comptime func fetch_sub(&self, v: %maybe_runtime<u8>, order: memory_order = memory_order.seq_cst) : u8 {
        return atomic_fetch_sub_u8(&raw mut value, v, order)
    }

    public comptime func fetch_and(&self, v: %maybe_runtime<u8>, order: memory_order = memory_order.seq_cst) : u8 {
        return atomic_fetch_and_u8(&raw mut value, v, order)
    }

    public comptime func fetch_or(&self, v: %maybe_runtime<u8>, order: memory_order = memory_order.seq_cst) : u8 {
        return atomic_fetch_or_u8(&raw mut value, v, order)
    }

    public comptime func fetch_xor(&self, v: %maybe_runtime<u8>, order: memory_order = memory_order.seq_cst) : u8 {
        return atomic_fetch_xor_u8(&raw mut value, v, order)
    }

}
