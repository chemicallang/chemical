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

    VariantCaseVariable* as_variant_case_var() override {
        return this;
    }

    void accept(Visitor *visitor) override;

    void declare_and_link(SymbolResolver &linker) override;

    ASTNodeKind kind() override {
        return ASTNodeKind::VariantCaseVariable;
    }

    ASTNode* parent() override;

#ifdef COMPILER_BUILD

    llvm::Value* llvm_pointer(Codegen &gen) override;

    llvm::Value* llvm_load(Codegen &gen) override;

#endif

};