// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/base/LoopASTNode.h"
#include "Scope.h"

class DoWhileLoop : public LoopASTNode {
public:

    /**
     * Initialize an empty do while loop
     */
    DoWhileLoop() {

    }

    /**
     * @brief Construct a new WhileLoop object.
     *
     * @param condition The loop condition.
     * @param body The body of the while loop.
     */
    DoWhileLoop(std::unique_ptr<Value> condition, LoopScope body);

    void accept(Visitor &visitor) override;

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override;
#endif

    void declare_and_link(SymbolResolver &linker) override;

    void interpret(InterpretScope &scope) override;

    void stopInterpretation() override;

    std::string representation() const override;

    std::unique_ptr<Value> condition;

private:
    bool stoppedInterpretation = false;
};