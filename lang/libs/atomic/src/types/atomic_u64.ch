public struct AtomicU64 {

    var value: u64

    public func new(v: u64) : AtomicU64 {
        return AtomicU64 { value : v }
    }

    public comptime func load(&self, order: memory_order = memory_order.seq_cst) : u64 {
        return atomic_load_u64(&value, order)
    }

    public comptime func store(&mut self, v: %maybe_runtime<u64>, order: memory_order = memory_order.seq_cst) {
        atomic_store_u64(&mut value, v, order)
    }

    public comptime func exchange(&mut self, v: %maybe_runtime<u64>, order: memory_order = memory_order.seq_cst) : u64 {
        return atomic_exchange_u64(&mut value, v, order)
    }

    public comptime func compare_exchange_weak(&mut self, expected: %runtime<*mut u64>, v: %maybe_runtime<u64>,
        success_order: memory_order = memory_order.seq_cst,
        failure_order: memory_order = memory_order.seq_cst
    ) : bool {
        return atomic_compare_exchange_weak_u64(&mut value, expected, v, success_order, failure_order) != 0
    }

    public comptime func compare_exchange_strong(&mut self, expected: %runtime<*mut u64>, v: %maybe_runtime<u64>,
        success_order: memory_order = memory_order.seq_cst,
        failure_order: memory_order = memory_order.seq_cst
    ) : bool {
        return atomic_compare_exchange_strong_u64(&mut value, expected, v, success_order, failure_order) != 0
    }

    public comptime func fetch_add(&mut self, v: %maybe_runtime<u64>, order: memory_order = memory_order.seq_cst) : u64 {
        return atomic_fetch_add_u64(&mut value, v, order)
    }

    public comptime func fetch_sub(&mut self, v: %maybe_runtime<u64>, order: memory_order = memory_order.seq_cst) : u64 {
        return atomic_fetch_sub_u64(&mut value, v, order)
    }

    public comptime func fetch_and(&mut self, v: %maybe_runtime<u64>, order: memory_order = memory_order.seq_cst) : u64 {
        return atomic_fetch_and_u64(&mut value, v, order)
    }

    public comptime func fetch_or(&mut self, v: %maybe_runtime<u64>, order: memory_order = memory_order.seq_cst) : u64 {
        return atomic_fetch_or_u64(&mut value, v, order)
    }

    public comptime func fetch_xor(&mut self, v: %maybe_runtime<u64>, order: memory_order = memory_order.seq_cst) : u64 {
        return atomic_fetch_xor_u64(&mut value, v, order)
    }

}
