// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class BreakStatement : public ASTNode {
public:

    /**
     * @brief Construct a new Break statement object.
     */
    BreakStatement() {}

    std::string representation() const override {
        std::string ret;
        ret.append("break;");
        return ret;
    }

};