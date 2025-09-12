// Copyright (c) chemicallang 2025.

enum class IntNTypeKind {

    Char,
    Short,
    Int,
    Long,
    BigInt,
    Int128,

    I8,
    I16,
    I32,
    I64,

    UChar,
    UShort,
    UInt,
    ULong,
    UBigInt,
    UInt128,

    U8,
    U16,
    U32,
    U64,

    UnsignedStart = UChar,

};

inline constexpr bool is_unsigned(IntNTypeKind kind) {
    return kind >= IntNTypeKind::UnsignedStart;
}

inline constexpr IntNTypeKind to_signed(IntNTypeKind kind) {
    if(!is_unsigned(kind)) return kind;
    return static_cast<IntNTypeKind>(static_cast<int>(kind) - static_cast<int>(IntNTypeKind::UnsignedStart));
}

inline constexpr IntNTypeKind determine_output(IntNTypeKind first, IntNTypeKind second) {
    if(is_unsigned(first) && is_unsigned(second) || (!is_unsigned(first) && !is_unsigned(second))) {
        return first > second ? first : second;
    } else {
        // one is signed, one is unsigned
        if(is_unsigned(first)) {
            // first is unsigned
            const auto signedFirst = to_signed(first);
            return signedFirst > second ? signedFirst : second;
        } else {
            // second is unsigned
            const auto signedSecond = to_signed(second);
            return signedSecond > first ? signedSecond : first;
        }
    }
}