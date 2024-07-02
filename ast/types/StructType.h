// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include "ast/base/BaseType.h"
#include <memory>

class VariablesContainer;

class StructType : public BaseType {
public:

    StructType() = default;

    virtual VariablesContainer* variables_container() = 0;

    virtual std::string struct_name() {
        return "";
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::Struct;
    }

    ValueType value_type() const override {
        return ValueType::Struct;
    }

    bool equals(StructType *type);

    bool is_same(BaseType *type) override {
        return kind() == type->kind() && equals(static_cast<StructType *>(type));
    }

    virtual BaseType *copy() const = 0;

    bool satisfies(ValueType type) override {
        return type == ValueType::Struct;
    }

#ifdef COMPILER_BUILD

    virtual bool is_anonymous() {
        return true;
    }

    virtual llvm::StructType* llvm_stored_type() {
        return nullptr;
    }

    virtual void llvm_store_type(llvm::StructType* type) {
        // does not store by default
    }

    llvm::Type *with_elements_type(Codegen &gen, const std::vector<llvm::Type *>& elements, bool anonymous);

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<Value>> &values, unsigned int index) override;

#endif

};