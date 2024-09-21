// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class StringType : public TokenizedBaseType {
public:

    static const StringType instance;

    using TokenizedBaseType::TokenizedBaseType;

    [[nodiscard]]
    BaseType* create_child_type(ASTAllocator& allocator) const override;

//    hybrid_ptr<BaseType> get_child_type() override;

    BaseType* known_child_type() override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool satisfies(ValueType type) override {
        return type == ValueType::String;
    }

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::String;
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::String;
    }

    bool satisfies(Value *value) override;

    bool is_same(BaseType *type) override {
        return type->kind() == kind();
    }

    [[nodiscard]]
    StringType *copy(ASTAllocator& allocator) const override {
        return new (allocator.allocate<StringType>()) StringType(token);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

#endif

};