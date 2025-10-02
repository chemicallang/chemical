// Copyright (c) Chemical Language Foundation 2025.

#ifndef COMPAT_ATOMICS_WIN32_STDATOMIC_H

#include "atomic.h"

/* Helper: small helper to treat order values consistently.
   We use atomic_thread_fence(order) already defined earlier. */
static inline void _atomic_fence_before_store(int order) {
    switch (order) {
        case memory_order_relaxed:
            /* no fence before relaxed store */
            break;
        case memory_order_release:
        case memory_order_acq_rel:
            atomic_thread_fence(memory_order_release);
            break;
        case memory_order_seq_cst:
            atomic_thread_fence(memory_order_seq_cst);
            break;
        default:
            /* conservative fallback */
            atomic_thread_fence(memory_order_seq_cst);
            break;
    }
}

static inline void _atomic_fence_after_store(int order) {
    switch (order) {
        case memory_order_relaxed:
            /* no fence after relaxed store */
            break;
        case memory_order_seq_cst:
            atomic_thread_fence(memory_order_seq_cst);
            break;
        default:
            /* release/acq_rel don't need post-store fence on many targets */
            break;
    }
}

static inline void _atomic_fence_before_load(int order) {
    switch (order) {
        case memory_order_seq_cst:
            atomic_thread_fence(memory_order_seq_cst);
            break;
        default:
            /* no fence before load for relaxed/acquire (acquire is after load) */
            break;
    }
}

static inline void _atomic_fence_after_load(int order) {
    switch (order) {
        case memory_order_acquire:
        case memory_order_consume:
        case memory_order_acq_rel:
            atomic_thread_fence(memory_order_acquire);
            break;
        case memory_order_seq_cst:
            atomic_thread_fence(memory_order_seq_cst);
            break;
        default:
            /* relaxed: nothing */
            break;
    }
}

/* ---------------- u64 ---------------- */

static inline void atomic_store_u64_explicit(unsigned long long volatile *object,
                                             unsigned long long desired, int order)
{
    /* For non-Interlocked store path: honour relaxed/release/seq_cst */
    _atomic_fence_before_store(order);
    /* perform the store */
    *(object) = (desired);
    _atomic_fence_after_store(order);
}

static inline unsigned long long atomic_load_u64_explicit(unsigned long long volatile *object,
                                                          int order)
{
    unsigned long long val;
    _atomic_fence_before_load(order);
    val = *(object);
    _atomic_fence_after_load(order);
    return val;
}

static inline unsigned long long atomic_exchange_u64_explicit(LONGLONG volatile *object,
                                                              unsigned long long desired, int order)
{
    /* InterlockedExchange64 is a full barrier; we still insert fences to match requested ordering
       as closely as possible for side effects before/after the call. */
    _atomic_fence_before_store(order);
    LONGLONG old = InterlockedExchange64(object, (LONGLONG)desired);
    _atomic_fence_after_store(order);
    return (unsigned long long)old;
}

static inline int atomic_compare_exchange_strong_u64_explicit(unsigned long long volatile *object,
                                                              unsigned long long volatile *expected,
                                                              unsigned long long desired, int order)
{
    unsigned long long old = *expected;
    /* InterlockedCompareExchange64 is a full barrier */
    LONGLONG res = InterlockedCompareExchange64((LONGLONG volatile *)object, (LONGLONG)desired, (LONGLONG)old);
    *expected = (unsigned long long)res;
    /* apply fences: success or failure use same order here */
    if (*expected == old) {
        /* success semantics */
        _atomic_fence_after_load(order); /* ensure acquire semantics if requested */
    } else {
        /* failure semantics: conservative fence for seq_cst if asked */
        if (order == memory_order_seq_cst)
            atomic_thread_fence(memory_order_seq_cst);
    }
    return *expected == old;
}

static inline int atomic_compare_exchange_weak_u64_explicit(unsigned long long volatile *object,
                                                            unsigned long long volatile *expected,
                                                            unsigned long long desired, int order)
{
    /* Implement weak as strong here (x86 does not provide spurious failures guarantee easily) */
    return atomic_compare_exchange_strong_u64_explicit(object, expected, desired, order);
}

