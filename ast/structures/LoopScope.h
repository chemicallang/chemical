// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#pragma once

#include "Scope.h"

class LoopScope : public Scope {
public:

    bool stoppedInterpretOnce = false;

    /**
     * empty constructor
     */
    LoopScope() : Scope() {

    }

    /**
     * construct with given nodes
     * @param nodes
     */
    LoopScope(std::vector<std::unique_ptr<ASTNode>> nodes);

    void interpret(InterpretScope& scope) override;

    /**
     * this will stop the interpretation at the current statement
     * meaning all the nodes in this scope will be skipped
     */
    void stopInterpretOnce() override;

};