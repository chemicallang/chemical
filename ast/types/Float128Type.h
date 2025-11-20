// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class Float128Type : public GlobalBaseType {
public:

    /**
     * constructor
     */
    constexpr Float128Type() : GlobalBaseType(BaseTypeKind::Float128) {

    }

    uint64_t byte_size(TargetData& target) final {
        return 16;
    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::Float128;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

};