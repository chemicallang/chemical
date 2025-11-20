// Copyright (c) Chemical Language Foundation 2025.

#include "SizeOfValue.h"
#include "AlignOfValue.h"
#include "ast/base/BaseType.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/values/IntNumValue.h"

Value* SizeOfValue::evaluated_value(InterpretScope &scope) {
    return new (scope.allocate<IntNumValue>()) IntNumValue(for_type->byte_size(scope.global->target_data), scope.global->typeBuilder.getU64Type(), encoded_location());
}

Value* AlignOfValue::evaluated_value(InterpretScope &scope) {
    return new (scope.allocate<IntNumValue>()) IntNumValue(for_type->type_alignment(scope.global->target_data), scope.global->typeBuilder.getU64Type(), encoded_location());
}