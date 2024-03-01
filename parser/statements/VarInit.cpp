// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "parser/Parser.h"
#include "lexer/model/tokens/KeywordToken.h"
#include "lexer/model/tokens/VariableToken.h"
#include "lexer/model/tokens/TypeToken.h"

lex_ptr<VarInitStatement> Parser::parseVariableInitStatement() {
    if(!consume("var")) {
        return std::nullopt;
    }
    auto var = consumeOfType<VariableToken>(LexTokenType::Variable);
    if (!var.has_value()) {
        return std::nullopt;
    }
    std::optional<std::string> type_string;
    if(consume_op(':')) {
        if(token_type() == LexTokenType::Type) {
            type_string = std::move(consume<TypeToken>()->value);
        } else {
            type_string = std::nullopt;
        }
    }
    if (!consume_op('=')) {
        return std::nullopt;
    }
    auto number = parseExpression();
    if (number.has_value()) {
        return std::make_unique<VarInitStatement>(var.value()->value, std::move(type_string), std::move(number.value()));
    } else {
        error("expected an integer token");
        return std::nullopt;
    }
}

bool Parser::parseVariableInitStatementBool() {
    auto statement = parseVariableInitStatement();
    if(statement.has_value()) {
        nodes.emplace_back(std::move(statement.value()));
        return true;
    } else {
        return false;
    }
}