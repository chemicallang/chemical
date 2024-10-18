// Copyright (c) Qinetik 2024.

#include "FloatType.h"
#include "ast/values/FloatValue.h"
#include "Float128Type.h"

bool FloatType::can_promote(Value *value) {
    return value->primitive() && value->value_type() == ValueType::Int;
}

Value *FloatType::promote(Value *value) {
    if(value->primitive() && value->value_type() == ValueType::Int) {
        return new FloatValue((float) value->get_the_int(), value->cst_token());
    } else {
        return nullptr;
    }
}

bool Float128Type::can_promote(Value *value) {
    return value->primitive() && value->value_type() == ValueType::Int;
}

Value *Float128Type::promote(Value *value) {
    // TODO promote it to a float 128 type instead
    if(value->primitive() && value->value_type() == ValueType::Int) {
        return new FloatValue((float) value->get_the_int(), value->cst_token());
    } else {
        return nullptr;
    }
}