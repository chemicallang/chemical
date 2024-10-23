// Copyright (c) Qinetik 2024.

#include "FloatType.h"
#include "DoubleType.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/FloatValue.h"
#include "Float128Type.h"
#include "ComplexType.h"
#include "LongDoubleType.h"

bool DoubleType::can_promote(Value *value) {
    return value->primitive() && value->value_type() == ValueType::Int;
}

Value *DoubleType::promote(Value *value) {
    if(value->primitive() && value->value_type() == ValueType::Int) {
        // TODO not allocating using an allocator
        return new DoubleValue((double) value->get_the_int(), value->cst_token());
    } else {
        return nullptr;
    }
}

bool FloatType::can_promote(Value *value) {
    return value->primitive() && value->value_type() == ValueType::Int;
}

Value *FloatType::promote(Value *value) {
    if(value->primitive() && value->value_type() == ValueType::Int) {
        // TODO not allocating using an allocator
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
        // TODO not allocating using an allocator
        return new FloatValue((float) value->get_the_int(), value->cst_token());
    } else {
        return nullptr;
    }
}

bool LongDoubleType::can_promote(Value *value) {
    return value->primitive() && value->value_type() == ValueType::Int;
}

Value *LongDoubleType::promote(Value *value) {
    // TODO promote it to a float 128 type instead
    if(value->primitive() && value->value_type() == ValueType::Int) {
        // TODO not allocating using an allocator
        return new FloatValue((float) value->get_the_int(), value->cst_token());
    } else {
        return nullptr;
    }
}

bool ComplexType::can_promote(Value *value) {
    return false;
}

Value *ComplexType::promote(Value *value) {
    return nullptr;
}