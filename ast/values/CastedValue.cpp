// Copyright (c) Qinetik 2024.

#include "CastedValue.h"
#include "ast/types/IntNType.h"
#include "ast/base/InterpretScope.h"
#include "IntNumValue.h"


CastedValue::CastedValue(
        Value* value,
        BaseType* type,
        SourceLocation location
) : value(value), type(type), location(location) {

}

CastedValue *CastedValue::copy(ASTAllocator& allocator) {
    return new CastedValue(
        value->copy(allocator),
        type->copy(allocator),
        location
    );
}

bool CastedValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType* expected_type) {
    value->link(linker, value);
    type->link(linker);
    return true;
}

ASTNode *CastedValue::linked_node() {
    return type->linked_node();
}

Value* CastedValue::evaluated_value(InterpretScope &scope) {
    const auto eval = value->evaluated_value(scope);
    const auto pure = type->pure_type();
    const auto pure_kind = pure->kind();
    if(pure_kind == BaseTypeKind::IntN) {
        const auto intNType = pure->as_intn_type_unsafe();
        if(eval->is_value_int_n()) {
            return intNType->create(scope.allocator, ((IntNumValue*) eval)->get_num_value());
        } else {
            scope.error("non integer value cannot be casted to integer type", this);
            return eval;
        }
    } else {
        // couldn't cast
        return eval;
    }
}