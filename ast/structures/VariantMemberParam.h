// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

class VariantMember;

class VariantMemberParam : public ASTNode {
public:

    std::string name;
    BaseType* type;
    Value* def_value;
    VariantMember* parent_node;
    unsigned index;
    CSTToken* token;

    VariantMemberParam(
        std::string name,
        unsigned index,
        BaseType* type,
        Value* def_value,
        VariantMember* parent_node,
        CSTToken* token
    );

    CSTToken *cst_token() override {
        return token;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::VariantMemberParam;
    }

    VariantMemberParam* copy(ASTAllocator& allocator);

    void declare_and_link(SymbolResolver &linker) override;

    uint64_t byte_size(bool is64Bit) override {
        return type->byte_size(is64Bit);
    }

    void accept(Visitor *visitor) override {
        throw std::runtime_error("VariantMemberParam::accept called");
    }

    BaseType* known_type() override {
        return type;
    }

    ASTNode* parent() override {
        return (ASTNode*) parent_node;
    }

    ASTNode* child(int index) override;

    int child_index(const std::string &name) override;

    ASTNode* child(const std::string &name) override;

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::FunctionType* llvm_func_type(Codegen &gen) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

#endif

};