// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/structures/Scope.h"
#include <unordered_map>

struct InitBlockInitializerValue {
    bool is_inherited_type;
    Value* value;
};

class InitBlock : public ASTNode {
public:

    Scope scope;
    std::unordered_map<chem::string_view, InitBlockInitializerValue> initializers;
    // the struct container for which init block is for
    ExtendableMembersContainerNode* container;
    // the function in which init block appears
    FunctionDeclaration* func_decl;

    /**
     * constructor
     */
    InitBlock(
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::InitBlock, parent_node, location), scope(this, location) {

    }

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    bool diagnose_missing_members_for_init(ASTDiagnoser& diagnoser);

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

};