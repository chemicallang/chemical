// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class DoubleType : public GlobalBaseType {
public:

    /**
     * constructor
     */
    constexpr DoubleType() : GlobalBaseType(BaseTypeKind::Double) {

    }

    uint64_t byte_size(TargetData& target) final {
        return 8;
    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::Double;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

};