#if defined(_WIN64)
static inline unsigned long long atomic_fetch_add_u64_explicit(LONGLONG volatile *object, unsigned long long operand, int order) {
    /* InterlockedExchangeAdd64 is full fence */
    _atomic_fence_before_load(order);
    LONGLONG old = InterlockedExchangeAdd64(object, (LONGLONG)operand);
    _atomic_fence_after_load(order);
    return (unsigned long long)old;
}
static inline unsigned long long atomic_fetch_sub_u64_explicit(LONGLONG volatile *object, unsigned long long operand, int order) {
    _atomic_fence_before_load(order);
    LONGLONG old = InterlockedExchangeAdd64(object, -(LONGLONG)operand);
    _atomic_fence_after_load(order);
    return (unsigned long long)old;
}
static inline unsigned long long atomic_fetch_or_u64_explicit(LONGLONG volatile *object, unsigned long long operand, int order) {
    _atomic_fence_before_load(order);
    LONGLONG old = InterlockedOr64(object, (LONGLONG)operand);
    _atomic_fence_after_load(order);
    return (unsigned long long)old;
}
static inline unsigned long long atomic_fetch_xor_u64_explicit(LONGLONG volatile *object, unsigned long long operand, int order) {
    _atomic_fence_before_load(order);
    LONGLONG old = InterlockedXor64(object, (LONGLONG)operand);
    _atomic_fence_after_load(order);
    return (unsigned long long)old;
}
static inline unsigned long long atomic_fetch_and_u64_explicit(LONGLONG volatile *object, unsigned long long operand, int order) {
    _atomic_fence_before_load(order);
    LONGLONG old = InterlockedAnd64(object, (LONGLONG)operand);
    _atomic_fence_after_load(order);
    return (unsigned long long)old;
}
#else
/* If building 32-bit, the existing macros/impl remain but expose explicit wrappers if needed.
   You can adapt these to use ManualInterlocked... helpers. */
static inline unsigned long long atomic_fetch_add_u64_explicit(volatile long long *object, unsigned long long operand, int order) {
    (void)order;
    return (unsigned long long)InterlockedExchangeAdd64((LONGLONG volatile *)object, (LONGLONG)operand);
}
static inline unsigned long long atomic_fetch_sub_u64_explicit(volatile long long *object, unsigned long long operand, int order) {
    (void)order;
    return (unsigned long long)InterlockedExchangeAdd64((LONGLONG volatile *)object, -(LONGLONG)operand);
}
static inline unsigned long long atomic_fetch_or_u64_explicit(volatile long long *object, unsigned long long operand, int order) {
    (void)order;
    return (unsigned long long)InterlockedOr64((LONGLONG volatile *)object, (LONGLONG)operand);
}
static inline unsigned long long atomic_fetch_xor_u64_explicit(volatile long long *object, unsigned long long operand, int order) {
    (void)order;
    return (unsigned long long)InterlockedXor64((LONGLONG volatile *)object, (LONGLONG)operand);
}
static inline unsigned long long atomic_fetch_and_u64_explicit(volatile long long *object, unsigned long long operand, int order) {
    (void)order;
    return (unsigned long long)InterlockedAnd64((LONGLONG volatile *)object, (LONGLONG)operand);
}
#endif

/* ---------------- u32 ---------------- */

/* store */
static inline void atomic_store_u32_explicit(unsigned volatile *object, unsigned desired, int order)
{
    _atomic_fence_before_store(order);
    *(object) = (desired);
    _atomic_fence_after_store(order);
}

/* load */
static inline unsigned atomic_load_u32_explicit(unsigned volatile *object, int order)
{
    unsigned val;
    _atomic_fence_before_load(order);
    val = *(object);
    _atomic_fence_after_load(order);
    return val;
}

/* exchange */
static inline unsigned atomic_exchange_u32_explicit(long volatile *object, unsigned desired, int order)
{
    /* InterlockedExchange acts as a full fence; add requested fences around call */
    _atomic_fence_before_store(order);
    LONG old = InterlockedExchange(object, (LONG)desired);
    _atomic_fence_after_store(order);
    return (unsigned)old;
}

