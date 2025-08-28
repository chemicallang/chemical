// Copyright (c) chemicallang 2025.

enum class IntNTypeKind {

    Char,
    Short,
    Int,
    Long,
    BigInt,
    Int128,

    UChar,
    UShort,
    UInt,
    ULong,
    UBigInt,
    UInt128,

    SignedStart = Char,
    SignedEnd = Int128,
    UnsignedStart = UChar,
    UnsignedEnd = UInt128

};