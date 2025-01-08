// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"

class ValueWrapperNode : public ASTNode {
public:

    Value* value;
    ASTNode* parent_node;

    ValueWrapperNode(Value* value, ASTNode* parent_node) : value(value), parent_node(parent_node) {

    }

    ASTNodeKind kind() override {
        return ASTNodeKind::ValueWrapper;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    SourceLocation encoded_location() override {
        return value->encoded_location();
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