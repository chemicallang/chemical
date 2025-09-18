// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class AnyType : public BaseType {
public:

    inline constexpr AnyType() : BaseType(BaseTypeKind::Any) {

    }

    bool satisfies(Value* value, bool assignment) final {
        return true;
    }

    bool satisfies(BaseType *type) final {
        return true;
    }

    bool is_same(BaseType *type) final {
        return true;
    }

    [[nodiscard]]
    AnyType* copy(ASTAllocator& allocator) final {
        // why does this return itself (without copying)
        // because the type exists in type builder
        // it is initialized once in the type builder
        // this will never be copied
        return this;
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) final;
#endif

};