/* compare-exchange (strong) */
static inline int atomic_compare_exchange_strong_u32_explicit(unsigned volatile *object,
                                                              unsigned volatile *expected,
                                                              unsigned desired, int order)
{
    unsigned old = *expected;
    LONG res = (LONG)InterlockedCompareExchange((LONG volatile *)object, (LONG)desired, (LONG)old);
    *expected = (unsigned)res;
    if (*expected == old) {
        /* success: ensure acquire semantics if requested */
        _atomic_fence_after_load(order);
    } else {
        /* failure: seq_cst fence if requested */
        if (order == memory_order_seq_cst)
            atomic_thread_fence(memory_order_seq_cst);
    }
    return *expected == old;
}

/* weak implemented as strong (x86 does not provide spurious failure easily) */
static inline int atomic_compare_exchange_weak_u32_explicit(unsigned volatile *object,
                                                            unsigned volatile *expected,
                                                            unsigned desired, int order)
{
    return atomic_compare_exchange_strong_u32_explicit(object, expected, desired, order);
}

/* fetch ops */
static inline unsigned atomic_fetch_add_u32_explicit(long volatile *object, unsigned operand, int order) {
    _atomic_fence_before_load(order);
    LONG old = InterlockedExchangeAdd(object, (LONG)operand);
    _atomic_fence_after_load(order);
    return (unsigned)old;
}
static inline unsigned atomic_fetch_sub_u32_explicit(long volatile *object, unsigned operand, int order) {
    _atomic_fence_before_load(order);
    LONG old = InterlockedExchangeAdd(object, -(LONG)operand);
    _atomic_fence_after_load(order);
    return (unsigned)old;
}
static inline unsigned atomic_fetch_or_u32_explicit(long volatile *object, unsigned operand, int order) {
    _atomic_fence_before_load(order);
    LONG old = InterlockedOr(object, (LONG)operand);
    _atomic_fence_after_load(order);
    return (unsigned)old;
}
static inline unsigned atomic_fetch_xor_u32_explicit(long volatile *object, unsigned operand, int order) {
    _atomic_fence_before_load(order);
    LONG old = InterlockedXor(object, (LONG)operand);
    _atomic_fence_after_load(order);
    return (unsigned)old;
}
static inline unsigned atomic_fetch_and_u32_explicit(long volatile *object, unsigned operand, int order) {
    _atomic_fence_before_load(order);
    LONG old = InterlockedAnd(object, (LONG)operand);
    _atomic_fence_after_load(order);
    return (unsigned)old;
}

/* ---------------- u16 ---------------- */

static inline void atomic_store_u16_explicit(unsigned short volatile *object, unsigned short desired, int order)
{
    _atomic_fence_before_store(order);
    *(object) = (desired);
    _atomic_fence_after_store(order);
}

static inline unsigned short atomic_load_u16_explicit(unsigned short volatile *object, int order)
{
    unsigned short val;
    _atomic_fence_before_load(order);
    val = *(object);
    _atomic_fence_after_load(order);
    return val;
}

static inline unsigned short atomic_exchange_u16_explicit(unsigned short volatile *object, unsigned short desired, int order)
{
    _atomic_fence_before_store(order);
    unsigned short old = InterlockedExchange16((volatile short *)object, (short)desired);
    _atomic_fence_after_store(order);
    return old;
}

static inline int atomic_compare_exchange_strong_u16_explicit(short volatile *object,
                                                              unsigned short volatile *expected,
                                                              unsigned short desired, int order)
{
    unsigned short old = *expected;
    unsigned short res = InterlockedCompareExchange16((volatile short *)object, desired, old);
    *expected = res;
    if (*expected == old) _atomic_fence_after_load(order);
    else if (order == memory_order_seq_cst) atomic_thread_fence(memory_order_seq_cst);
    return *expected == old;
}

