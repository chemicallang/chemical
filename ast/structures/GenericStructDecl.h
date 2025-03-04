// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "BaseGenericDecl.h"
#include "ast/structures/StructDefinition.h"

class GenericStructDecl : public BaseGenericDecl {
public:

    /**
     * master implementation is the first implementation we encounter
     */
    StructDefinition* master_impl;

    /**
     * these are concrete instantiations
     */
    std::vector<StructDefinition*> instantiations;

    /**
     * constructor
     */
    GenericStructDecl(
        StructDefinition* master_impl,
        ASTNode* parent_node,
        SourceLocation location
    ) : BaseGenericDecl(ASTNodeKind::GenericStructDecl, parent_node, location), master_impl(master_impl) {

    }

    void declare_top_level(SymbolResolver &linker, ASTNode *&node_ptr) override;

    void declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) override;

#ifdef COMPILER_BUILD

    void code_gen_declare(Codegen &gen) override;

    void code_gen(Codegen &gen) override;

#endif

};

