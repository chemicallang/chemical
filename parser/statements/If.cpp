// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#include "parser/Parser.h"

bool Parser::parseIfStatementBool() {
    auto statement = parseIfStatement();
    if(statement.has_value()) {
        nodes.emplace_back(std::move(statement.value()));
        return true;
    } else {
        return false;
    }
}

lex_ptr<IfStatement> Parser::parseIfStatement() {
    if(!consume("if")) {
        return std::nullopt;
    }
    if(consume_op('(')) {
        auto condition = parseExpression();
        if(!condition.has_value()) {
            error("expected a conditional expression for 'if' condition");
            return std::nullopt;
        }
        if(!consume_op(')')) {
            error("expected a ending parenthesis ')' in 'if' condition");
            return std::nullopt;
        }
        if(consume_op('{')) {
            auto scope = parseScope();
            if(!consume_op('}')) {
                error("expected a ending brace '}' for 'if' block");
            }
            auto elseIfs = std::vector<std::unique_ptr<IfStatement>>();
            while(consume("else")) {
                if(consume_op('{')) {
                    auto elseScope = parseScope();
                    auto ifst = std::make_unique<IfStatement>(std::move(condition.value()), std::move(scope), std::move(elseIfs), std::move(elseScope));
                    if(!consume_op('}')) {
                        error("expected a ending brace '}' for 'else' block");
                    }
                    return ifst;
                } else {
                    auto elseIf = parseIfStatement();
                    if (elseIf.has_value()) {
                        elseIfs.emplace_back(std::move(elseIf.value()));
                    } else {
                        error("expected a else if / else block after 'if' statement");
                        break;
                    }
                }
            }
            return std::make_unique<IfStatement>(std::move(condition.value()), std::move(scope), std::move(elseIfs), std::nullopt);
        } else {
            error("expected a starting brace after the ')' for the 'if' block");
        }
    } else {
        error("expected a starting parenthesis '('");
    }
    return std::nullopt;
}