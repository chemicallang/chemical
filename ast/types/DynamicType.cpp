// Copyright (c) Qinetik 2024.

#include "DynamicType.h"

DynamicType::DynamicType(BaseType* referenced, CSTToken* token) : referenced(referenced), TokenizedBaseType(token) {

}

bool DynamicType::link(SymbolResolver &linker) {
    return referenced->link(linker);
}

bool DynamicType::satisfies(ASTAllocator &allocator, Value *value) {
    return referenced->satisfies(allocator, value);
}