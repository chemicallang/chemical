// Copyright (c) Chemical Language Foundation 2025.

#ifndef COMPAT_ATOMICS_WIN32_STDATOMIC_H

#include "atomic.h"
#include <stdbool.h>

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

void atomic_store_u64_explicit(unsigned long long volatile *object,
                                             unsigned long long desired, int order)
{
    /* For non-Interlocked store path: honour relaxed/release/seq_cst */
    _atomic_fence_before_store(order);
    /* perform the store */
    *(object) = (desired);
    _atomic_fence_after_store(order);
}

unsigned long long atomic_load_u64_explicit(unsigned long long volatile *object,
                                                          int order)
{
    unsigned long long val;
    _atomic_fence_before_load(order);
    val = *(object);
    _atomic_fence_after_load(order);
    return val;
}

unsigned long long atomic_exchange_u64_explicit(LONGLONG volatile *object,
                                                              unsigned long long desired, int order)
{
    /* InterlockedExchange64 is a full barrier; we still insert fences to match requested ordering
       as closely as possible for side effects before/after the call. */
    _atomic_fence_before_store(order);
    LONGLONG old = InterlockedExchange64(object, (LONGLONG)desired);
    _atomic_fence_after_store(order);
    return (unsigned long long)old;
}

_Bool atomic_compare_exchange_strong_u64_explicit(unsigned long long volatile *object,
                                                              unsigned long long volatile *expected,
                                                              unsigned long long desired, int success_order, int failure_order)
{
    unsigned long long old = *expected;

    // Fences for failure ordering (only when CAS fails)
    _atomic_fence_before_store(failure_order);

    LONGLONG res = InterlockedCompareExchange64(
            (LONGLONG volatile *)object,
            (LONGLONG)desired,
            (LONGLONG)old);

    if (res == old) {
        // Success path: success_order applies
        _atomic_fence_after_load(success_order);
        return true;
    } else {
        // Failure path: failure_order applies
        *expected = (unsigned long long)res;
        _atomic_fence_after_load(failure_order);
        return false;
    }
}

_Bool atomic_compare_exchange_weak_u64_explicit(unsigned long long volatile *object,
                                                            unsigned long long volatile *expected,
                                                            unsigned long long desired, int success_order, int failure_order)
{
    /* Implement weak as strong here (x86 does not provide spurious failures guarantee easily) */
    return atomic_compare_exchange_strong_u64_explicit(object, expected, desired, success_order, failure_order);
}

#if defined(_WIN64)
unsigned long long atomic_fetch_add_u64_explicit(LONGLONG volatile *object, unsigned long long operand, int order) {
    /* InterlockedExchangeAdd64 is full fence */
    _atomic_fence_before_load(order);
    LONGLONG old = InterlockedExchangeAdd64(object, (LONGLONG)operand);
    _atomic_fence_after_load(order);
    return (unsigned long long)old;
}
unsigned long long atomic_fetch_sub_u64_explicit(LONGLONG volatile *object, unsigned long long operand, int order) {
    _atomic_fence_before_load(order);
    LONGLONG old = InterlockedExchangeAdd64(object, -(LONGLONG)operand);
    _atomic_fence_after_load(order);
    return (unsigned long long)old;
}
unsigned long long atomic_fetch_or_u64_explicit(LONGLONG volatile *object, unsigned long long operand, int order) {
    _atomic_fence_before_load(order);
    LONGLONG old = InterlockedOr64(object, (LONGLONG)operand);
    _atomic_fence_after_load(order);
    return (unsigned long long)old;
}
unsigned long long atomic_fetch_xor_u64_explicit(LONGLONG volatile *object, unsigned long long operand, int order) {
    _atomic_fence_before_load(order);
    LONGLONG old = InterlockedXor64(object, (LONGLONG)operand);
    _atomic_fence_after_load(order);
    return (unsigned long long)old;
}
unsigned long long atomic_fetch_and_u64_explicit(LONGLONG volatile *object, unsigned long long operand, int order) {
    _atomic_fence_before_load(order);
    LONGLONG old = InterlockedAnd64(object, (LONGLONG)operand);
    _atomic_fence_after_load(order);
    return (unsigned long long)old;
}
#else
/* If building 32-bit, the existing macros/impl remain but expose explicit wrappers if needed.
   You can adapt these to use ManualInterlocked... helpers. */
unsigned long long atomic_fetch_add_u64_explicit(volatile long long *object, unsigned long long operand, int order) {
    (void)order;
    return (unsigned long long)InterlockedExchangeAdd64((LONGLONG volatile *)object, (LONGLONG)operand);
}
unsigned long long atomic_fetch_sub_u64_explicit(volatile long long *object, unsigned long long operand, int order) {
    (void)order;
    return (unsigned long long)InterlockedExchangeAdd64((LONGLONG volatile *)object, -(LONGLONG)operand);
}
unsigned long long atomic_fetch_or_u64_explicit(volatile long long *object, unsigned long long operand, int order) {
    (void)order;
    return (unsigned long long)InterlockedOr64((LONGLONG volatile *)object, (LONGLONG)operand);
}
unsigned long long atomic_fetch_xor_u64_explicit(volatile long long *object, unsigned long long operand, int order) {
    (void)order;
    return (unsigned long long)InterlockedXor64((LONGLONG volatile *)object, (LONGLONG)operand);
}
unsigned long long atomic_fetch_and_u64_explicit(volatile long long *object, unsigned long long operand, int order) {
    (void)order;
    return (unsigned long long)InterlockedAnd64((LONGLONG volatile *)object, (LONGLONG)operand);
}
#endif

