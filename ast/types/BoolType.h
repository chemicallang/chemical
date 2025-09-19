// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class BoolType : public GlobalBaseType {
public:

    /**
     * constructor
     */
    constexpr BoolType() : GlobalBaseType(BaseTypeKind::Bool) {

    }

    uint64_t byte_size(bool is64Bit) final {
        return 1;
    }

    bool satisfies(BaseType *type) final;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

};