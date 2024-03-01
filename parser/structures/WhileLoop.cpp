// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//
#include "parser/Parser.h"


/**
 * Parse a single for loop
 * @return
 */
lex_ptr<WhileLoop> Parser::parseWhileLoop() {
    if (!consume("while")) {
        return std::nullopt;
    }
    if (consume_op('(')) {
        auto condition = parseExpression();
        if (!condition.has_value()) {
            error("expected a conditional expression for 'while' loop condition");
            return std::nullopt;
        }
        if(!consume_op(')')) {
            error("expected a ending brace ')' for the 'while' loop");
            return std::nullopt;
        }
        if (consume_op('{')) {
            auto scope = parseScope();
            if (!consume_op('}')) {
                error("expected a ending brace '}' for 'while' loop");
                return std::nullopt;
            }
            return std::make_unique<WhileLoop>(std::move(condition.value()),std::move(scope));
        } else {
            error("expected a starting brace '{' after the ')' for the 'while' loop");
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
bool Parser::parseWhileLoopBool() {
    auto loop = parseWhileLoop();
    if (loop.has_value()) {
        nodes.emplace_back(std::move(loop.value()));
        return true;
    } else {
        return false;
    }
}