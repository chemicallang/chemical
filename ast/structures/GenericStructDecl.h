// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "GenericMembersDecl.h"
#include "ast/structures/StructDefinition.h"

class GenericStructDecl : public GenericMembersDecl {
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
    ) : GenericMembersDecl(ASTNodeKind::GenericStructDecl, parent_node, location), master_impl(master_impl) {

    }

    BaseType* create_value_type(ASTAllocator &allocator) override;

    void declare_top_level(SymbolResolver &linker, ASTNode *&node_ptr) override;

    void link_signature(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) override;

    /**
     * register generic args
     */
    StructDefinition* register_generic_args(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser, std::vector<BaseType*>& types);

#ifdef COMPILER_BUILD

    void code_gen_declare(Codegen &gen) override;

    void code_gen(Codegen &gen) override;

    void code_gen_external_declare(Codegen &gen) override;

#endif

};

