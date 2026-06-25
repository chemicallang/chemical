// Copyright (c) Chemical Language Foundation 2025.

#include "DynamicValue.h"
#include "ast/base/InterpretScope.h"

Value* DynamicValue::child(InterpretScope& scope, const chem::string_view& name) {
    if(!value) return nullptr;
    // Evaluate the inner value to get the actual struct, then delegate child lookup
    auto evaluated = value->evaluated_value(scope);
    if(evaluated) {
        return evaluated->child(scope, name);
    }
    return nullptr;
}
