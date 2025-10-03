// Copyright (c) Chemical Language Foundation 2025.

#pragma once

enum class BackendAtomicMemoryOrder : int {
    NotAtomic = 0,
    Unordered = 1,
    Monotonic = 2, // Equivalent to C++'s relaxed.
    Acquire = 4,
    Release = 5,
    AcquireRelease = 6,
    SequentiallyConsistent = 7,
    Last = SequentiallyConsistent,
};

enum class BackendAtomicSyncScope : int {
    System = 0,
    SingleThread = 1,
    Last = SingleThread,
};

enum class BackendAtomicOp : int {
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
    Last = UDecWrap,
};