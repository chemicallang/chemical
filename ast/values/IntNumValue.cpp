// Copyright (c) Qinetik 2024.

#include "IncDecValue.h"
#include "ast/base/InterpretScope.h"
#include "ast/values/ShortValue.h"

Value* IncDecValue::evaluated_value(InterpretScope &scope) {
    const auto val = new (scope.allocate<ShortValue>()) ShortValue(1, location);
    value->set_value(scope, val, increment ? Operation::Addition : Operation::Subtraction, location);
    // TODO support post and pre properly
    return value->evaluated_value(scope);
}