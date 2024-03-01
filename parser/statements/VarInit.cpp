// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "parser/Parser.h"
#include "lexer/model/tokens/KeywordToken.h"
#include "lexer/model/tokens/VariableToken.h"

bool Parser::parseVariableInitStatement() {
    if(!consume("var")) {
        return false;
    }
    auto var = consumeOfType<VariableToken>(LexTokenType::Variable);
    if (!var.has_value()) {
        return false;
    }
    if (!consume_op('=')) {
        return false;
    }
    auto number = parseExpression();
    if (number.has_value()) {
        nodes.emplace_back(std::make_unique<VarInitStatement>(var.value()->value, std::move(number.value())));
        return true;
    } else {
        error("expected an integer token");
        return false;
    }
}