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
    std::unique_ptr<Value> value;
    ASTNode* parent_node;

    /**
     * constructor
     */
    ValueNode(std::unique_ptr<Value> value, ASTNode* parent_node) : value(std::move(value)), parent_node(parent_node) {

    }

    Value *holding_value() override {
        return value.get();
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::ValueNode;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode> &node_ptr) override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

#endif
};