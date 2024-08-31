// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include <unordered_map>

class BaseType;

class CSTToken;

/**
 * primitive type provide is a function when given a type in string format
 * like 'int' it will create a AST BaseType
 */
typedef BaseType *(*PrimitiveTypeProvider)(bool is64Bit, CSTToken* token);

namespace TypeMakers {

    BaseType *make_any_type(bool is64Bit, CSTToken* token);

    BaseType *make_bool_type(bool is64Bit, CSTToken* token);

    BaseType *make_char_type(bool is64Bit, CSTToken* token);

    BaseType *make_uchar_type(bool is64Bit, CSTToken* token);

    BaseType *make_double_type(bool is64Bit, CSTToken* token);

    BaseType *make_float_type(bool is64Bit, CSTToken* token);

    BaseType *make_int_type(bool is64Bit, CSTToken* token);

    BaseType *make_uint_type(bool is64Bit, CSTToken* token);

    BaseType *make_short_type(bool is64Bit, CSTToken* token);

    BaseType *make_ushort_type(bool is64Bit, CSTToken* token);

    BaseType *make_long_type(bool is64Bit, CSTToken* token);

    BaseType *make_ulong_type(bool is64Bit, CSTToken* token);

    BaseType *make_bigint_type(bool is64Bit, CSTToken* token);

    BaseType *make_ubigint_type(bool is64Bit, CSTToken* token);

    BaseType *make___int128_type(bool is64Bit, CSTToken* token);

    BaseType *make___uint128_type(bool is64Bit, CSTToken* token);

    BaseType *make_string_type(bool is64Bit, CSTToken* token);

    BaseType *make_void_type(bool is64Bit, CSTToken* token);

    const std::unordered_map<std::string, PrimitiveTypeProvider> PrimitiveMap = {
            {"any",       TypeMakers::make_any_type},
            {"bool",      TypeMakers::make_bool_type},
            {"char",      TypeMakers::make_char_type},
            {"uchar",      TypeMakers::make_uchar_type},
            {"double",    TypeMakers::make_double_type},
            {"float",     TypeMakers::make_float_type},
            {"int",       TypeMakers::make_int_type},
            {"uint",      TypeMakers::make_uint_type},
            {"short",     TypeMakers::make_short_type},
            {"ushort",    TypeMakers::make_ushort_type},
            {"long",      TypeMakers::make_long_type},
            {"ulong",     TypeMakers::make_ulong_type},
            {"bigint",    TypeMakers::make_bigint_type},
            {"ubigint",   TypeMakers::make_ubigint_type},
            {"__int128",  TypeMakers::make___int128_type},
            {"__uint128", TypeMakers::make___uint128_type},
//            {"string",    TypeMakers::make_string_type},
            {"void",      TypeMakers::make_void_type},
    };

}