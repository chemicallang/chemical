// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class LiteralType : public TokenizedBaseType {
public:

    BaseType* underlying;

    explicit LiteralType(BaseType* underlying, CSTToken* token) : underlying(underlying), TokenizedBaseType(token) {
        // do nothing
    }

    uint64_t byte_size(bool is64Bit) override {
        return underlying->byte_size(is64Bit);
    }

    bool satisfies(ValueType type) override {
        return underlying->satisfies(type);
    }

    bool satisfies(ASTAllocator& allocator, Value* value) override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    BaseType* pure_type() override {
        return underlying;
    }

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::Literal;
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return underlying->value_type();
    }

    bool is_same(BaseType *type) override {
        return type->kind() == BaseTypeKind::Literal && ((LiteralType*) type)->underlying->is_same(underlying);
    }

    [[nodiscard]]
    LiteralType* copy(ASTAllocator& allocator) const override {
        return new (allocator.allocate<LiteralType>()) LiteralType(underlying->copy(allocator), token);
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) override {
        return underlying->llvm_type(gen);
    }
#endif

};