// Copyright (c) Qinetik 2024.

#include "NumberValue.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/Return.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/StructValue.h"
#include "ast/types/FunctionType.h"
#include "ast/values/IntValue.h"
#include "compiler/SymbolResolver.h"

unsigned int NumberValue::get_num_bits() {
    if(linked_type == nullptr) {
        return 32;
    } else {
        const auto pure = linked_type->pure_type();
        const auto num_type = pure->as_intn_type();
        if(num_type) {
            return num_type->num_bits();
        } else {
            return 32;
        }
    }
}

bool NumberValue::is_unsigned() {
    if(linked_type == nullptr) {
        return false;
    } else {
        const auto pure = linked_type->pure_type();
        const auto num_type = pure->as_intn_type();
        if(num_type) {
            return num_type->is_unsigned();
        } else {
            return false;
        }
    }
}

Value* NumberValue::evaluated_value(InterpretScope &scope) {
    if(linked_type) {
        const auto pure = linked_type->pure_type();
        const auto num_type = pure->as_intn_type();
        if(num_type) {
            return num_type->create(scope.allocator, value);
        } else {
            return this;
        }
    } else {
        return this;
    }
}

ValueType NumberValue::value_type() const {
    if(linked_type == nullptr) {
        return ValueType::Int;
    } else {
        return linked_type->value_type();
    }
}

BaseType* NumberValue::known_type() {
    if(!linked_type) {
        return (BaseType*) &IntType::instance;
    }
    return linked_type;
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
                    linked_type = param->def_type;
                }
                return true;
            }
        }
        auto pure = type->pure_type();
        if(pure) {
            const auto pure_kind = pure->kind();
            if(pure_kind == BaseTypeKind::IntN) {
                linked_type = pure->copy(*linker.ast_allocator);
            }
        }
    }
    return true;
}