// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class EnumDeclaration : public ASTNode {
public:
    /**
     * @brief Construct a new EnumDeclaration object.
     *
     * @param name The name of the enum.
     * @param values The values of the enum.
     */
    EnumDeclaration(const std::string& name, std::vector<std::string> values)
            : name(name), values(values) {}

private:
    std::string name; ///< The name of the enum.
    std::vector<std::string> values; ///< The values of the enum.
};