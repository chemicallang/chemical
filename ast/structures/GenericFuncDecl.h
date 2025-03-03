// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "BaseGenericDecl.h"
#include "ast/structures/FunctionDeclaration.h"

class GenericFuncDecl : public BaseGenericDecl {
public:

    /**
     * master implementation is the first implementation we encounter
     */
    FunctionDeclaration* master_impl;

    /**
     * these are concrete instantiations
     */
    std::vector<FunctionDeclaration*> instantiations;

    /**
     * constructor
     */
    GenericFuncDecl(
        FunctionDeclaration* master_impl,
        ASTNode* parent_node,
        SourceLocation location
    ) : BaseGenericDecl(ASTNodeKind::GenericFuncDecl, parent_node, location), master_impl(master_impl) {

    }

    void declare_top_level(SymbolResolver &linker, ASTNode *&node_ptr) override;

    void declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) override;

    /**
     * a call notifies a function, during symbol resolution that it exists
     * when this happens, generics are checked, proper types are registered in generic
     * @return iteration that corresponds to this call
     */
    FunctionDeclaration* instantiate_call(
            ASTAllocator& astAllocator,
            ASTDiagnoser& diagnoser,
            FunctionCall* call,
            BaseType* expected_type
    );

    /**
     * a call notifies a function, during symbol resolution that it exists
     * when this happens, generics are checked, proper types are registered in generic
     * @return iteration that corresponds to this call
     */
    FunctionDeclaration* instantiate_call(
        SymbolResolver& resolver,
        FunctionCall* call,
        BaseType* expected_type
    );

#ifdef COMPILER_BUILD

    void code_gen_declare(Codegen &gen) override;

    void code_gen(Codegen &gen) override;

#endif

};

