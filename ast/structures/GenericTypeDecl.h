// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/structures/BaseGenericDecl.h"
#include "ast/statements/Typealias.h"
#include "compiler/generics/GenInstantiatorAPI.h"

class GenericTypeDecl : public BaseGenericDecl {
public:

    /**
     * finalize the signature of the struct decl, it means copy the signature in place
     */
    static void finalize_signature(ASTAllocator& allocator, TypealiasStatement* def);

    /**
     * master implementation is the first implementation we encounter
     */
    TypealiasStatement* master_impl;

    /**
     * these are concrete instantiations, however before link_signature and before declare_and_link, we only create
     * shallow copies of the master implementation, which has no symbols linked and use that
     */
    std::vector<TypealiasStatement*> instantiations;

    /**
     * how manu instantiations we've declared in this module
     */
    unsigned total_declared_instantiations = 0;

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
     * constructor
     */
    GenericTypeDecl(
            TypealiasStatement* master_impl,
            ASTNode* parent_node,
            SourceLocation location
    ) : BaseGenericDecl(ASTNodeKind::GenericTypeDecl, parent_node, location), master_impl(master_impl) {

    }

    BaseType* known_type() override {
        return master_impl->known_type();
    }

    /**
     * register generic args
     */
    TypealiasStatement* register_generic_args(GenericInstantiatorAPI& instantiator, std::vector<TypeLoc>& types);

    TypealiasStatement* instantiate_type(GenericInstantiatorAPI& instantiator, std::vector<TypeLoc>& types);

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override {

    }

#endif

};

