// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class LiteralType : public BaseType {
public:

    std::unique_ptr<BaseType> underlying;

    explicit LiteralType(std::unique_ptr<BaseType> underlying) : underlying(std::move(underlying)) {
        // do nothing
    }

    uint64_t byte_size(bool is64Bit) override {
        return underlying->byte_size(is64Bit);
    }

    bool satisfies(ValueType type) const override {
        return underlying->satisfies(type);
    }

    bool satisfies(Value *value) override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::Literal;
    }

    ValueType value_type() const override {
        return underlying->value_type();
    }

    bool is_same(BaseType *type) const override {
        return type->kind() == BaseTypeKind::Literal && ((LiteralType*) type)->underlying->is_same(underlying.get());
    }

    virtual BaseType* copy() const {
        return new LiteralType(std::unique_ptr<BaseType>(underlying->copy()));
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) override {
        return underlying->llvm_type(gen);
    }
#endif

};