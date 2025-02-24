// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class LiteralType : public BaseType {
public:

    BaseType* underlying;

    explicit LiteralType(BaseType* underlying, SourceLocation location) : underlying(underlying), BaseType(BaseTypeKind::Literal, location) {
        // do nothing
    }

    uint64_t byte_size(bool is64Bit) final {
        return underlying->byte_size(is64Bit);
    }

    bool satisfies(ASTAllocator& allocator, Value* value, bool assignment) final;

    bool is_same(BaseType *type) final {
        return type->kind() == BaseTypeKind::Literal && ((LiteralType*) type)->underlying->is_same(underlying);
    }

    [[nodiscard]]
    LiteralType* copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<LiteralType>()) LiteralType(underlying->copy(allocator), encoded_location());
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) final {
        return underlying->llvm_type(gen);
    }
#endif

};