// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "parser/Parser.h"
#include "lexer/model/tokens/KeywordToken.h"
#include "lexer/model/tokens/VariableToken.h"
#include "lexer/model/tokens/TypeToken.h"

lex_ptr<VarInitStatement> Parser::parseVariableInitStatement() {
    auto consumed_const = consume("const");
    if (!consumed_const && !consume("var")) {
        return std::nullopt;
    }
    auto var = consumeOfType<VariableToken>(LexTokenType::Variable);
    if (!var.has_value()) {
        return std::nullopt;
    }
    lex_ptr<BaseType> base_type = std::nullopt;
    if (consume_op(':')) {
        base_type = parseType();
    }
    lex_ptr<Value> value = std::nullopt;
    if (consume_op('=')) {
        value = parseExpression();
    }
    return std::make_unique<VarInitStatement>(consumed_const, var.value()->value, std::move(base_type), std::move(value));
}

bool Parser::parseVariableInitStatementBool() {
    auto statement = parseVariableInitStatement();
    if (statement.has_value()) {
        nodes.emplace_back(std::move(statement.value()));
        return true;
    } else {
        return false;
    }
}