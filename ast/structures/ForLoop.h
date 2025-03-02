// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/LoopASTNode.h"
#include "ast/base/Value.h"
#include "Scope.h"
#include "ast/statements/VarInit.h"

class ForLoop : public LoopASTNode {
public:

    ASTNode* initializer;
    Value* conditionExpr;
    ASTNode* incrementerExpr;
    bool stoppedInterpretation = false;

    /**
     * @brief Construct a new ForLoop object.
     */
    constexpr ForLoop(
            VarInitStatement* initializer,
            Value* conditionExpr,
            ASTNode* incrementerExpr,
            ASTNode* parent_node,
            SourceLocation location
    ) : LoopASTNode(ASTNodeKind::ForLoopStmt, parent_node, location), initializer(initializer), conditionExpr(conditionExpr),
        incrementerExpr(incrementerExpr) {

    }

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) final;
#endif

    void interpret(InterpretScope &scope) final;

    void stopInterpretation() final;

};