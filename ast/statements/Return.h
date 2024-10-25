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
    FunctionType* func_type = nullptr;
    Value* value;
    CSTToken* token;

    /**
     * @brief Construct a new ReturnStatement object.
     */
    ReturnStatement(
            Value* value,
            FunctionType *declaration,
            ASTNode* parent_node,
            CSTToken* token
    );

    CSTToken *cst_token() final {
        return token;
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

    void declare_and_link(SymbolResolver &linker) final;

    void accept(Visitor *visitor) final;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen, Scope *scope, unsigned int index) final;

#endif

};