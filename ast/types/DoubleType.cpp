// Copyright (c) Qinetik 2024.

#include "DoubleType.h"
#include "ast/values/DoubleValue.h"

bool DoubleType::can_promote(Value *value) {
    return value->primitive() && value->value_type() == ValueType::Int;
}

Value *DoubleType::promote(Value *value) {
    if(value->primitive() && value->value_type() == ValueType::Int) {
        return new DoubleValue((double) value->get_the_int(), value->cst_token());
    } else {
        return nullptr;
    }
}