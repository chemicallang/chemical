// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class FunctionDeclaration : public ASTNode {
public:
    /**
     * @brief Construct a new FunctionDeclaration object.
     *
     * @param name The name of the function.
     * @param returnType The return type of the function.
     * @param parameters The parameters of the function.
     * @param body The body of the function.
     */
    FunctionDeclaration(const std::string& name, const std::string& returnType,
                        std::vector<std::pair<std::string, std::string>> parameters,
                        std::shared_ptr<ASTNode> body)
            : name(name), returnType(returnType), parameters(parameters), body(body) {}

private:
    std::string name; ///< The name of the function.
    std::string returnType; ///< The return type of the function.
    std::vector<std::pair<std::string, std::string>> parameters; ///< The parameters of the function.
    std::shared_ptr<ASTNode> body; ///< The body of the function.
};