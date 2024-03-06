// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <vector>
#include <memory>
#include <string>
#include "InterpretScope.h"
#include <iostream>

/**
 * @brief Base class for all AST nodes.
 */
class ASTNode {
public:

    /**
     * Interpret the current node in the given interpret scope
     * @param scope
     */
    virtual void interpret(InterpretScope& scope) {
        std::cerr << "ASTNode Bare interpret Called";
        // TODO make = 0
    }

    /**
     * return true if this node supports continue statement
     * @return
     */
    virtual bool supportsContinue() {
        return false;
    }

    /**
     * return true if this node supports break statement
     * @return
     */
    virtual bool supportsBreak() {
        return false;
    }

    /**
     * This is called by statements like break
     * to break the current interpretation, that is run by ASTNode's like loops (for, while)
     */
    virtual void stopInterpretation() {

    }

    /**
     * This would return the representation of the node
     * @return
     */
    virtual std::string representation() const = 0;

    /**
     * virtual destructor for the ASTNode
     */
    virtual ~ASTNode() = default;

};