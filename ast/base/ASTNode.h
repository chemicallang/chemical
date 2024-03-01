// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <vector>
#include <memory>
#include <string>

/**
 * @brief Base class for all AST nodes.
 */
class ASTNode {
public:

    /**
     * This would return the representation of the node
     * @return
     */
    virtual std::string representation() const = 0;

    virtual ~ASTNode() = default;

};