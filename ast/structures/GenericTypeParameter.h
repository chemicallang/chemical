// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/TypeLoc.h"
#include "ast/base/InterfaceBits.h"

class GenericTypeParameter : public ASTNode {
private:

    /**
     * we set this to an active type, before instantiating the generic decl
     */
    BaseType* active_type = nullptr;

public:

    chem::string_view identifier;
    std::vector<TypeLoc> traits;
    TypeLoc def_type;
    // TODO use int32 here
     // <-- index of active type in usage vector
    unsigned param_index = 0; // <-- index in the generic type parameters

    /**
     * current trait bits for this generic parameter.
     * Updated temporarily by `where` clauses during type checking.
     */
    InterfaceBits current_bits;

    /**
     * constructor
     */
    constexpr GenericTypeParameter(
        chem::string_view identifier,
        TypeLoc def_type,
        ASTNode* parent_node,
        unsigned param_index,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::GenericTypeParam, parent_node, location), identifier(identifier),
        def_type(def_type), param_index(param_index)
    {

    }

    void set_active_type(BaseType* type) {
        active_type = type;
    }

    void deactivate_iteration() {
        active_type = nullptr;
    }

    inline BaseType *known_type() {
        return active_type;
    }

    /**
     * this method is called by the generic instantiator
     */
    inline BaseType* concrete_type() {
        return active_type;
    }

    inline ASTNode* active_linked() {
        return active_type ? active_type->linked_node() : nullptr;
    }



};