// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/AnnotableNode.h"
#include "ast/values/AccessChain.h"

struct UsingStmtAttributes {

    /**
     * is 'namespace' keyword after 'using'
     */
    bool is_namespace;

    /**
     * did the chain failed linking
     */
    bool failed_chain_link = false;

};

/**
 * using statement just like in c++, it allows users to bring symbols in current scope
 * from namespaces
 */
class UsingStmt : public ASTNode {
public:

    AccessChain* chain;
    UsingStmtAttributes attrs;

    constexpr UsingStmt(
            AccessChain* chain,
            ASTNode* parent_node,
            bool is_namespace,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::UsingStmt, parent_node, location), chain(chain),
        attrs(is_namespace, false)
    {

    }

    UsingStmt* copy(ASTAllocator &allocator) override {
        const auto stmt = new (allocator.allocate<UsingStmt>()) UsingStmt(
            chain->copy(allocator),
            parent(),
            false,
            encoded_location()
        );
        stmt->attrs = attrs;
        return stmt;
    }

    inline bool is_namespace() {
        return attrs.is_namespace;
    }

    inline void set_is_namespace(bool value) {
        attrs.is_namespace = value;
    }

    inline bool is_failed_chain_link() {
        return attrs.failed_chain_link;
    }

    void declare_symbols(SymbolResolver &linker);

    void link_signature(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final {
        // does nothing
    }

#endif

};