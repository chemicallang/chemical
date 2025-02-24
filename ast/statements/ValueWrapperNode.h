// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"

class ValueWrapperNode : public ASTNode {
public:

    Value* value;
    ASTNode* parent_node;

    ValueWrapperNode(
        Value* value,
        ASTNode* parent_node
    ) : ASTNode(ASTNodeKind::ValueWrapper, value->encoded_location()), value(value), parent_node(parent_node) {

    }

    ASTNode* parent() override {
        return parent_node;
    }

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) override {
        value->link(linker, value);
    }

    void interpret(InterpretScope &scope) override {
        value->evaluated_value(scope);
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override {
        const auto inst = value->llvm_value(gen);
        value->llvm_destruct(gen, inst);
    }

#endif

};