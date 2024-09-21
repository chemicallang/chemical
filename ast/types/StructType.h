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

    virtual std::string get_runtime_name() = 0;

    virtual int16_t get_generic_iteration() = 0;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::Struct;
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Struct;
    }

    bool equals(StructType *type);

    bool is_same(BaseType *type) override {
        return kind() == type->kind() && equals(static_cast<StructType *>(type));
    }

    [[nodiscard]]
    BaseType *copy(ASTAllocator& allocator) const override = 0;

    bool satisfies(ValueType type) override {
        return type == ValueType::Struct;
    }

#ifdef COMPILER_BUILD

    virtual llvm::StructType* llvm_stored_type() {
        return nullptr;
    }

    virtual void llvm_store_type(llvm::StructType* type) {
        // does not store by default
    }

    llvm::Type *with_elements_type(Codegen &gen, const std::vector<llvm::Type *>& elements, const std::string& runtime_name);

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_param_type(Codegen &gen) override;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) override;

#endif

};