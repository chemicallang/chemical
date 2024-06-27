// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"
#include "ast/base/Value.h"

class ReferencedValueType: public BaseType {
public:

    std::unique_ptr<Value> value;

    explicit ReferencedValueType(std::unique_ptr<Value> value) : value(std::move(value)) {

    }

    uint64_t byte_size(bool is64Bit) override {
        return value->byte_size(is64Bit);
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

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

    bool satisfies(ValueType value_type) const override {
        return value->get_base_type()->satisfies(value_type);
    }

    BaseTypeKind kind() const override {
        return value->get_base_type()->kind();
    }

    bool is_same(BaseType *other) const override {
        return value->get_base_type()->is_same(other);
    }

    virtual BaseType *copy() const {
        return new ReferencedValueType(std::unique_ptr<Value>(value->copy()));
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override {
        return value->get_base_type()->llvm_type(gen);
    }

#endif

};