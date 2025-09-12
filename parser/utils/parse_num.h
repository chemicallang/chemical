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

template<typename T>
inline parse_num_result<T> parse_num(const char* _Ptr, std::size_t num_size, T(*parseFunc)(const char* _String, char** _EndPtr, int _Radix)) {
    int& _Errno_ref  = errno; // Nonzero cost, pay it once
    char* _Eptr;
    _Errno_ref               = 0;
    const T _Ans = parseFunc(_Ptr, &_Eptr, get_parse_num_base(_Ptr, num_size));
    if (_Ptr == _Eptr) {
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