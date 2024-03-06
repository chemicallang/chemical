// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#pragma once

#include "Scope.h"

class LoopScope : public Scope {
public:
    bool stoppedInterpretOnce = false;

    LoopScope(std::vector<std::unique_ptr<ASTNode>> nodes) : Scope(std::move(nodes)) {

    }

    void interpret(InterpretScope& scope) override {
        for(const auto& node : nodes) {
            node->interpret(scope);
            if(stoppedInterpretOnce) {
                stoppedInterpretOnce = false;
                return;
            }
        }
    }

    void stopInterpretOnce() override {
        stoppedInterpretOnce = true;
    }

};