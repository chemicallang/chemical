// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#include "parser/Parser.h"
#include "lexer/model/tokens/VariableToken.h"
#include "ast/statements/Assignment.h"

void Parser::parseVarAssignStatement() {
    auto var = consumeOfType<VariableToken>(LexTokenType::Variable, false);
    if (!var.has_value()) {
        return;
    }
    if(consumeOperator('=')) {
        auto value = parseValueNode();
        if(value.has_value()) {
            nodes.emplace_back(std::make_unique<AssignStatement>(var.value()->value, std::move(value.value())));
        } else {
            error("expected a value after the equal in var assignment");
        }
    }
};