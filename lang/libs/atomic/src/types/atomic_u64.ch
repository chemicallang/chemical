// public struct AtomicU64 {

//     var value: u64

//     public func new(v: u64) : AtomicU64 {
//         return AtomicU64 { value : v }
//     }

//     public func load(&self, order: memory_order = memory_order.seq_cst) : u64 {
//         return atomic_load_u64(&value, order)
//     }

//     // public func store(&self, v: u64, order: memory_order = memory_order.seq_cst) {
//     //     atomic_store_u64(&self.value, v, order)
//     // }

//     // public func exchange(&self, v: u64, order: memory_order = memory_order.seq_cst) : u64 {
//     //     return atomic_exchange_u64(&self.value, v, order)
//     // }

//     // public func compare_exchange_weak(&self, expected: *mut u64, v: u64,
//     //     success_order: memory_order = memory_order.seq_cst,
//     //     failure_order: memory_order = memory_order.seq_cst
//     // ) : bool {
//     //     return atomic_compare_exchange_weak_u64(&self.value, expected, v, success_order, failure_order) != 0
//     // }

//     // public func compare_exchange_strong(&self, expected: *mut u64, v: u64,
//     //     success_order: memory_order = memory_order.seq_cst,
//     //     failure_order: memory_order = memory_order.seq_cst
//     // ) : bool {
//     //     return atomic_compare_exchange_strong_u64(&self.value, expected, v, success_order, failure_order) != 0
//     // }

//     // public func fetch_add(&self, v: u64, order: memory_order = memory_order.seq_cst) : u64 {
//     //     return atomic_fetch_add_u64(&self.value, v, order)
//     // }

//     // public func fetch_sub(&self, v: u64, order: memory_order = memory_order.seq_cst) : u64 {
//     //     return atomic_fetch_sub_u64(&self.value, v, order)
//     // }

//     // public func fetch_and(&self, v: u64, order: memory_order = memory_order.seq_cst) : u64 {
//     //     return atomic_fetch_and_u64(&self.value, v, order)
//     // }

//     // public func fetch_or(&self, v: u64, order: memory_order = memory_order.seq_cst) : u64 {
//     //     return atomic_fetch_or_u64(&self.value, v, order)
//     // }

//     // public func fetch_xor(&self, v: u64, order: memory_order = memory_order.seq_cst) : u64 {
//     //     return atomic_fetch_xor_u64(&self.value, v, order)
//     // }

// }
