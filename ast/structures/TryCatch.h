// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "Scope.h"
#include "ast/base/BaseType.h"
#include <optional>

using catch_var_type = std::optional<std::pair<std::string, BaseType*>>;

class TryCatch : public ASTNode {
public:

    FunctionCall* tryCall;
    catch_var_type catchVar;
    std::optional<Scope> catchScope;
    ASTNode* parent_node;

    /**
     * constructor
     */
    TryCatch(
            FunctionCall* tryCall,
            catch_var_type catchVar,
            ASTNode* parent_node,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::TryStmt, location), tryCall(tryCall), catchVar(std::move(catchVar)), parent_node(parent_node) {

    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }


    ASTNode *parent() final {
        return parent_node;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

};