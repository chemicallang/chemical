// Copyright (c) Qinetik 2024.

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
    ASTNode* parent_node;
    SourceLocation location;
    std::unordered_map<chem::string_view, InitBlockInitializerValue> initializers;
    // the struct container for which init block is for
    ExtendableMembersContainerNode* container;
    // the function in which init block appears
    FunctionDeclaration* func_decl;

    InitBlock(
        Scope scope,
        ASTNode* parent_node,
        SourceLocation location
    ) : scope(std::move(scope)), parent_node(parent_node), location(location) {

    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::InitBlock;
    }

    ASTNode* parent() final {
        return parent_node;
    }

    SourceLocation encoded_location() final {
        return location;
    }

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    bool diagnose_missing_members_for_init(ASTDiagnoser& diagnoser);

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

};