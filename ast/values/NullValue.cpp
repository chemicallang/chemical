// Copyright (c) Chemical Language Foundation 2025.

#include "NullValue.h"
#include "ast/types/PointerType.h"
#include "ast/types/VoidType.h"
#include "ast/types/NullPtrType.h"
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
    return new (allocator.allocate<NullPtrType>()) NullPtrType();
}

BaseType* NullValue::known_type() {
    return (NullPtrType*) &NullPtrType::instance;
}