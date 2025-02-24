// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>
#include <unordered_map>
#include "cst/SourceLocation.h"
#include "std/chem_string_view.h"

class ASTAllocator;

class BaseType;

class CSTToken;

/**
 * primitive type provide is a function when given a type in string format
 * like 'int' it will create a AST BaseType
 */
typedef BaseType *(*PrimitiveTypeProvider)(ASTAllocator& allocator, bool is64Bit, SourceLocation location);

namespace TypeMakers {

    BaseType *make_any_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location);

    BaseType *make_bool_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location);

    BaseType *make_char_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location);

    BaseType *make_uchar_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location);

    BaseType *make_double_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location);

    BaseType *make_float_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location);

    BaseType *make_long_double_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location);

    BaseType *make_int_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location);

    BaseType *make_uint_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location);

    BaseType *make_short_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location);

    BaseType *make_ushort_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location);

    BaseType *make_long_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location);

    BaseType *make_ulong_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location);

    BaseType *make_bigint_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location);

    BaseType *make_ubigint_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location);

    BaseType *make_int128_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location);

    BaseType *make_uint128_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location);

    BaseType *make_float128_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location);

    BaseType *make_string_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location);

    BaseType *make_void_type(ASTAllocator& allocator, bool is64Bit, SourceLocation location);

    const std::unordered_map<chem::string_view, PrimitiveTypeProvider> PrimitiveMap = {
            {"any",       TypeMakers::make_any_type},
            {"bool",      TypeMakers::make_bool_type},
            {"char",      TypeMakers::make_char_type},
            {"uchar",      TypeMakers::make_uchar_type},
            {"double",    TypeMakers::make_double_type},
            {"float",     TypeMakers::make_float_type},
            {"longdouble",     TypeMakers::make_long_double_type},
            {"int",       TypeMakers::make_int_type},
            {"uint",      TypeMakers::make_uint_type},
            {"short",     TypeMakers::make_short_type},
            {"ushort",    TypeMakers::make_ushort_type},
            {"long",      TypeMakers::make_long_type},
            {"ulong",     TypeMakers::make_ulong_type},
            {"bigint",    TypeMakers::make_bigint_type},
            {"ubigint",   TypeMakers::make_ubigint_type},
            {"int128",  TypeMakers::make_int128_type},
            {"uint128", TypeMakers::make_uint128_type},
            {"float128", TypeMakers::make_float128_type},
//            {"string",    TypeMakers::make_string_type},
            {"void",      TypeMakers::make_void_type},
    };

}