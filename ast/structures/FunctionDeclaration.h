// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

using func_params = std::vector<std::pair<std::string, std::string>>;

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
    FunctionDeclaration(
            std::string name,
            func_params params,
            std::string returnType,
            Scope body
    ) : name(std::move(name)), params(std::move(params)), returnType(std::move(returnType)), body(std::move(body)) {}

    std::string representation() const override {
        std::string ret;
        ret.append("func ");
        ret.append(name);
        ret.append(1, '(');
        int i = 0;
        while (i < params.size()) {
            const auto &param = params[i];
            ret.append(param.first);
            ret.append(" : ");
            ret.append(param.second);
            if (i < params.size() - 1) {
                ret.append(", ");
            }
            i++;
        }
        ret.append(1, ')');
        if (returnType.has_value()) {
            ret.append(" : ");
            ret.append(returnType.value());
            ret.append(1, ' ');
        }
        ret.append("{\n");
        ret.append(body.representation());
        ret.append("\n}");
        return ret;
    }

private:
    std::string name; ///< The name of the function.
    func_params params;
    std::optional<std::string> returnType;
    Scope body; ///< The body of the function.
};