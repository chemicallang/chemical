// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#include "parser/Parser.h"
#include "lexer/model/tokens/FunctionToken.h"
#include "lexer/model/tokens/ParameterToken.h"
#include "lexer/model/tokens/TypeToken.h"

lex_ptr<ReturnStatement> Parser::parseReturnStatement() {
    if(consume("return")) {
        auto value = parseExpression();
        if(value.has_value()) {
            return std::make_unique<ReturnStatement>(std::move(value.value()));
        } else {
            return std::make_unique<ReturnStatement>(std::nullopt);
        }
    }
    return std::nullopt;
}

bool Parser::parseReturnStatementBool() {
    auto value = parseReturnStatement();
    if(value.has_value()) {
        nodes.emplace_back(std::move(value.value()));
        return true;
    } else {
        return false;
    }
}

/**
 * parses a single for loop
 * @return true if parsed
 */
lex_ptr<FunctionDeclaration> Parser::parseFunctionDefinition() {
    if (consume("func")) {
        if (token_type() == LexTokenType::Function) {
            auto token = consume<FunctionToken>();
            if (consume_op('(')) {
                func_params params;
                do {
                    if(token_type() == LexTokenType::Parameter) {
                        auto paramToken = consume<ParameterToken>();
                        if(!consume_op(':')) {
                            error("expected a ':' after the parameter for its type");
                            break;
                        }
                        if(token_type() == LexTokenType::Type) {
                            auto typeToken  = consume<TypeToken>();
                            params.emplace_back(std::move(paramToken->value), std::move(typeToken->value));
                        } else {
                            error("expected a type after the colon ':' for the function signature");
                            break;
                        }
                    }
                } while (consume_op(','));
                if (!consume_op(')')) {
                    error("expected a ')' after the function signature");
                }
                std::optional<std::string> returnType;
                if(consume_op(':')) {
                    if (token_type() == LexTokenType::Type) {
                        returnType = std::move(consume<TypeToken>()->value);
                    } else {
                        error("expected a type after the colon ':' for the function signature");
                        return std::nullopt;
                    }
                } else {
                    returnType = std::nullopt;
                }
                if (!consume_op('{')) {
                    error("expected a '}' for the function body");
                }
                auto prevReturn = isParseReturnStatement;
                isParseReturnStatement = true;
                auto scope = parseScope();
                isParseReturnStatement = prevReturn;
                if (!consume_op('}')) {
                    error("expected a '}' for ending the function body");
                } else {
                    return std::make_unique<FunctionDeclaration>(std::move(token->value), std::move(params), std::move(returnType), std::move(scope));
                }
            } else {
                error("expected a '(' after the function name");
            }
        } else {
            error("expected a function name after the 'func' keyword");
        }
    }
    return std::nullopt;
}

/**
 * parses a single function
 * @return true if parsed
 */
bool Parser::parseFunctionDefinitionBool() {
    auto loop = parseFunctionDefinition();
    if (loop.has_value()) {
        nodes.emplace_back(std::move(loop.value()));
        return true;
    } else {
        return false;
    }
}