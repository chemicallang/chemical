// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class LongDoubleType : public GlobalBaseType {
public:

    /**
     * constructor
     */
    constexpr LongDoubleType() : GlobalBaseType(BaseTypeKind::LongDouble) {

    }

    uint64_t byte_size(TargetData& target) final {
        return 16;
    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::LongDouble;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

};