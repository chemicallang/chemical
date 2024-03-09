// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/ASTNode.h"

class StructDefinition : public ASTNode {
public:

    /**
     * @brief Construct a new StructDeclaration object.
     *
     * @param name The name of the struct.
     * @param fields The fields of the struct.
     */
    StructDefinition(std::string name, std::vector<std::unique_ptr<VarInitStatement>> fields)
            : name(std::move(name)), fields(std::move(fields)) {}

    void interpret(InterpretScope &scope) override {

    }

    std::string representation() const override {
        std::string ret("struct " + name + " {\n");
        int i = 0;
        while (i < fields.size()) {
            ret.append(fields[i]->representation());
            if (i < fields.size() - 1) {
                ret.append(1, '\n');
            }
            i++;
        }
        ret.append("\n}");
        return ret;
    }

private:
    std::string name; ///< The name of the struct.
    std::vector<std::unique_ptr<VarInitStatement>> fields; ///< The fields of the struct.
};