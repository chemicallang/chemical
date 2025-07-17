// Copyright (c) Chemical Language Foundation 2025.

#include "IncDecValue.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/values/ShortValue.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/types/ReferenceType.h"

Value* IncDecValue::evaluated_value(InterpretScope &scope) {
    const auto val = new (scope.allocate<ShortValue>()) ShortValue(1, scope.global->typeBuilder.getShortType(), encoded_location());
    value->set_value(scope, val, increment ? Operation::Addition : Operation::Subtraction, encoded_location());
    // TODO support post and pre properly
    return value->evaluated_value(scope);
}

BaseType* IncDecValue::determine_type() {
    const auto type = value->getType();
    const auto pure = type->canonical();
    if(pure->kind() == BaseTypeKind::Reference) {
        const auto ref = pure->as_reference_type_unsafe();
        const auto ref_type = ref->type->canonical();
        if(BaseType::isLoadableReferencee(ref_type->kind())) {
            return ref_type;
        } else {
            return ref;
        }
    } else {
        return type;
    }
}

BaseType* IncDecValue::create_type(ASTAllocator &allocator) {
    const auto type = value->create_type(allocator);
    const auto pure = type->canonical();
    if(pure && pure->kind() == BaseTypeKind::Reference) {
        const auto ref = pure->as_reference_type_unsafe();
        const auto ref_type = ref->type->canonical();
        if(BaseType::isLoadableReferencee(ref_type->kind())) {
            return ref_type;
        } else {
            return ref;
        }
    } else {
        return pure;
    }
}