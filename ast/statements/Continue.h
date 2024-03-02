// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class ContinueStatement : public ASTNode {
public:

    /**
     * @brief Construct a new ContinueStatement object.
     */
    ContinueStatement() {}

    std::string representation() const override {
        std::string ret;
        ret.append("continue;");
        return ret;
    }

};