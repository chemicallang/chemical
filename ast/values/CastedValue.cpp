// Copyright (c) Chemical Language Foundation 2025.

#include "CastedValue.h"
#include "ast/types/IntNType.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "IntNumValue.h"
#include "ast/values/PointerValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/types/PointerType.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"

ASTNode *CastedValue::linked_node() {
    return getType()->linked_node();
}

Value* CastedValue::evaluated_value(InterpretScope &scope) {
    const auto eval = value->evaluated_value(scope);
    if(!eval) {
        return nullptr;
    }
    const auto pure = getType()->canonical();
    const auto pure_kind = pure->kind();
    switch(pure_kind) {
        case BaseTypeKind::IntN: {
            const auto intNType = pure->as_intn_type_unsafe();
            // A cast to a narrower integer type must truncate (and sign extend for
            // signed types), e.g. `300 as u8 == 44`. coerce_to_type normalizes the
            // freshly created value to the target width.
            const auto make_int = [&](uint64_t value) {
                return scope.coerce_to_type(
                    intNType->create(scope.allocator, scope.global->typeBuilder, value, encoded_location()),
                    pure
                );
            };
            if(eval->is_value_int_n()) {
                return make_int(((IntNumValue*) eval)->get_num_value());
            } else {
                // Handle Float/Double to Integer cast
                const auto eval_kind = eval->val_kind();
                if(eval_kind == ValueKind::Float) {
                    const auto floatVal = eval->as_float_unsafe();
                    return make_int((uint64_t)(int64_t)floatVal->value);
                } else if(eval_kind == ValueKind::Double) {
                    const auto doubleVal = eval->as_double_unsafe();
                    return make_int((uint64_t)(int64_t)doubleVal->value);
                } else if(eval_kind == ValueKind::PointerValue) {
                    const auto ptrVal = (PointerValue*) eval;
                    return make_int((uint64_t)(uintptr_t)ptrVal->data);
                }
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
                case ValueKind::IntN: {
                    // Casting integer to pointer: store the integer value as the pointer data
                    const auto numVal = eval->get_number();
                    if(numVal.has_value()) {
                        return new (scope.allocate<PointerValue>()) PointerValue(
                            (void*)(uintptr_t) numVal.value(), ptrType->type, 0, 0, encoded_location()
                        );
                    }
                    scope.error("could not cast integer value to pointer", this);
                    return eval;
                }
                case ValueKind::WrapValue:
                    // currently we shouldn't error out here, we use this
                    // maybe in the future, we should verify the underlying value
                    return eval;
                default:
                    // in codegen mode (outside interpretation), the backends
                    // translate the casted expression (with the cast preserved)
                    // directly at the codegen site: a runtime variable reference
                    // (Identifier that couldn't be resolved to a value) or a
                    // %runtime_value / %runtime_block_value cannot be evaluated
                    // to a pointer at comptime, so keep the cast instead of
                    // erroring out on the unevaluated expression
                    if(!scope.global->interpretation_mode &&
                       (value->val_kind() == ValueKind::RuntimeValue || value->val_kind() == ValueKind::RuntimeBlockValue ||
                        eval->val_kind() == ValueKind::Identifier)) {
                        return this;
                    }
                    scope.error("unknown value being casted to a pointer", this);
                    return eval;
            }
        }
        default:
            return eval;
    }
}