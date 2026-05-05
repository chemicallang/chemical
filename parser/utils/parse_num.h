// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>
#include "core/source/SourceLocation.h"

template<typename T>
struct parse_num_result {
    T result;
    std::string_view error;
};

inline int get_parse_num_base(const char* num, std::size_t num_size) {
    if(num[0] == '0' && num_size > 1) {
        switch(num[1]) {
            case 'x':
            case 'X':
                return 16;
            case 'b':
            case 'B':
                return 2;
            case 'o':
            case 'O':
                return 8;
            default:
                return 10;
        }
    } else {
        return 10;
    }
}

inline std::size_t get_parse_num_prefix_size(const char* num, std::size_t num_size, int base) {
    if(num_size > 2 && num[0] == '0') {
        const auto prefix = num[1];
        if((base == 16 && (prefix == 'x' || prefix == 'X')) ||
           (base == 8 && (prefix == 'o' || prefix == 'O')) ||
           (base == 2 && (prefix == 'b' || prefix == 'B'))) {
            return 2;
        }
    }
    return 0;
}

template<typename T>
inline parse_num_result<T> parse_num(const char* _Ptr, std::size_t num_size, T(*parseFunc)(const char* _String, char** _EndPtr, int _Radix)) {
    int& _Errno_ref  = errno; // Nonzero cost, pay it once
    char* _Eptr;
    _Errno_ref               = 0;
    const auto base = get_parse_num_base(_Ptr, num_size);
    const auto prefix_size = get_parse_num_prefix_size(_Ptr, num_size, base);
    const auto parse_start = _Ptr + prefix_size;
    const auto parse_size = num_size - prefix_size;
    const T _Ans = parseFunc(parse_start, &_Eptr, base);
    if (parse_start == _Eptr) {
        return { _Ans, "invalid value provided" };
    }
    if(static_cast<std::size_t>(_Eptr - parse_start) != parse_size) {
        return { _Ans, "invalid value provided" };
    }
    if (_Errno_ref == ERANGE) {
        return { _Ans, "argument out of range" };
    }
    return { _Ans, "" };
}

template<typename T>
inline parse_num_result<T> parse_num(const char* _Ptr, std::size_t num_size, T(*parseFunc)(const char* _String, char** _EndPtr)) {
    int& _Errno_ref  = errno; // Nonzero cost, pay it once
    char* _Eptr;
    _Errno_ref               = 0;
    const T _Ans = parseFunc(_Ptr, &_Eptr);
    if (_Ptr == _Eptr) {
        return { _Ans, "invalid value provided" };
    }
    if (_Errno_ref == ERANGE) {
        return { _Ans, "argument out of range" };
    }
    return { _Ans, "" };
}

class Value;
class ASTAllocator;
class TypeBuilder;

Value* allocate_number_value(ASTAllocator& alloc, TypeBuilder& typeBuilder, uint64_t value, SourceLocation location);

parse_num_result<Value*> convert_number_to_value(ASTAllocator& alloc, TypeBuilder& typeBuilder, const char* value, std::size_t value_size, bool is64Bit, SourceLocation location);
