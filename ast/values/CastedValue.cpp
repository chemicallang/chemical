// Copyright (c) Chemical Language Foundation 2025.

#include "CastedValue.h"
#include "ast/types/IntNType.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "IntNumValue.h"
#include "ast/values/PointerValue.h"
#include "ast/types/PointerType.h"

ASTNode *CastedValue::linked_node() {
    return getType()->linked_node();
}

Value* CastedValue::evaluated_value(InterpretScope &scope) {
    const auto eval = value->evaluated_value(scope);
    if(!eval) {
        return nullptr;
    }
    const auto pure = getType()->pure_type(scope.allocator);
    const auto pure_kind = pure->kind();
    switch(pure_kind) {
        case BaseTypeKind::IntN: {
            const auto intNType = pure->as_intn_type_unsafe();
            if(eval->is_value_int_n()) {
                return intNType->create(scope.allocator, scope.global->typeBuilder, ((IntNumValue*) eval)->get_num_value(), encoded_location());
            } else {
                // TODO: cannot error out, we are returning intrinsics::wrap with a cast to integer
                // scope.error("non integer value cannot be casted to integer type", this);
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