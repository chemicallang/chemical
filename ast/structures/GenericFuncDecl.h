// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "BaseGenericDecl.h"
#include "ast/structures/FunctionDeclaration.h"
#include "compiler/generics/GenInstantiatorAPI.h"

class GenericFuncDecl : public BaseGenericDecl {
public:

    /**
     * finalize the signature of the given function, it just means copy the signature in place
     */
    static void finalize_signature(ASTAllocator& allocator, FunctionDeclaration* decl);

    /**
     * finalize the body of the given function, it just means copy the body in place
     */
    static void finalize_body(ASTAllocator& allocator, FunctionDeclaration* decl);

    /**
     * master implementation is the first implementation we encounter
     */
    FunctionDeclaration* master_impl;

    /**
     * these are concrete instantiations
     */
    std::vector<FunctionDeclaration*> instantiations;

    /**
     * this allows us to know how many instantiations we've generated code for
     * so in the other modules we do not generate code for them, just declare them
     */
    unsigned total_bodied_instantiations = 0;

    /**
     * we set this to true, after link_signature call
     */
    bool signature_linked = false;

    /**
     * we set this to true, after declare_and_link call
     */
    bool body_linked = false;

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

    void link_signature(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) override;

    BaseType* create_value_type(ASTAllocator &allocator) override {
        return master_impl->create_value_type(allocator);
    }

    BaseType* known_type() override {
        return master_impl->known_type();
    }

    /**
     * a call notifies a function, during symbol resolution that it exists
     * when this happens, generics are checked, proper types are registered in generic
     * @return iteration that corresponds to this call
     */
    FunctionDeclaration* instantiate_call(
            GenericInstantiatorAPI& instantiator,
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

    void code_gen_external_declare(Codegen &gen) override;

#endif

};

