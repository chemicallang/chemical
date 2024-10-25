// Copyright (c) Qinetik 2024.

#include "CastedValue.h"


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