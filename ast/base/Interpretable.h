// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include "InterpretScope.h"
#include <iostream>

/**
 * @brief Base class for all things that can be interpreted
 */
class Interpretable {
public:

    /**
     * Interpret the current node in the given interpret scope
     * @param scope
     */
    virtual void interpret(InterpretScope &scope) {
        std::cerr << "[Interpretable:Base]";
    }

};