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
        auto incrementer = parseVarAssignStatement();
        if (!incrementer.has_value()) {
            error("expected an incrementer expression for 'for' loop");
            return std::nullopt;
        }
        if(!consume_op(')')) {
            error("expected a ending brace ')' for the 'for' loop");
            return std::nullopt;
        }
        if (consume_op('{')) {

            auto loop = std::make_unique<ForLoop>(std::move(statement.value()),std::move(condition.value()), std::move(incrementer.value()));

            auto prevLoopNode = current_loop_node;
            // the warning : the address of the local variable may escape the function is invalid because loop is moved at the end of the scope
            current_loop_node = loop.get();
            auto prevParseBreak = isParseBreakStatement;
            auto prevParseContinue = isParseContinueStatement;
            isParseBreakStatement = true;
            isParseContinueStatement = true;
            auto scope_nodes = parseScopeNodes();
            isParseBreakStatement = prevParseBreak;
            isParseContinueStatement = prevParseContinue;
            current_loop_node = prevLoopNode;

            loop->body.nodes = std::move(scope_nodes);

            if (!consume_op('}')) {
                error("expected a ending brace '}' for 'for' loop");
                return std::nullopt;
            }
            return loop;
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