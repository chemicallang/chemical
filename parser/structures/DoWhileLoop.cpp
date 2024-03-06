// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#include "parser/Parser.h"

/**
 * Parse a single do while loop
 * @return
 */
lex_ptr<DoWhileLoop> Parser::parseDoWhileLoop() {
    if (!consume("do")) {
        return std::nullopt;
    }
    if (consume_op('{')) {
        auto prevParseBreak = isParseBreakStatement;
        auto prevParseContinue = isParseContinueStatement;
        isParseBreakStatement = true;
        isParseContinueStatement = true;
        auto scope = parseLoopScope();
        isParseBreakStatement = prevParseBreak;
        isParseContinueStatement = prevParseContinue;
        if (!consume_op('}')) {
            error("expected a ending brace '}' for 'while' loop");
            return std::nullopt;
        }
        if(!consume("while")) {
            error("expected a 'while' in 'do while' loop for the condition");
            return std::nullopt;
        }
        if(!consume_op('(')) {
            error("expected a starting brace '(' for the 'do while' loop");
            return std::nullopt;
        }
        auto condition = parseExpression();
        if (!condition.has_value()) {
            error("expected a conditional expression for 'while' loop condition");
            return std::nullopt;
        }
        if(!consume_op(')')) {
            error("expected a ending brace ')' for the 'while' loop");
            return std::nullopt;
        }
        return std::make_unique<DoWhileLoop>(std::move(condition.value()),std::move(scope));
    } else {
        error("expected a starting brace '{' for the 'do while' loop");
        return std::nullopt;
    }
}

/**
 * parses a single do while loop
 * @return true if parsed
 */
bool Parser::parseDoWhileLoopBool() {
    auto loop = parseDoWhileLoop();
    if (loop.has_value()) {
        nodes.emplace_back(std::move(loop.value()));
        return true;
    } else {
        return false;
    }
}