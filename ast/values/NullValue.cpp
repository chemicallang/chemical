// Copyright (c) Chemical Language Foundation 2025.

#include "NullValue.h"
#include "ast/types/PointerType.h"
#include "ast/types/VoidType.h"
#include "compiler/SymbolResolver.h"

bool NullValue::link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type) {
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
        new (allocator.allocate<PointerType>()) PointerType(new (allocator.allocate<VoidType>()) VoidType(), false)
    );
}

BaseType* NullValue::known_type() {
    return expected ? expected : (PointerType*) &PointerType::void_ptr_instance;
}