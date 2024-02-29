// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/ASTNode.h"

class Scope : public ASTNode {
public:

    /**
     * @brief Construct a new Scope object.
     * @param nodes All the ASTNode(s) present in the scope
     */
    Scope(std::vector<std::unique_ptr<ASTNode>> nodes) : nodes(std::move(nodes)) {}

    /**
     * move constructor
     * @param other
     */
    Scope(Scope &&other) : nodes(std::move(other.nodes)) {}

private:
    std::vector<std::unique_ptr<ASTNode>> nodes;

};