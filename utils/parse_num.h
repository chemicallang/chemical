// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include "cst/SourceLocation.h"

template<typename T>
struct parse_num_result {
    T result;
    std::string_view error;
};

inline int get_parse_num_base(const char* num, std::size_t num_size) {
    return (num[0] == '0' && num_size > 1 && (num[1] == 'x' || num[1] == 'X')) ? 16 : 10;
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

parse_num_result<Value*> convert_number_to_value(ASTAllocator& alloc, char* mut_value, std::size_t value_size, bool is64Bit, SourceLocation location);