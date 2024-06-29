// Copyright (c) Qinetik 2024.

#include "ast/base/Value.h"
#include "ArrayType.h"
#include "BoolType.h"
#include "CharType.h"
#include "UCharType.h"
#include "DoubleType.h"
#include "FloatType.h"
#include "PointerType.h"
#include "ReferencedType.h"
#include "ReferencedValueType.h"
#include "StringType.h"
#include "LiteralType.h"

bool ArrayType::satisfies(Value *value) {
    return value->get_base_type()->is_same(this);
}

bool BoolType::satisfies(Value *value) {
    return value->value_type() == ValueType::Bool;
}

bool CharType::satisfies(Value *value) {
    return value->value_type() == ValueType::Char;
}

bool UCharType::satisfies(Value *value) {
    return value->value_type() == ValueType::UChar;
}

bool DoubleType::satisfies(Value *value) {
    return value->value_type() == ValueType::Double;
}

bool FloatType::satisfies(Value *value) {
    return value->value_type() == ValueType::Float;
}

bool PointerType::satisfies(Value *value) {
    return value->value_type() == ValueType::Pointer;
}

bool ReferencedType::satisfies(Value *value) {
    return value->get_base_type()->is_same(this);
}

bool ReferencedValueType::satisfies(Value *current_value) {
    return current_value->get_base_type()->is_same(this);
}

bool StringType::satisfies(Value *value) {
    return value->value_type() == ValueType::String;
}

bool LiteralType::satisfies(Value *value) {
    return !value->reference() && underlying->satisfies(value);
}