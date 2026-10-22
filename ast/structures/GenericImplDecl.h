// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "GenericMembersDecl.h"
#include "ast/structures/ImplDefinition.h"
#include "compiler/generics/GenInstantiatorAPI.h"

class ImplementationsIndex;
class ASTDiagnoser;
class MembersContainer;
class ASTAny;

/**
 * summary:
 * we check if is_master_linked, if not then we shallow copy struct, and store the pointer it into the vector
 * 1 - two bool variables are required signature_linked, body_linked
 *
 * when link_signature is called, it can be assumed that all the instantiations that exist up until this point are shallow copies and don't have any symbols linked
 * so in link_signature we link the signature of master implementation of struct, set signature_linked to true, then looping over instantiations we copy the signature of the struct into each one
 * copy_signature_into is called upon master implementation, which takes the instantiation struct pointer with an allocator to copy it's signature (types & values) into the instantiation
 *
 * when declare_and_link is called, we first link the master implementation completely, set body_linked to true, it can be assumed that all instantiations that exist up until this point are shallow copies with signatures linked
 * we loop over instantiataions, we call master implementation's copy_body_into with the instantiation and allocator
 */
class GenericImplDecl : public GenericMembersDecl {
public:

    /**
     * finalize the signature of the struct decl, it means copy the signature in place
     */
    static void finalize_signature(ASTAllocator& allocator, ImplDefinition* def);

    /**
     * finalize the body of the struct decl, it means copy the body in place
     */
    static void finalize_body(ASTAllocator& allocator, ImplDefinition* def);

    /**
     * master implementation is the first implementation we encounter
     */
    ImplDefinition* master_impl;

    /**
     * these are concrete instantiations, however before link_signature and before declare_and_link, we only create
     * shallow copies of the master implementation, which has no symbols linked and use that
     */
    std::vector<ImplDefinition*> instantiations;

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
     * we set this to true, after declare_and_link call
     */
    bool body_linked = false;

    /**
     * constructor
     */
    GenericImplDecl(
        ImplDefinition* master_impl,
        ASTNode* parent_node,
        SourceLocation location
    ) : GenericMembersDecl(ASTNodeKind::GenericImplDecl, parent_node, location), master_impl(master_impl) {

    }

    BaseType* known_type() {
        return master_impl->known_type();
    }

    /**
     * register generic args.
     *
     * `adopter` is the concrete target container (e.g. `vector<int>` or
     * `TypeDecoder<vector<int>>`) when this instantiation is demanded by a
     * resolution path. The concrete impl is adopted into it and indexed once its
     * signature is finalized. Passing null only registers/finalizes the type.
     */
    ImplDefinition* register_generic_args(
        GenericInstantiatorAPI& instantiator,
        std::vector<TypeLoc>& types,
        InstantiationRequirement requirement,
        MembersContainer* adopter = nullptr,
        SourceLocation demand_loc = 0
    );

    ImplDefinition* instantiate_type(
        GenericInstantiatorAPI& instantiator,
        std::vector<TypeLoc>& types,
        SourceLocation location,
        InstantiationRequirement requirement
    );


};

/**
 * Validate a top-level generic impl and register it as dormant under its generic
 * target declaration. No instantiation happens here. Returns false (with a
 * diagnostic) when the declaration cannot be implemented generically.
 */
bool register_generic_impl(
    ImplementationsIndex& index,
    GenericImplDecl* impl,
    ASTDiagnoser& diagnoser
);

/**
 * Demand-driven instantiation: instantiate every registered generic impl whose
 * target pattern matches the concrete target `for_` and whose interface/member
 * matches the optional filters. `for_` is the concrete target's MembersContainer.
 *
 * Returns true if at least one candidate matched (whether it was already
 * instantiated or created now). The caller retries the failed lookup afterwards.
 */
bool instantiate_generic_impls_for(
    GenericInstantiatorAPI& instantiator,
    ASTAny* for_,
    ASTNode* interface_filter,
    chem::string_view member_filter,
    SourceLocation demand_loc
);

