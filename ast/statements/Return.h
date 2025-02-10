// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"

class FunctionType;

class ReturnStatement : public ASTNode {
public:

    ASTNode* parent_node;
    FunctionTypeBody* func_type = nullptr;
    Value* value;
    SourceLocation location;

    /**
     * @brief Construct a new ReturnStatement object.
     */
    ReturnStatement(
            Value* value,
            FunctionTypeBody* declaration,
            ASTNode* parent_node,
            SourceLocation location
    ) : value(value), func_type(declaration), parent_node(parent_node), location(location) {

    }

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::ReturnStmt;
    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    BaseType* known_type() final;

    void interpret(InterpretScope &scope) final;

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void accept(Visitor *visitor) final;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen, Scope *scope, unsigned int index) final;

#endif

};