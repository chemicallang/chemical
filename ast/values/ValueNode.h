// Copyright (c) Qinetik 2024.

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
    ASTNode* parent_node;
    CSTToken* token;

    /**
     * constructor
     */
    ValueNode(Value* value, ASTNode* parent_node, CSTToken* token) : value(value), parent_node(parent_node), token(token) {

    }

    CSTToken *cst_token() final {
        return token;
    }

    Value *holding_value() final {
        return value;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::ValueNode;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    void declare_and_link(SymbolResolver &linker) final;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif
};