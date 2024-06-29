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

void evaluate_values(std::vector<std::unique_ptr<Value>>& values, InterpretScope& scope) {
    for(auto& value : values) {
        auto evaluated = value->evaluated_value(scope);
        if(evaluated.get() == value.get()) continue;
        if(evaluated.get_will_free()) {
            value.reset(evaluated.release());
        } else {
            value.reset(evaluated->copy());
        }
    }
}