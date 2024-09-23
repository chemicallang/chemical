// Copyright (c) Qinetik 2024.

#include "NullValue.h"
#include "ast/types/PointerType.h"
#include "ast/types/VoidType.h"
#include "compiler/SymbolResolver.h"

bool NullValue::link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type) {
    if(linker.safe_context) {
        linker.error("null value can only be used in unsafe context", this);
        return false;
    }
    return true;
}

BaseType* NullValue::create_type(ASTAllocator &allocator) {
    return new (allocator.allocate<PointerType>()) PointerType(new (allocator.allocate<VoidType>()) VoidType(nullptr), nullptr);
}

BaseType* NullValue::known_type() {
    return (BaseType*) &PointerType::void_ptr_instance;
}