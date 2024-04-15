// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#include "parser/Parser.h"
#include "lexer/model/tokens/TypeToken.h"
#include "ast/types/VoidType.h"
#include "ast/statements/Return.h"
#include "ast/structures/FunctionDeclaration.h"

lex_ptr<ReturnStatement> Parser::parseReturnStatement() {
    if (consume("return")) {
        auto value = parseExpression();
        if (value.has_value()) {
            return std::make_unique<ReturnStatement>(std::move(value.value()), current_func_decl);
        } else {
            return std::make_unique<ReturnStatement>(std::nullopt, current_func_decl);
        }
    }
    return std::nullopt;
}

bool Parser::parseReturnStatementBool() {
    return parse_return_bool([&]() -> lex_ptr<ReturnStatement> {
        return parseReturnStatement();
    });
}

std::pair<std::vector<std::unique_ptr<FunctionParam>>, bool> Parser::parseFunctionParams(bool optionalTypes) {
    func_params params;
    unsigned paramsCount = 0;
    bool isVariadic = false;
    do {
        if (token_type() == LexTokenType::Variable) {
            auto paramToken = consume<VariableToken>();
            if(consume_op(':')) {
                auto type = parseType();
                if (type.has_value()) {
                    if (token_type() == LexTokenType::StringOperator &&
                        as<AbstractStringToken>()->value == "...") {
                        increment();
                        isVariadic = true;
                    }
                    lex_ptr<Value> defValue = std::nullopt;
                    if(!isVariadic && consume_op('=')) {
                        defValue = parseValue();
                        if(!defValue.has_value()) {
                            error("expected a default value after '=' for the parameter with name " + paramToken->value);
                        }
                    }
                    params.emplace_back(std::make_unique<FunctionParam>(paramToken->value, std::move(type.value()), paramsCount, isVariadic, std::move(defValue)));
                    paramsCount++;
                    if(isVariadic) {
                        break;
                    }
                } else {
                    error("expected a type after the colon ':' for the function parameter");
                    break;
                }
            } else if(optionalTypes) {
                params.emplace_back(std::make_unique<FunctionParam>(paramToken->value, std::move(std::make_unique<VoidType>()), paramsCount, isVariadic, std::nullopt));
                paramsCount++;
            } else {
                error("expected a ':' after the parameter for its type");
                break;
            }
        }
    } while (consume_op(','));
    return std::pair(std::move(params), isVariadic);
}

/**
 * parses a single for loop
 * @return true if parsed
 */
lex_ptr<FunctionDeclaration> Parser::parseFunctionDefinition(bool declarations) {
    if (consume("func")) {
        if (token_type() == LexTokenType::Variable) {
            auto token = consume<VariableToken>();
            if (consume_op('(')) {
                auto params = parseFunctionParams();
                if (!consume_op(')')) {
                    error("expected a ')' after the function signature");
                }
                lex_ptr<BaseType> returnType;
                if (consume_op(':')) {
                    returnType = parseType();
                    if (!returnType.has_value()) {
                        error("expected a type after the colon ':' for the function return");
                        return std::nullopt;
                    }
                } else {
                    returnType = std::make_unique<VoidType>();
                }

                // create declaration before, so that return statement can take a pointer to it
                auto declaration = std::make_unique<FunctionDeclaration>(token->value, std::move(params.first),
                                                                         std::move(returnType.value()), params.second);

                if (!consume_op('{')) {
                    if (declarations) {
                        return declaration;
                    } else {
                        error("expected a '{' for the function body");
                    }
                }

                auto prevFuncDecl = current_func_decl;
                // the warning the address of local may escape the function should be ignored
                // because declaration will be moved by return so this is valid
                current_func_decl = declaration.get();
                auto prevReturn = isParseReturnStatement;
                isParseReturnStatement = true;
                auto scope_nodes = parseLoopScope();
                isParseReturnStatement = prevReturn;
                current_func_decl = prevFuncDecl;

                if (!consume_op('}')) {
                    error("expected a '}' for ending the function body");
                } else {
                    declaration->body.emplace(std::move(scope_nodes));
                    return std::move(declaration);
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

bool Parser::parseFunctionDefinitionBool(bool declaration) {
    return parse_return_bool([&]() -> std::optional<std::unique_ptr<FunctionDeclaration>> {
        return parseFunctionDefinition(declaration);
    });
}