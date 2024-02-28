// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "parser/Parser.h"
#include "lexer/model/tokens/KeywordToken.h"
#include "lexer/model/tokens/VariableToken.h"

void Parser::parseVariableInitStatement() {
    auto varToken = consume("var");
    if (!varToken.has_value()) {
        return;
    }
    auto var = consumeOfType<VariableToken>(LexTokenType::Variable);
    if (!var.has_value()) {
        return;
    }
    if (!consumeOperator('=')) {
        return;
    }
    auto number = parseIntNode();
    if (number.has_value()) {
        nodes.emplace_back(std::make_unique<VarInitStatement>(var.value()->value, (number.value())));
    } else {
        error("expected an integer token");
    }
}