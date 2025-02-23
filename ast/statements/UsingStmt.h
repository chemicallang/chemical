// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/AnnotableNode.h"
#include "ast/values/AccessChain.h"

struct UsingStmtAttributes {

    /**
     * is 'namespace' keyword after 'using'
     */
    bool is_namespace;

    /**
     * should the symbols be propagated to other files
     */
    bool propagate;

};

/**
 * using statement just like in c++, it allows users to bring symbols in current scope
 * from namespaces
 */
class UsingStmt : public ASTNode {
public:

    AccessChain* chain;
    UsingStmtAttributes attrs;
    ASTNode* parent_node;

    UsingStmt(
            AccessChain* chain,
            ASTNode* parent_node,
            bool is_namespace,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::UsingStmt, location), chain(chain), parent_node(parent_node),
        attrs(is_namespace, false)
    {

    }

    inline bool is_namespace() {
        return attrs.is_namespace;
    }

    inline void set_is_namespace(bool value) {
        attrs.is_namespace = value;
    }

    inline bool is_propagate() {
        return attrs.propagate;
    }

    inline void set_propagate(bool value) {
        attrs.propagate = value;
    }


    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) final;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final {
        // does nothing
    }

#endif

};