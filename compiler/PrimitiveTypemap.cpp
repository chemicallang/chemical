// Copyright (c) Qinetik 2024.

#include "PrimitiveTypeMap.h"
#include "ast/types/AnyType.h"
#include "ast/types/BoolType.h"
#include "ast/types/CharType.h"
#include "ast/types/UCharType.h"
#include "ast/types/DoubleType.h"
#include "ast/types/FloatType.h"
#include "ast/types/Float128Type.h"
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
#include "ast/types/LongDoubleType.h"
#include "ast/types/StringType.h"
#include "ast/types/VoidType.h"

namespace TypeMakers {

    BaseType *make_any_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
        return new (allocator.allocate<AnyType>()) AnyType(location);
    }

    BaseType *make_bool_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
        return new (allocator.allocate<BoolType>()) BoolType(location);
    }

    BaseType *make_char_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
        return new (allocator.allocate<CharType>()) CharType(location);
    }

    BaseType *make_uchar_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
        return new (allocator.allocate<UCharType>()) UCharType(location);
    }

    BaseType *make_double_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
        return new (allocator.allocate<DoubleType>()) DoubleType(location);
    }

    BaseType *make_float_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
        return new (allocator.allocate<FloatType>()) FloatType(location);
    }

    BaseType *make_long_double_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
        return new (allocator.allocate<LongDoubleType>()) LongDoubleType(location);
    }

    BaseType *make_int_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
        return new (allocator.allocate<IntType>()) IntType(location);
    }

    BaseType *make_uint_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
        return new (allocator.allocate<UIntType>()) UIntType(location);
    }

    BaseType *make_short_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
        return new (allocator.allocate<ShortType>()) ShortType(location);
    }

    BaseType *make_ushort_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
        return new (allocator.allocate<UShortType>()) UShortType(location);
    }

    BaseType *make_long_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
        return new (allocator.allocate<LongType>()) LongType(is64Bit, location);
    }

    BaseType *make_ulong_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
        return new (allocator.allocate<ULongType>()) ULongType(is64Bit, location);
    }

    BaseType *make_bigint_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
        return new (allocator.allocate<BigIntType>()) BigIntType(location);
    }

    BaseType *make_ubigint_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
        return new (allocator.allocate<UBigIntType>()) UBigIntType(location);
    }

    BaseType *make_int128_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
        return new (allocator.allocate<Int128Type>()) Int128Type(location);
    }

    BaseType *make_uint128_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
        return new (allocator.allocate<UInt128Type>()) UInt128Type(location);
    }

    BaseType *make_float128_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
        return new (allocator.allocate<Float128Type>()) Float128Type(location);
    }

    BaseType *make_string_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
        return new (allocator.allocate<StringType>()) StringType(location);
    }

    BaseType *make_void_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
        return new (allocator.allocate<VoidType>()) VoidType(location);
    }

}