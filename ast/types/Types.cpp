// Copyright (c) Qinetik 2024.

#include "ast/base/Value.h"
#include "ArrayType.h"
#include "AnyType.h"
#include "BoolType.h"
#include "CharType.h"
#include "UCharType.h"
#include "DoubleType.h"
#include "FloatType.h"
#include "PointerType.h"
#include "LinkedType.h"
#include "LinkedValueType.h"
#include "StringType.h"
#include "LiteralType.h"
#include "VoidType.h"

const AnyType AnyType::instance(nullptr);
const BoolType BoolType::instance(nullptr);
const CharType CharType::instance(nullptr);
const DoubleType DoubleType::instance(nullptr);
const FloatType FloatType::instance(nullptr);
const StringType StringType::instance(nullptr);
const UCharType UCharType::instance(nullptr);
const VoidType VoidType::instance(nullptr);

bool ArrayType::satisfies(Value *value) {
    if(value->value_type() != ValueType::Array) return false;
    const auto pure_type = value->get_pure_type();
    if(pure_type->kind() != BaseTypeKind::Array) return false;
    const auto arr_type = (ArrayType*) pure_type.get();
    if(array_size != -1 && arr_type->array_size != -1 && array_size != arr_type->array_size) return false;
    // can't get array element type, because array is empty probably and has no type declaration to lean on
    if(!arr_type->elem_type) return true;
    return elem_type->satisfies(arr_type->elem_type.get());
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
    return value->value_type() == ValueType::Pointer || value->is_pointer() && value->get_child_type()->satisfies(ValueType::Char);
}

bool LinkedType::satisfies(Value *value) {
    return value->get_base_type()->is_same(this);
}

bool LinkedValueType::satisfies(Value *current_value) {
    return current_value->get_base_type()->is_same(this);
}

bool StringType::satisfies(Value *value) {
    return value->value_type() == ValueType::String;
}

bool LiteralType::satisfies(Value *value) {
    return !value->reference() && underlying->satisfies(value);
}