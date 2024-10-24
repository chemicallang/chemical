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
    return new (scope.allocate<IntValue>()) IntValue((int) value, token);
}

IntNType* linked(BaseType* type) {
    auto pure = type->pure_type();
    if(pure && pure->kind() == BaseTypeKind::IntN) {
        return (IntNType*) pure;
    } else {
        return nullptr;
    }
}

bool NumberValue::link(SymbolResolver &linker, BaseType *type) {
    if(type) {
        const auto linked = type->linked_node();
        if(linked) {
            const auto param = linked->as_generic_type_param();
            if(param && param->active_iteration < 0) {
                if(param->def_type) {
                    linked_type = (IntNType*) param->def_type;
                }
                return true;
            }
        }
        auto pure = type->pure_type();
        if(pure && pure->kind() == BaseTypeKind::IntN) {
            linked_type = (IntNType*) pure;
        }
    }
    return true;
}