// Copyright (c) Qinetik 2024.

#include "CastedValue.h"
#include "ast/types/IntNType.h"
#include "ast/base/InterpretScope.h"
#include "IntNumValue.h"
#include "ast/values/PointerValue.h"
#include "ast/types/PointerType.h"

CastedValue *CastedValue::copy(ASTAllocator& allocator) {
    return new CastedValue(
        value->copy(allocator),
        type->copy(allocator),
        location
    );
}

bool CastedValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType* expected_type) {
    if(type->link(linker)) {
        if(value->link(linker, value, type)) {
            return true;
        }
    }
    return false;
}

ASTNode *CastedValue::linked_node() {
    return type->linked_node();
}

Value* CastedValue::evaluated_value(InterpretScope &scope) {
    const auto eval = value->evaluated_value(scope);
    if(!eval) {
        return nullptr;
    }
    const auto pure = type->pure_type();
    const auto pure_kind = pure->kind();
    switch(pure_kind) {
        case BaseTypeKind::IntN: {
            const auto intNType = pure->as_intn_type_unsafe();
            if(eval->is_value_int_n()) {
                return intNType->create(scope.allocator, ((IntNumValue*) eval)->get_num_value());
            } else {
                scope.error("non integer value cannot be casted to integer type", this);
                return eval;
            }
        }
        case BaseTypeKind::Pointer: {
            const auto ptrType = pure->as_pointer_type_unsafe();
            switch(eval->val_kind()) {
                case ValueKind::PointerValue:{
                    const auto ptrVal = (PointerValue*) eval;
                    return ptrVal->cast(scope, pure);
                }
                case ValueKind::String:{
                    const auto str = eval->as_string_unsafe();
                    return new (scope.allocate<PointerValue>()) PointerValue(
                        scope, str, ptrType->type
                    );
                }
                default:
                    scope.error("unknown value being casted to a pointer", this);
                    return eval;
            }
        }
        default:
            return eval;
    }
}