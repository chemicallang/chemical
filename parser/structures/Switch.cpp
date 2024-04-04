// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"
#include "ast/statements/SwitchStatement.h"

lex_ptr<SwitchStatement> Parser::parseSwitchStatement() {
    if(consume("switch")) {
        if(consume_op('(')) {
            auto expr = parseExpression();
            if(!expr.has_value()) {
                error("expected expression in switch");
            }
            if(!consume_op(')')) {
                error("expected ')' in switch");
            }
            if(consume_op('{')) {
                std::vector<std::pair<std::unique_ptr<Value>, Scope>> scopes;
                std::optional<Scope> defScope = std::nullopt;
                while(true) {
                    if(consume("case")) {
                        auto value = parseValue();
                        if(!value.has_value()){
                            error("expected case to have a value");
                            break;
                        }
                        if(consume_op(':')) {
                            auto scope = parseScope();
                            scopes.emplace_back(std::move(value.value()), std::move(scope));
                        } else if(consume_op("->")) {
                            consume_op('{');
                            auto scope = parseScope();
                            scopes.emplace_back(std::move(value.value()), std::move(scope));
                            consume_op('}');
                        }
                    } else if(consume("default")) {
                        if(consume_op(':')) {
                            auto scope = parseScope();
                        } else if(consume("->")) {
                            consume_op('{');
                            auto scope = parseScope();
                            defScope.emplace(std::move(scope));
                            consume_op('}');
                        }
                    } else {
                        break;
                    }
                }
                if(!consume_op('}')) {
                    error("expected '}' for ending the switch statement");
                }
                return std::make_unique<SwitchStatement>(std::move(expr.value()), std::move(scopes), std::move(defScope));
            }
        } else {
            error("expected '(' in switch");
        }
    }
    return std::nullopt;
}

bool Parser::parseSwitchStatementBool() {
    return parse_return_bool([&]() -> lex_ptr<SwitchStatement> {
        return parseSwitchStatement();
    });
}