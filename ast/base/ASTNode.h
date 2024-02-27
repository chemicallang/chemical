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

    std::vector<std::unique_ptr<ASTNode>> children;

    /**
     * @brief Construct a new ASTNode object.
     */
    ASTNode() = default;

    /**
     * @brief Destroy the ASTNode object.
     */
    virtual ~ASTNode() = default;

};