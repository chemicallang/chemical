// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

class VariantMemberParam;

class VariantCaseVariable : public ASTNode {
public:

    std::string name;
    VariantCase* variant_case;
    VariantMemberParam* member_param;

    /**
     * variant case
     */
    VariantCaseVariable(std::string name, VariantCase* variant_case);

    void accept(Visitor *visitor) override;

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    ASTNodeKind kind() override {
        return ASTNodeKind::VariantCaseVariable;
    }

    ASTNode* parent() override;

    hybrid_ptr<BaseType> get_value_type() override;

    std::unique_ptr<BaseType> create_value_type() override;

    BaseType* known_type() override;

    ASTNode* child(const std::string &name) override;

    int child_index(const std::string &name) override;

    ASTNode* child(int index) override;

    std::pair<BaseType*, int16_t> set_iteration();

    bool is_generic_param();

#ifdef COMPILER_BUILD

    llvm::Value* llvm_pointer(Codegen &gen) override;

    llvm::Value* llvm_load(Codegen &gen) override;

    llvm::Type* llvm_type(Codegen &gen) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

#endif

};