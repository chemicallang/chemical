// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class FloatType : public GlobalBaseType {
public:

    /**
     * constructor
     */
    constexpr FloatType() : GlobalBaseType(BaseTypeKind::Float) {

    }

    uint64_t byte_size(TargetData& targetData) final {
        return 4;
    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::Float;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

};