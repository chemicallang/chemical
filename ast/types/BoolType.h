// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class BoolType : public TokenizedBaseType {
public:

    static const BoolType instance;

    using TokenizedBaseType::TokenizedBaseType;

    uint64_t byte_size(bool is64Bit) override {
        return 1;
    }

    bool satisfies(ValueType type) override {
        return type == ValueType::Bool;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::Bool;
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Bool;
    }

    bool satisfies(Value *value) override;

    bool is_same(BaseType *type) override {
        return type->kind() == kind();
    }

    virtual BoolType* copy(ASTAllocator& allocator) const {
        return new (allocator.allocate<BoolType>()) BoolType(token);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    clang::QualType clang_type(clang::ASTContext &context) override;

#endif

};