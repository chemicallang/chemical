// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"
#include "ast/base/Value.h"

class ReferencedValueType: public TokenizedBaseType {
public:

    std::unique_ptr<Value> value;

    ReferencedValueType(std::unique_ptr<Value> value, CSTToken* token) : value(std::move(value)), TokenizedBaseType(token) {

    }

    uint64_t byte_size(bool is64Bit) override {
        return value->byte_size(is64Bit);
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return value->value_type();
    }

    void link(SymbolResolver &linker, std::unique_ptr<BaseType>& current) override {
        value->link(linker, value);
        current = value->create_type();
    }

    ASTNode *linked_node() override {
        return value->linked_node();
    }

    bool satisfies(ValueType value_type) override {
        return value->get_base_type()->satisfies(value_type);
    }

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return value->get_base_type()->kind();
    }

    bool is_same(BaseType *other) override {
        return value->get_base_type()->is_same(other);
    }

    bool satisfies(Value *value) override;

    [[nodiscard]]
    ReferencedValueType *copy() const override {
        return new ReferencedValueType(std::unique_ptr<Value>(value->copy()), token);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override {
        return value->get_base_type()->llvm_type(gen);
    }

#endif

};