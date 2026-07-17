// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include <thread>

enum class InstantiationStatus : int {
    Building,
    Finalized
};

struct InstantiationStatusEntry {
    InstantiationStatus status;
    std::thread::id builder_thread;
};

class BaseGenericDecl : public ASTNode {
public:

    std::vector<GenericTypeParameter*> generic_params;

    /**
     * per-instantiation status vector, parallel to the instantiations vector
     * in each concrete Generic*Decl subclass
     */
    std::vector<InstantiationStatusEntry> instantiation_statuses;

    /**
     * constructor
     */
    inline constexpr BaseGenericDecl(
        ASTNodeKind k,
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(k, parent_node, location) {

    }

};