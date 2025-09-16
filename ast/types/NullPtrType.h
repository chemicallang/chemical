// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class NullPtrType : public BaseType {
public:

    NullPtrType() : BaseType(BaseTypeKind::NullPtr) {

    }

    bool is_same(BaseType *type) override {
        return type->kind() == BaseTypeKind::NullPtr;
    }

    bool satisfies(BaseType *type) override {
        return type->kind() == BaseTypeKind::NullPtr;
    }

    BaseType* copy(ASTAllocator &allocator) final {
        // why does this return itself (without copying)
        // because the type exists in type builder
        // it is initialized once in the type builder
        // this will never be copied
        return this;
    }

#ifdef COMPILER_BUILD
    llvm::Type* llvm_type(Codegen &gen) override;
#endif

};