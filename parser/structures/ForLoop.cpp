// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#include "parser/Parser.h"
#include "lexer/model/tokens/VariableToken.h"

/**
 * Parse a single for loop
 * @return
 */
lex_ptr<ForLoop> Parser::parseForLoop() {
    if (!consume("for")) {
        return std::nullopt;
    }
    if (consume_op('(')) {
        auto statement = parseVariableInitStatement();
        if(!statement.has_value()) {
            error("expected a variable initialization statement for 'for' loop");
            return std::nullopt;
        }
        if (!consume_op(';')) {
            error("expected a ';' for ending variable initialization in for loop");
            return std::nullopt;
        }
        auto condition = parseExpression();
        if (!condition.has_value()) {
            error("expected a conditional expression for 'for' loop condition");
            return std::nullopt;
        }
        if (!consume_op(';')) {
            error("expected a ';' for ending 'for' loop condition");
            return std::nullopt;
        }
        auto incrementer = parseExpression();
        if (!incrementer.has_value()) {
            error("expected an incrementer expression for 'for' loop");
            return std::nullopt;
        }
        if(!consume_op(')')) {
            error("expected a ending brace ')' for the 'for' loop");
            return std::nullopt;
        }
        if (consume_op('{')) {
            auto prevParseBreak = isParseBreakStatement;
            auto prevParseContinue = isParseContinueStatement;
            isParseBreakStatement = true;
            isParseContinueStatement = true;
            auto scope = parseScope();
            isParseBreakStatement = prevParseBreak;
            isParseContinueStatement = prevParseContinue;
            if (!consume_op('}')) {
                error("expected a ending brace '}' for 'for' loop");
                return std::nullopt;
            }
            return std::make_unique<ForLoop>(std::move(statement.value()),
                                             std::move(condition.value()), std::move(incrementer.value()),
                                             std::move(scope));
        } else {
            error("expected a starting brace '{' after the ')' for the 'for' loop");
            return std::nullopt;
        }
    } else {
        error("expected a starting parenthesis '('");
        return std::nullopt;
    }
}

/**
 * parses a single for loop
 * @return true if parsed
 */
bool Parser::parseForLoopBool() {
    auto loop = parseForLoop();
    if (loop.has_value()) {
        nodes.emplace_back(std::move(loop.value()));
        return true;
    } else {
        return false;
    }
}