// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "Scope.h"
#include "LoopScope.h"
#include "ast/base/LoopASTNode.h"

class WhileLoop : public LoopASTNode {
public:

    Value* condition;
    bool stoppedInterpretation = false;
    ASTNode* parent_node;
    SourceLocation location;

    /**
     * initializes the loop with only a condition and empty body
     * @param condition
     */
//    WhileLoop(std::unique_ptr<Value> condition, ASTNode* parent_node, CSTToken* token);

    /**
     * @brief Construct a new WhileLoop object.
     *
     * @param condition The loop condition.
     * @param body The body of the while loop.
     */
    WhileLoop(Value* condition, LoopScope body, ASTNode* parent_node, SourceLocation location);

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::WhileLoopStmt;
    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    void accept(Visitor *visitor) final;

    void declare_and_link(SymbolResolver &linker) final;

    void interpret(InterpretScope &scope) final;

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) final;
#endif

    void stopInterpretation() final;

};