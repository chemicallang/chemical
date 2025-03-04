// Copyright (c) Chemical Language Foundation 2025.

#include "NumberValue.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/Return.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/StructValue.h"
#include "ast/types/FunctionType.h"
#include "ast/values/IntValue.h"
#include "compiler/SymbolResolver.h"

IntNType* get_int_n_type(BaseType* type) {
    if(!type) return nullptr;
    const auto pure = type->pure_type();
    return pure->as_intn_type();
}

unsigned int NumberValue::get_num_bits() {
    if(linked_type == nullptr) {
        return 32;
    } else {
        const auto t = get_int_n_type(linked_type);
        return t ? t->num_bits() : 32;
    }
}

bool NumberValue::is_unsigned() {
    if(linked_type == nullptr) {
        return false;
    } else {
        const auto t = get_int_n_type(linked_type);
        return t != nullptr && t->is_unsigned();
    }
}

Value* NumberValue::evaluated_value(InterpretScope &scope) {
    if(linked_type) {
        const auto t = get_int_n_type(linked_type);
        return t ? t->create(scope.allocator, value) : this;
    } else {
        return this;
    }
}

BaseType* NumberValue::known_type() {
    const auto t = get_int_n_type(linked_type);
    return t ? t : (BaseType*) &IntType::instance;
}

bool NumberValue::link(SymbolResolver &linker, BaseType *type) {
    if(type) {
        const auto linked = type->linked_node();
        if(linked) {
            const auto param = linked->as_generic_type_param();
            if(param) {
                const auto known = param->known_type();
                if(known) {
                    linked_type = known->copy(*linker.ast_allocator);
                } else {
                    linked_type = type->copy(*linker.ast_allocator);
                }
                return true;
            }
        }
        auto pure = type->pure_type(linker.allocator);
        if(pure) {
            const auto pure_kind = pure->kind();
            if(pure_kind == BaseTypeKind::IntN) {
                linked_type = pure->copy(*linker.ast_allocator);
            }
        }
    }
    return true;
}