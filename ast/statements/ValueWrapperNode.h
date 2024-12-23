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
        value->accept(visitor);
    }

    SourceLocation encoded_location() override {
        return value->encoded_location();
    }

    ASTNode* parent() override {
        return parent_node;
    }

    void declare_and_link(SymbolResolver &linker) override {
        value->link(linker, value);
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override {
        value->llvm_value(gen);
    }

#endif

};