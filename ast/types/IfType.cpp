// Copyright (c) Chemical Language Foundation 2025.

#include "IfType.h"

std::optional<bool> evaluate_bool(Value* value, InterpretScope& scope) {
    const auto constant = value->evaluated_value(scope);
    if(!constant || constant->kind() != ValueKind::Bool) {
        return std::nullopt;
    }
    return constant->get_the_bool();
}

TypeLoc IfType::evaluate(InterpretScope& scope) {
    const auto evaluated = evaluate_bool(condition, scope);
    if(!evaluated.has_value()) {
        return nullptr;
    }
    if(evaluated.value()) {
        return thenType;
    }
    for(auto& pair : elseIfs) {
        const auto evaluated2 = evaluate_bool(pair.first, scope);
        if(!evaluated2.has_value()) {
            return nullptr;
        }
        if(evaluated2.value()) {
            return pair.second;
        }
    }
    return elseType;
}