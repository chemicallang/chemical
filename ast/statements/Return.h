// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"

class ReturnStatement : public ASTNode {
public:

    /**
     * @brief Construct a new ReturnStatement object.
     */
    ReturnStatement(std::optional<std::unique_ptr<Value>> value) : value(std::move(value)) {}

    std::string representation() const override {
        std::string ret;
        ret.append("return");
        if(value.has_value()) {
            ret.append(1, ' ');
            ret.append(value.value()->representation());
            ret.append(1, ';');
        } else {
            ret.append(1, ';');
        }
        return ret;
    }

private:
    std::optional<std::unique_ptr<Value>> value;

};