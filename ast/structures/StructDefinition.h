// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class StructDefinition : public ASTNode {
public:
    /**
     * @brief Construct a new StructDeclaration object.
     *
     * @param name The name of the struct.
     * @param fields The fields of the struct.
     */
    StructDefinition(const std::string& name, std::vector<std::pair<std::string, std::shared_ptr<ASTNode>>> fields)
    : name(name), fields(fields) {}

private:
    std::string name; ///< The name of the struct.
    std::vector<std::pair<std::string, std::shared_ptr<ASTNode>>> fields; ///< The fields of the struct.
};