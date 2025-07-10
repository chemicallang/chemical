// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"

/**
 * a specific node that is used to wrap a value, so it can be extracted
 * this is useful in if statement where if block contains a value, for example
 *
 * if(true) 3 else 4
 *
 * here 3 and 4 are like statements, so we wrap them in a value node
 *
 * ValueNode doesn't provide a single feature above, except for maybe linking
 * it crashes if code_gen is required or llvm_value is required, the container must
 * extract the value to use it
 *
 */
class ValueNode : public ASTNode {
public:

    /**
     * actual value
     */
    Value* value;

    /**
     * constructor
     */
    constexpr ValueNode(
        Value* value,
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::ValueNode, parent_node, location), value(value) {

    }


    Value *holding_value() final {
        return value;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif
};