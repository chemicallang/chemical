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

    BaseType *make_any_type(ASTAllocator& allocator, bool is64Bit, CSTToken* token) {
        return new (allocator.allocate<AnyType>()) AnyType(token);
    }

    BaseType *make_bool_type(ASTAllocator& allocator, bool is64Bit, CSTToken* token) {
        return new (allocator.allocate<BoolType>()) BoolType(token);
    }

    BaseType *make_char_type(ASTAllocator& allocator, bool is64Bit, CSTToken* token) {
        return new (allocator.allocate<CharType>()) CharType(token);
    }

    BaseType *make_uchar_type(ASTAllocator& allocator, bool is64Bit, CSTToken* token) {
        return new (allocator.allocate<UCharType>()) UCharType(token);
    }

    BaseType *make_double_type(ASTAllocator& allocator, bool is64Bit, CSTToken* token) {
        return new (allocator.allocate<DoubleType>()) DoubleType(token);
    }

    BaseType *make_float_type(ASTAllocator& allocator, bool is64Bit, CSTToken* token) {
        return new (allocator.allocate<FloatType>()) FloatType(token);
    }

    BaseType *make_int_type(ASTAllocator& allocator, bool is64Bit, CSTToken* token) {
        return new (allocator.allocate<IntType>()) IntType(token);
    }

    BaseType *make_uint_type(ASTAllocator& allocator, bool is64Bit, CSTToken* token) {
        return new (allocator.allocate<UIntType>()) UIntType(token);
    }

    BaseType *make_short_type(ASTAllocator& allocator, bool is64Bit, CSTToken* token) {
        return new (allocator.allocate<ShortType>()) ShortType(token);
    }

    BaseType *make_ushort_type(ASTAllocator& allocator, bool is64Bit, CSTToken* token) {
        return new (allocator.allocate<UShortType>()) UShortType(token);
    }

    BaseType *make_long_type(ASTAllocator& allocator, bool is64Bit, CSTToken* token) {
        return new (allocator.allocate<LongType>()) LongType(is64Bit, token);
    }

    BaseType *make_ulong_type(ASTAllocator& allocator, bool is64Bit, CSTToken* token) {
        return new (allocator.allocate<ULongType>()) ULongType(is64Bit, token);
    }

    BaseType *make_bigint_type(ASTAllocator& allocator, bool is64Bit, CSTToken* token) {
        return new (allocator.allocate<BigIntType>()) BigIntType(token);
    }

    BaseType *make_ubigint_type(ASTAllocator& allocator, bool is64Bit, CSTToken* token) {
        return new (allocator.allocate<UBigIntType>()) UBigIntType(token);
    }

    BaseType *make___int128_type(ASTAllocator& allocator, bool is64Bit, CSTToken* token) {
        return new (allocator.allocate<Int128Type>()) Int128Type(token);
    }

    BaseType *make___uint128_type(ASTAllocator& allocator, bool is64Bit, CSTToken* token) {
        return new (allocator.allocate<UInt128Type>()) UInt128Type(token);
    }

    BaseType *make_string_type(ASTAllocator& allocator, bool is64Bit, CSTToken* token) {
        return new (allocator.allocate<StringType>()) StringType(token);
    }

    BaseType *make_void_type(ASTAllocator& allocator, bool is64Bit, CSTToken* token) {
        return new (allocator.allocate<VoidType>()) VoidType(token);
    }

}