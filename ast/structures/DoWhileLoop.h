// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/base/LoopASTNode.h"
#include "Scope.h"

class DoWhileLoop : public LoopASTNode {
private:
    bool stoppedInterpretation = false;
public:
    Value* condition;
    ASTNode* parent_node;
    SourceLocation location;

    /**
     * Initialize an empty do while loop
     */
//    explicit DoWhileLoop(ASTNode* parent, CSTToken* token) : parent_node(parent), token(token), LoopASTNode(nullptr) {
//
//    }

    SourceLocation encoded_location() override {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::DoWhileLoopStmt;
    }

    ASTNode * parent() final {
        return parent_node;
    }

    /**
     * @brief Construct a new WhileLoop object.
     *
     * @param condition The loop condition.
     * @param body The body of the while loop.
     */
    DoWhileLoop(
            Value* condition,
            LoopScope body,
            ASTNode* parent_node,
            SourceLocation location
    );

    void accept(Visitor *visitor) final;

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) final;
#endif

    void declare_and_link(SymbolResolver &linker) final;

    void interpret(InterpretScope &scope) final;

    void stopInterpretation() final;

};