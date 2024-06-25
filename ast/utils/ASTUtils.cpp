// Copyright (c) Qinetik 2024.

#include "ASTUtils.h"

bool chain_contains_func_call(std::vector<std::unique_ptr<Value>>& values, int start, int end) {
    while(start < end) {
        if(values[start]->as_func_call()) {
            return true;
        }
        start++;
    }
    return false;
}