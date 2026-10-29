// Trivially copyable marker — structs that can be memcpy'd without
// running a custom copy-constructor.
public interface Copy {}

// Auto traits: a type is Send when its values may move to another thread, and
// Sync when shared references to it may be used from another thread. These are
// derived structurally from fields (raw pointers are neither).
public interface Send {}
public interface Sync {}
// Marker for types whose all-zero bit pattern is a valid value. Implemented
// only with an explicit `unsafe impl Zeroable`; the compiler derives it
// structurally for primitive/pointer/array/all-Zeroable struct types.
public unsafe interface Zeroable {}

// Internal audited seed for explicit unchecked zero-initialization. Safe code
// must use `zeroed<T>()` with `T: Zeroable`; library code that needs an
// uninitialized temporary should use `MaybeUninit` or `std::mem.zeroed_unchecked`.
public unsafe func <T> zeroed_unchecked_raw() : T {
    return zeroed:unsafe<T>()
}
