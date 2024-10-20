// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class AnyType : public TokenizedBaseType {
public:

    static const AnyType instance;

    using TokenizedBaseType::TokenizedBaseType;

    bool satisfies(ValueType type) override {
        return true;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::Any;
    }

    bool satisfies(ASTAllocator& allocator, Value* value, bool assignment) override {
        return true;
    }

    bool satisfies(BaseType *type) override {
        return true;
    }

    bool is_same(BaseType *type) override {
        return true;
    }

    [[nodiscard]]
    AnyType* copy(ASTAllocator& allocator) const override {
        return new (allocator.allocate<AnyType>()) AnyType(token);
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) override;
#endif

};