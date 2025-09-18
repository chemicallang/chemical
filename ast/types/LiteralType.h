// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class LiteralType : public BaseType {
public:

    BaseType* underlying;

    /**
     * constructor
     */
    constexpr LiteralType(BaseType* underlying) : underlying(underlying), BaseType(BaseTypeKind::Literal) {
        // do nothing
    }

    uint64_t byte_size(bool is64Bit) final {
        return underlying->byte_size(is64Bit);
    }

    bool satisfies(Value* value, bool assignment) final;

    bool is_same(BaseType *type) final {
        return type->kind() == BaseTypeKind::Literal && ((LiteralType*) type)->underlying->is_same(underlying);
    }

    [[nodiscard]]
    LiteralType* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<LiteralType>()) LiteralType(underlying->copy(allocator));
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) final {
        return underlying->llvm_type(gen);
    }
#endif

};