// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class NullPtrType : public GlobalBaseType {
public:

    NullPtrType() : GlobalBaseType(BaseTypeKind::NullPtr) {

    }

    bool satisfies(BaseType *type) override {
        return type->kind() == BaseTypeKind::NullPtr;
    }

#ifdef COMPILER_BUILD
    llvm::Type* llvm_type(Codegen &gen) override;
#endif

};