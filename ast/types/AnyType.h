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

    bool satisfies(Value *value) override {
        return true;
    }

    bool satisfies(BaseType *type) override {
        return true;
    }

    bool is_same(BaseType *type) override {
        return true;
    }

    [[nodiscard]]
    AnyType* copy() const override {
        return new AnyType(token);
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) override;
#endif

};