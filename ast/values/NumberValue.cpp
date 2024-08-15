// Copyright (c) Qinetik 2024.

#include "NumberValue.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/Return.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/StructValue.h"
#include "ast/types/FunctionType.h"
#include "ast/values/IntValue.h"

unsigned int NumberValue::get_num_bits() {
    if(linked_type == nullptr) {
        return 32;
    } else {
        return linked_type->num_bits();
    }
}

bool NumberValue::is_unsigned() {
    if(linked_type == nullptr) {
        return false;
    } else {
        return linked_type->is_unsigned();
    }
}

ValueType NumberValue::value_type() const {
    if(linked_type == nullptr) {
        return ValueType::Int;
    } else {
        return linked_type->value_type();
    }
}

Value *NumberValue::scope_value(InterpretScope &scope) {
    return new IntValue((int) value);
}

std::unique_ptr<IntNType> linked(BaseType* type) {
    auto pure = type->pure_type();
    if(pure && pure->kind() == BaseTypeKind::IntN) {
        return std::unique_ptr<IntNType>(((IntNType*) pure->copy()));
    } else {
        return nullptr;
    }
}

void NumberValue::link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr, BaseType *type) {
    if(type) {
        auto pure = type->pure_type();
        if(pure && pure->kind() == BaseTypeKind::IntN) {
            linked_type = std::unique_ptr<IntNType>(((IntNType*) pure->copy()));
        }
    }
}