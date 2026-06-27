// Copyright (c) Chemical Language Foundation 2025.

#include "InValue.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/values/BoolValue.h"

Value* InValue::evaluated_value(InterpretScope &scope) {

    const auto expr = value->evaluated_value(scope);

    for(const auto val : values) {
        auto result = scope.evaluate(Operation::IsEqual, expr, val->evaluated_value(scope), encoded_location(), this);
        if(result && result->val_kind() == ValueKind::Bool && result->get_the_bool()) {
            return new (scope.allocate<BoolValue>()) BoolValue(!is_negating, getType(), encoded_location());
        }
    }

    return new (scope.allocate<BoolValue>()) BoolValue(is_negating, getType(), encoded_location());

}