/* ---------------- u32 ---------------- */

/* store */
void atomic_store_u32_explicit(unsigned volatile *object, unsigned desired, int order)
{
    _atomic_fence_before_store(order);
    *(object) = (desired);
    _atomic_fence_after_store(order);
}

/* load */
unsigned atomic_load_u32_explicit(unsigned volatile *object, int order)
{
    unsigned val;
    _atomic_fence_before_load(order);
    val = *(object);
    _atomic_fence_after_load(order);
    return val;
}

/* exchange */
unsigned atomic_exchange_u32_explicit(long volatile *object, unsigned desired, int order)
{
    /* InterlockedExchange acts as a full fence; add requested fences around call */
    _atomic_fence_before_store(order);
    LONG old = InterlockedExchange(object, (LONG)desired);
    _atomic_fence_after_store(order);
    return (unsigned)old;
}

/* compare-exchange (strong) */
_Bool atomic_compare_exchange_strong_u32_explicit(unsigned volatile *object,
                                                              unsigned volatile *expected,
                                                              unsigned desired, int success_order, int failure_order)
{
    unsigned int old = *expected;

    _atomic_fence_before_store(failure_order);

    unsigned int res = (unsigned int)InterlockedCompareExchange(
            (LONG volatile *)object,
            (LONG)desired,
            (LONG)old);

    if (res == old) {
        _atomic_fence_after_load(success_order);
        return true;
    } else {
        *expected = res;
        _atomic_fence_after_load(failure_order);
        return false;
    }
}

/* weak implemented as strong (x86 does not provide spurious failure easily) */
_Bool atomic_compare_exchange_weak_u32_explicit(unsigned volatile *object,
                                                            unsigned volatile *expected,
                                                            unsigned desired, int success_order, int failure_order)
{
    return atomic_compare_exchange_strong_u32_explicit(object, expected, desired, success_order, failure_order);
}

/* fetch ops */
unsigned atomic_fetch_add_u32_explicit(long volatile *object, unsigned operand, int order) {
    _atomic_fence_before_load(order);
    LONG old = InterlockedExchangeAdd(object, (LONG)operand);
    _atomic_fence_after_load(order);
    return (unsigned)old;
}
unsigned atomic_fetch_sub_u32_explicit(long volatile *object, unsigned operand, int order) {
    _atomic_fence_before_load(order);
    LONG old = InterlockedExchangeAdd(object, -(LONG)operand);
    _atomic_fence_after_load(order);
    return (unsigned)old;
}
unsigned atomic_fetch_or_u32_explicit(long volatile *object, unsigned operand, int order) {
    _atomic_fence_before_load(order);
    LONG old = InterlockedOr(object, (LONG)operand);
    _atomic_fence_after_load(order);
    return (unsigned)old;
}
unsigned atomic_fetch_xor_u32_explicit(long volatile *object, unsigned operand, int order) {
    _atomic_fence_before_load(order);
    LONG old = InterlockedXor(object, (LONG)operand);
    _atomic_fence_after_load(order);
    return (unsigned)old;
}
unsigned atomic_fetch_and_u32_explicit(long volatile *object, unsigned operand, int order) {
    _atomic_fence_before_load(order);
    LONG old = InterlockedAnd(object, (LONG)operand);
    _atomic_fence_after_load(order);
    return (unsigned)old;
}

/* ---------------- u16 ---------------- */

void atomic_store_u16_explicit(unsigned short volatile *object, unsigned short desired, int order)
{
    _atomic_fence_before_store(order);
    *(object) = (desired);
    _atomic_fence_after_store(order);
}

unsigned short atomic_load_u16_explicit(unsigned short volatile *object, int order)
{
    unsigned short val;
    _atomic_fence_before_load(order);
    val = *(object);
    _atomic_fence_after_load(order);
    return val;
}

unsigned short atomic_exchange_u16_explicit(unsigned short volatile *object, unsigned short desired, int order)
{
    _atomic_fence_before_store(order);
    unsigned short old = InterlockedExchange16((volatile short *)object, (short)desired);
    _atomic_fence_after_store(order);
    return old;
}

_Bool atomic_compare_exchange_strong_u16_explicit(short volatile *object,
                                                              unsigned short volatile *expected,
                                                              unsigned short desired, int success_order, int failure_order)
{
    unsigned short old = *expected;

    _atomic_fence_before_store(failure_order);

    unsigned short res = (unsigned short)InterlockedCompareExchange16(
            (SHORT volatile *)object,
            (SHORT)desired,
            (SHORT)old);

    if (res == old) {
        _atomic_fence_after_load(success_order);
        return true;
    } else {
        *expected = res;
        _atomic_fence_after_load(failure_order);
        return false;
    }
}

