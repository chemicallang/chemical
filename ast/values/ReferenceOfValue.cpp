// Copyright (c) Chemical Language Foundation 2026.

//
// Created by wakaztahir on 6/4/26.
//

#include "ReferenceOfValue.h"

#include "ast/base/InterpretScope.h"

Value* ReferenceOfValue::evaluated_value(InterpretScope& scope) {
    const auto ev = value->evaluated_value(scope);
    return ev;
}
