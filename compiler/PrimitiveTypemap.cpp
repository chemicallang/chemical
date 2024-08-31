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

    BaseType *make_any_type(bool is64Bit, CSTToken* token) {
        return new AnyType(token);
    }

    BaseType *make_bool_type(bool is64Bit, CSTToken* token) {
        return new BoolType(token);
    }

    BaseType *make_char_type(bool is64Bit, CSTToken* token) {
        return new CharType(token);
    }

    BaseType *make_uchar_type(bool is64Bit, CSTToken* token) {
        return new UCharType(token);
    }

    BaseType *make_double_type(bool is64Bit, CSTToken* token) {
        return new DoubleType(token);
    }

    BaseType *make_float_type(bool is64Bit, CSTToken* token) {
        return new FloatType(token);
    }

    BaseType *make_int_type(bool is64Bit, CSTToken* token) {
        return new IntType(token);
    }

    BaseType *make_uint_type(bool is64Bit, CSTToken* token) {
        return new UIntType(token);
    }

    BaseType *make_short_type(bool is64Bit, CSTToken* token) {
        return new ShortType(token);
    }

    BaseType *make_ushort_type(bool is64Bit, CSTToken* token) {
        return new UShortType(token);
    }

    BaseType *make_long_type(bool is64Bit, CSTToken* token) {
        return new LongType(is64Bit, token);
    }

    BaseType *make_ulong_type(bool is64Bit, CSTToken* token) {
        return new ULongType(is64Bit, token);
    }

    BaseType *make_bigint_type(bool is64Bit, CSTToken* token) {
        return new BigIntType(token);
    }

    BaseType *make_ubigint_type(bool is64Bit, CSTToken* token) {
        return new UBigIntType(token);
    }

    BaseType *make___int128_type(bool is64Bit, CSTToken* token) {
        return new Int128Type(token);
    }

    BaseType *make___uint128_type(bool is64Bit, CSTToken* token) {
        return new UInt128Type(token);
    }

    BaseType *make_string_type(bool is64Bit, CSTToken* token) {
        return new StringType(token);
    }

    BaseType *make_void_type(bool is64Bit, CSTToken* token) {
        return new VoidType(token);
    }

}