/* weak implemented as strong (x86 does not provide spurious failure easily) */
_Bool atomic_compare_exchange_weak_u16_explicit(short volatile *object,
                                                            unsigned short volatile *expected,
                                                            unsigned short desired, int success_order, int failure_order)
{
    return atomic_compare_exchange_strong_u16_explicit(object, expected, desired, success_order, failure_order);
}

/* fetch ops for u16 */
unsigned short atomic_fetch_add_u16_explicit(short volatile *object, unsigned short operand, int order) {
    _atomic_fence_before_load(order);
    unsigned short old = InterlockedExchangeAdd16((short volatile *)object, operand);
    _atomic_fence_after_load(order);
    return old;
}
unsigned short atomic_fetch_sub_u16_explicit(short volatile *object, unsigned short operand, int order) {
    _atomic_fence_before_load(order);
    unsigned short old = InterlockedExchangeAdd16((short volatile *)object, (unsigned short)(-(short)operand));
    _atomic_fence_after_load(order);
    return old;
}
unsigned short atomic_fetch_or_u16_explicit(short volatile *object, unsigned short operand, int order) {
    _atomic_fence_before_load(order);
    unsigned short old = InterlockedOr16((short volatile *)object, operand);
    _atomic_fence_after_load(order);
    return old;
}
unsigned short atomic_fetch_xor_u16_explicit(short volatile *object, unsigned short operand, int order) {
    _atomic_fence_before_load(order);
    unsigned short old = InterlockedXor16((short volatile *)object, operand);
    _atomic_fence_after_load(order);
    return old;
}
unsigned short atomic_fetch_and_u16_explicit(short volatile *object, unsigned short operand, int order) {
    _atomic_fence_before_load(order);
    unsigned short old = InterlockedAnd16((short volatile *)object, operand);
    _atomic_fence_after_load(order);
    return old;
}

/* ---------------- byte / u8 ---------------- */

void atomic_store_byte_explicit(unsigned char volatile *object, unsigned char desired, int order) {
    _atomic_fence_before_store(order);
    *(object) = desired;
    _atomic_fence_after_store(order);
}

unsigned char atomic_load_byte_explicit(unsigned char volatile *object, int order) {
    unsigned char val;
    _atomic_fence_before_load(order);
    val = *object;
    _atomic_fence_after_load(order);
    return val;
}

unsigned char atomic_exchange_byte_explicit(char volatile *object, unsigned char desired, int order) {
    _atomic_fence_before_store(order);
    char old = InterlockedExchange8((char volatile *)object, (char)desired);
    _atomic_fence_after_store(order);
    return (unsigned char)old;
}

_Bool atomic_compare_exchange_strong_byte_explicit(char volatile *object,
                                                               unsigned char volatile *expected,
                                                               unsigned char desired, int success_order, int failure_order)
{
    unsigned char old = *expected;

    _atomic_fence_before_store(failure_order);

    unsigned char res = (unsigned char)InterlockedCompareExchange8(
            (CHAR volatile *)object,
            (CHAR)desired,
            (CHAR)old);

    if (res == old) {
        _atomic_fence_after_load(success_order);
        return true;
    } else {
        *expected = res;
        _atomic_fence_after_load(failure_order);
        return false;
    }
}

/* weak implemented as strong (x86 does not provide spurious failure easily) */
_Bool atomic_compare_exchange_weak_byte_explicit(char volatile *object,
                                                            unsigned char volatile *expected,
                                                            unsigned char desired, int success_order, int failure_order)
{
    return atomic_compare_exchange_strong_byte_explicit(object, expected, desired, success_order, failure_order);
}

/* fetch ops for byte */
unsigned char atomic_fetch_add_byte_explicit(char volatile *object, unsigned char operand, int order) {
    _atomic_fence_before_load(order);
    unsigned char old = InterlockedExchangeAdd8((char volatile *)object, operand);
    _atomic_fence_after_load(order);
    return old;
}
unsigned char atomic_fetch_sub_byte_explicit(char volatile *object, unsigned char operand, int order) {
    _atomic_fence_before_load(order);
    unsigned char old = InterlockedExchangeAdd8((char volatile *)object, (unsigned char)(-(signed char)operand));
    _atomic_fence_after_load(order);
    return old;
}
unsigned char atomic_fetch_or_byte_explicit(char volatile *object, unsigned char operand, int order) {
    _atomic_fence_before_load(order);
    unsigned char old = InterlockedOr8((char volatile *)object, operand);
    _atomic_fence_after_load(order);
    return old;
}
unsigned char atomic_fetch_xor_byte_explicit(char volatile *object, unsigned char operand, int order) {
    _atomic_fence_before_load(order);
    unsigned char old = InterlockedXor8((char volatile *)object, operand);
    _atomic_fence_after_load(order);
    return old;
}
unsigned char atomic_fetch_and_byte_explicit(char volatile *object, unsigned char operand, int order) {
    _atomic_fence_before_load(order);
    unsigned char old = InterlockedAnd8((char volatile *)object, operand);
    _atomic_fence_after_load(order);
    return old;
}

#endif