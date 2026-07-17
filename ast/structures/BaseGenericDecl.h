// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include <thread>

/**
 * determines what register_generic_args does when called:
 * - Registration: only register (shallow copy + push), no signature/body finalization
 * - SignatureFinalization: register + finalize signature (no body)
 * - BodyFinalization: register + finalize signature + body, wait for deps' signatures
 */
enum class InstantiationRequirement : uint8_t {
    Registration,
    SignatureFinalization
};

enum class InstantiationStatus : uint8_t {
    Registered,
    SignatureFinalized
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
     * in each concrete Generic*Decl subclass. Used for phase-aware synchronization:
     * - During signature finalization: only registration is needed (no waiting)
     * - During body finalization: signature finalization is needed (wait on condvar)
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
