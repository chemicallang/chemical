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
    if(expected_type) {
        const auto kind = expected_type->kind();
        if(kind == BaseTypeKind::Function || kind == BaseTypeKind::Pointer) {
            expected = expected_type->copy(*linker.ast_allocator);
        }
    }
    return true;
}

BaseType* NullValue::create_type(ASTAllocator &allocator) {
    return expected ? expected : (
        new (allocator.allocate<PointerType>()) PointerType(new (allocator.allocate<VoidType>()) VoidType(encoded_location()), encoded_location())
    );
}

BaseType* NullValue::known_type() {
    return expected ? expected : (PointerType*) &PointerType::void_ptr_instance;
}