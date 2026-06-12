// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <cstdint>

/**
 * 64-bit flags that track which trait interfaces (Copy, etc.) are
 * satisfied by a type or required by a generic parameter constraint.
 *
 * Bit 0 = Copy (trivially copyable / memcpy-able)
 * Bits 1..63 = reserved for future trait interfaces.
 */
class InterfaceBits {
public:
    using BitsType = uint64_t;

    static constexpr BitsType COPY_BIT = BitsType(1) << 0;

    constexpr InterfaceBits() : bits_(0) {}
    constexpr InterfaceBits(BitsType bits) : bits_(bits) {}

    constexpr bool has(BitsType mask) const { return (bits_ & mask) == mask; }
    constexpr bool has_any(BitsType mask) const { return (bits_ & mask) != 0; }
    constexpr void set(BitsType mask) { bits_ |= mask; }
    constexpr void clear(BitsType mask) { bits_ &= ~mask; }
    constexpr BitsType raw() const { return bits_; }

    constexpr InterfaceBits& operator|=(const InterfaceBits& other) { bits_ |= other.bits_; return *this; }
    constexpr InterfaceBits& operator&=(const InterfaceBits& other) { bits_ &= other.bits_; return *this; }

    friend constexpr InterfaceBits operator|(InterfaceBits a, const InterfaceBits& b) { a |= b; return a; }
    friend constexpr InterfaceBits operator&(InterfaceBits a, const InterfaceBits& b) { a &= b; return a; }
    friend constexpr bool operator==(const InterfaceBits& a, const InterfaceBits& b) { return a.bits_ == b.bits_; }
    friend constexpr bool operator!=(const InterfaceBits& a, const InterfaceBits& b) { return a.bits_ != b.bits_; }

    static constexpr InterfaceBits copy() { return InterfaceBits(COPY_BIT); }

private:
    BitsType bits_;
};