/* fetch ops for u16 */
static inline unsigned short atomic_fetch_add_u16_explicit(short volatile *object, unsigned short operand, int order) {
    _atomic_fence_before_load(order);
    unsigned short old = InterlockedExchangeAdd16((short volatile *)object, operand);
    _atomic_fence_after_load(order);
    return old;
}
static inline unsigned short atomic_fetch_sub_u16_explicit(short volatile *object, unsigned short operand, int order) {
    _atomic_fence_before_load(order);
    unsigned short old = InterlockedExchangeAdd16((short volatile *)object, (unsigned short)(-(short)operand));
    _atomic_fence_after_load(order);
    return old;
}
static inline unsigned short atomic_fetch_or_u16_explicit(short volatile *object, unsigned short operand, int order) {
    _atomic_fence_before_load(order);
    unsigned short old = InterlockedOr16((short volatile *)object, operand);
    _atomic_fence_after_load(order);
    return old;
}
static inline unsigned short atomic_fetch_xor_u16_explicit(short volatile *object, unsigned short operand, int order) {
    _atomic_fence_before_load(order);
    unsigned short old = InterlockedXor16((short volatile *)object, operand);
    _atomic_fence_after_load(order);
    return old;
}
static inline unsigned short atomic_fetch_and_u16_explicit(short volatile *object, unsigned short operand, int order) {
    _atomic_fence_before_load(order);
    unsigned short old = InterlockedAnd16((short volatile *)object, operand);
    _atomic_fence_after_load(order);
    return old;
}

/* ---------------- byte / u8 ---------------- */

static inline void atomic_store_byte_explicit(unsigned char volatile *object, unsigned char desired, int order) {
    _atomic_fence_before_store(order);
    *(object) = desired;
    _atomic_fence_after_store(order);
}

static inline unsigned char atomic_load_byte_explicit(unsigned char volatile *object, int order) {
    unsigned char val;
    _atomic_fence_before_load(order);
    val = *object;
    _atomic_fence_after_load(order);
    return val;
}

static inline unsigned char atomic_exchange_byte_explicit(char volatile *object, unsigned char desired, int order) {
    _atomic_fence_before_store(order);
    char old = InterlockedExchange8((char volatile *)object, (char)desired);
    _atomic_fence_after_store(order);
    return (unsigned char)old;
}

static inline int atomic_compare_exchange_strong_byte_explicit(char volatile *object,
                                                               unsigned char volatile *expected,
                                                               unsigned char desired, int order)
{
    unsigned char old = *expected;
    unsigned char res = InterlockedCompareExchange8((char volatile *)object, desired, old);
    *expected = res;
    if (*expected == old) _atomic_fence_after_load(order);
    else if (order == memory_order_seq_cst) atomic_thread_fence(memory_order_seq_cst);
    return *expected == old;
}

/* fetch ops for byte */
static inline unsigned char atomic_fetch_add_byte_explicit(char volatile *object, unsigned char operand, int order) {
    _atomic_fence_before_load(order);
    unsigned char old = InterlockedExchangeAdd8((char volatile *)object, operand);
    _atomic_fence_after_load(order);
    return old;
}
static inline unsigned char atomic_fetch_sub_byte_explicit(char volatile *object, unsigned char operand, int order) {
    _atomic_fence_before_load(order);
    unsigned char old = InterlockedExchangeAdd8((char volatile *)object, (unsigned char)(-(signed char)operand));
    _atomic_fence_after_load(order);
    return old;
}
static inline unsigned char atomic_fetch_or_byte_explicit(char volatile *object, unsigned char operand, int order) {
    _atomic_fence_before_load(order);
    unsigned char old = InterlockedOr8((char volatile *)object, operand);
    _atomic_fence_after_load(order);
    return old;
}
static inline unsigned char atomic_fetch_xor_byte_explicit(char volatile *object, unsigned char operand, int order) {
    _atomic_fence_before_load(order);
    unsigned char old = InterlockedXor8((char volatile *)object, operand);
    _atomic_fence_after_load(order);
    return old;
}
static inline unsigned char atomic_fetch_and_byte_explicit(char volatile *object, unsigned char operand, int order) {
    _atomic_fence_before_load(order);
    unsigned char old = InterlockedAnd8((char volatile *)object, operand);
    _atomic_fence_after_load(order);
    return old;
}

#endif