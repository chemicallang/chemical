// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/utils/Operation.h"
#include <utility>

class AssignStatement : public ASTNode {
public:

    Value* lhs;
    Value* value;
    // TODO there's no usage of this, remove this
    InterfaceDefinition* definition;
    Operation assOp;

    /**
     * @brief Construct a new AssignStatement object.
     *
     * @param identifier The identifier being assigned.
     * @param value The value being assigned to the identifier.
     */
    constexpr AssignStatement(
            Value* lhs,
            Value* value,
            Operation assOp,
            ASTNode* parent_node,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::AssignmentStmt, parent_node, location), lhs(lhs), value(value), assOp(assOp) {

    }

    AssignStatement* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<AssignStatement>()) AssignStatement(
            lhs->copy(allocator),
            value->copy(allocator),
            assOp,
            parent(),
            encoded_location()
        );
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) final;
#endif

};