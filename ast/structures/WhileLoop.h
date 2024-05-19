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

    /**
     * initializes the loop with only a condition and empty body
     * @param condition
     */
    WhileLoop(std::unique_ptr<Value> condition);

    /**
     * @brief Construct a new WhileLoop object.
     *
     * @param condition The loop condition.
     * @param body The body of the while loop.
     */
    WhileLoop(std::unique_ptr<Value> condition, LoopScope body);

    void accept(Visitor *visitor) override;

    void declare_and_link(SymbolResolver &linker) override;

    void interpret(InterpretScope &scope) override;

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override;
#endif

    void stopInterpretation() override;

    std::string representation() const override;

    std::unique_ptr<Value> condition;
    bool stoppedInterpretation = false;
};