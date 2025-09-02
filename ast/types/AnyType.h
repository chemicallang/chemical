// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class AnyType : public BaseType {
public:

    inline constexpr AnyType() : BaseType(BaseTypeKind::Any) {

    }

    bool satisfies(ASTAllocator& allocator, Value* value, bool assignment) final {
        return true;
    }

    bool satisfies(BaseType *type) final {
        return true;
    }

    bool is_same(BaseType *type) final {
        return true;
    }

    [[nodiscard]]
    AnyType* copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<AnyType>()) AnyType();
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) final;
#endif

};