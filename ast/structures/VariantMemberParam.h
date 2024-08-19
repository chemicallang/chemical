// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

class VariantMember;

class VariantMemberParam : public ASTNode {
public:

    std::string name;
    std::unique_ptr<BaseType> type;
    std::unique_ptr<Value> def_value;
    VariantMember* parent_node;

    VariantMemberParam(
        std::string name,
        std::unique_ptr<BaseType> type,
        std::unique_ptr<Value> def_value,
        VariantMember* parent_node
    );

    VariantMemberParam* copy();

    void declare_and_link(SymbolResolver &linker) override;

    uint64_t byte_size(bool is64Bit) override {
        return type->byte_size(is64Bit);
    }

    void accept(Visitor *visitor) override {
        throw std::runtime_error("VariantMemberParam::accept called");
    }

    BaseType* known_type() override {
        return type.get();
    }

    ASTNode* parent() override {
        return (ASTNode*) parent_node;
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::FunctionType* llvm_func_type(Codegen &gen) override;

#endif

};