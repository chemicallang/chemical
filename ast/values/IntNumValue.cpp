// Copyright (c) Chemical Language Foundation 2025.

#include "IncDecValue.h"
#include "ast/base/InterpretScope.h"
#include "ast/values/ShortValue.h"
#include "ast/types/ReferenceType.h"

Value* IncDecValue::evaluated_value(InterpretScope &scope) {
    const auto val = new (scope.allocate<ShortValue>()) ShortValue(1, encoded_location());
    value->set_value(scope, val, increment ? Operation::Addition : Operation::Subtraction, encoded_location());
    // TODO support post and pre properly
    return value->evaluated_value(scope);
}

BaseType* IncDecValue::create_type(ASTAllocator &allocator) {
    const auto type = value->create_type(allocator);
    const auto pure = type->pure_type();
    if(pure && pure->kind() == BaseTypeKind::Reference) {
        const auto ref = pure->as_reference_type_unsafe();
        if(BaseType::isLoadableReferencee(ref->type->kind())) {
            return ref->type;
        } else {
            return ref;
        }
    } else {
        return pure;
    }
}