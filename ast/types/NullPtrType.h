// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class NullPtrType : public BaseType {
public:

    static const NullPtrType instance;

    NullPtrType() : BaseType(BaseTypeKind::NullPtr) {

    }

    bool is_same(BaseType *type) override {
        return type->kind() == BaseTypeKind::NullPtr;
    }

    bool satisfies(BaseType *type) override {
        return type->kind() == BaseTypeKind::NullPtr;
    }

    BaseType* copy(ASTAllocator &allocator) const override {
        return new (allocator.allocate<NullPtrType>()) NullPtrType();
    }

#ifdef COMPILER_BUILD
    llvm::Type* llvm_type(Codegen &gen) override;
#endif

};