// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class LiteralType : public TokenizedBaseType {
public:

    BaseType* underlying;

    explicit LiteralType(BaseType* underlying, SourceLocation location) : underlying(underlying), TokenizedBaseType(location) {
        // do nothing
    }

    uint64_t byte_size(bool is64Bit) final {
        return underlying->byte_size(is64Bit);
    }

    bool satisfies(ValueType type) final {
        return underlying->satisfies(type);
    }

    bool satisfies(ASTAllocator& allocator, Value* value, bool assignment) final;

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    BaseType* pure_type() final {
        return underlying;
    }

    [[nodiscard]]
    BaseTypeKind kind() const final {
        return BaseTypeKind::Literal;
    }

    [[nodiscard]]
    ValueType value_type() const final {
        return underlying->value_type();
    }

    bool is_same(BaseType *type) final {
        return type->kind() == BaseTypeKind::Literal && ((LiteralType*) type)->underlying->is_same(underlying);
    }

    [[nodiscard]]
    LiteralType* copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<LiteralType>()) LiteralType(underlying->copy(allocator), location);
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) final {
        return underlying->llvm_type(gen);
    }
#endif

};