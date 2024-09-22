// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/structures/Scope.h"

struct InitBlockInitializerValue {
    bool is_inherited_type;
    Value* value;
};

class InitBlock : public ASTNode {
public:

    Scope scope;
    ASTNode* parent_node;
    CSTToken* token;
    std::unordered_map<std::string, InitBlockInitializerValue> initializers;
    // the struct container for which init block is for
    ExtendableMembersContainerNode* container;
    // the function in which init block appears
    FunctionDeclaration* func_decl;

    InitBlock(Scope scope, ASTNode* parent_node, CSTToken* token);

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::InitBlock;
    }

    ASTNode* parent() override {
        return parent_node;
    }

    CSTToken* cst_token() override {
        return token;
    }

    void declare_and_link(SymbolResolver &linker) override;

    bool diagnose_missing_members_for_init(ASTDiagnoser& diagnoser);

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

#endif

};