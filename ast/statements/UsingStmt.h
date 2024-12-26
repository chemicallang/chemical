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
class UsingStmt : public AnnotableNode {
public:

    AccessChain* chain;
    SourceLocation location;
    UsingStmtAttributes attrs;
    ASTNode* parent_node;
    /**
     * TODO remove this, this is for containing node identifier
     */
    std::string node_id;

    UsingStmt(
            AccessChain* chain,
            ASTNode* parent_node,
            bool is_namespace,
            SourceLocation location
    );

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

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::UsingStmt;
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

    void declare_top_level(SymbolResolver &linker) final;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final {
        // does nothing
    }

#endif

};