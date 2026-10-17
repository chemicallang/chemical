// Copyright (c) Chemical Language Foundation 2025.

#include "ZeroedValue.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/values/StructValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/structures/StructDefinition.h"
#include "ast/types/ArrayType.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/NullValue.h"
#include "ast/types/IntNType.h"
#include "ast/types/FloatType.h"
#include "ast/types/DoubleType.h"

Value* ZeroedValue::child(InterpretScope& scope, const chem::string_view& name) {
    // First evaluate to get the proper struct value, then access child
    auto eval = evaluated_value(scope);
    if(eval && eval != this) {
        return eval->child(scope, name);
    }
    return nullptr;
}

