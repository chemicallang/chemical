// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#include "parser/Parser.h"
#include "lexer/model/tokens/FunctionToken.h"
#include "lexer/model/tokens/ParameterToken.h"
#include "lexer/model/tokens/TypeToken.h"

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

/**
 * parses a single for loop
 * @return true if parsed
 */
lex_ptr<FunctionDeclaration> Parser::parseFunctionDefinition(bool declarations) {
    if (consume("func")) {
        if (token_type() == LexTokenType::Function) {
            auto token = consume<FunctionToken>();
            if (consume_op('(')) {
                func_params params;
                do {
                    if (token_type() == LexTokenType::Parameter) {
                        auto paramToken = consume<ParameterToken>();
                        if (!consume_op(':')) {
                            error("expected a ':' after the parameter for its type");
                            break;
                        }
                        if (token_type() == LexTokenType::Type) {
                            auto typeToken = consume<TypeToken>();
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
                if (consume_op(':')) {
                    if (token_type() == LexTokenType::Type) {
                        returnType = std::move(consume<TypeToken>()->value);
                    } else {
                        error("expected a type after the colon ':' for the function signature");
                        return std::nullopt;
                    }
                } else {
                    returnType = std::nullopt;
                }

                // create declaration before, so that return statement can take a pointer to it
                auto declaration = std::make_unique<FunctionDeclaration>(std::move(token->value), std::move(params),
                                                                         std::move(returnType));

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
                auto scope_nodes = parseScope();
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