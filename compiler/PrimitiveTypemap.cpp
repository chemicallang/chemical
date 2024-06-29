// Copyright (c) Qinetik 2024.

#include "PrimitiveTypeMap.h"
#include "ast/types/AnyType.h"
#include "ast/types/BoolType.h"
#include "ast/types/CharType.h"
#include "ast/types/UCharType.h"
#include "ast/types/DoubleType.h"
#include "ast/types/FloatType.h"
#include "ast/types/IntType.h"
#include "ast/types/UIntType.h"
#include "ast/types/ShortType.h"
#include "ast/types/UShortType.h"
#include "ast/types/LongType.h"
#include "ast/types/ULongType.h"
#include "ast/types/BigIntType.h"
#include "ast/types/UBigIntType.h"
#include "ast/types/Int128Type.h"
#include "ast/types/UInt128Type.h"
#include "ast/types/StringType.h"
#include "ast/types/VoidType.h"

namespace TypeMakers {

    BaseType *make_any_type(bool is64Bit) {
        return new AnyType();
    }

    BaseType *make_bool_type(bool is64Bit) {
        return new BoolType();
    }

    BaseType *make_char_type(bool is64Bit) {
        return new CharType();
    }

    BaseType *make_uchar_type(bool is64Bit) {
        return new UCharType();
    }

    BaseType *make_double_type(bool is64Bit) {
        return new DoubleType();
    }

    BaseType *make_float_type(bool is64Bit) {
        return new FloatType();
    }

    BaseType *make_int_type(bool is64Bit) {
        return new IntType();
    }

    BaseType *make_uint_type(bool is64Bit) {
        return new UIntType();
    }

    BaseType *make_short_type(bool is64Bit) {
        return new ShortType();
    }

    BaseType *make_ushort_type(bool is64Bit) {
        return new UShortType();
    }

    BaseType *make_long_type(bool is64Bit) {
        return new LongType(is64Bit);
    }

    BaseType *make_ulong_type(bool is64Bit) {
        return new ULongType(is64Bit);
    }

    BaseType *make_bigint_type(bool is64Bit) {
        return new BigIntType();
    }

    BaseType *make_ubigint_type(bool is64Bit) {
        return new UBigIntType();
    }

    BaseType *make___int128_type(bool is64Bit) {
        return new Int128Type();
    }

    BaseType *make___uint128_type(bool is64Bit) {
        return new UInt128Type();
    }

    BaseType *make_string_type(bool is64Bit) {
        return new StringType();
    }

    BaseType *make_void_type(bool is64Bit) {
        return new VoidType();
    }

}