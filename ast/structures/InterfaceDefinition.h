// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class InterfaceDeclaration : public ASTNode {
public:
    /**
     * @brief Construct a new InterfaceDeclaration object.
     *
     * @param name The name of the interface.
     * @param methods The methods declared in the interface.
     */
    InterfaceDeclaration(const std::string& name, std::vector<std::shared_ptr<ASTNode>> methods)
            : name(name), methods(methods) {}

private:
    std::string name; ///< The name of the interface.
    std::vector<std::shared_ptr<ASTNode>> methods; ///< The methods declared in the interface.
};