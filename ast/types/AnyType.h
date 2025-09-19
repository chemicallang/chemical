// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class AnyType : public GlobalBaseType {
public:

    inline constexpr AnyType() : GlobalBaseType(BaseTypeKind::Any) {

    }

    bool satisfies(Value* value, bool assignment) final {
        return true;
    }

    bool satisfies(BaseType *type) final {
        return true;
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) final;
#endif

};