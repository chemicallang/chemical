// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#include "parser/Parser.h"
#include "ast/values/StringValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/VariableIdentifier.h"

lex_ptr<AccessChain> Parser::parseAccessChain() {
    std::unique_ptr<AccessChain> chain;
    do {
        auto var = consumeOfType<AbstractStringToken>(LexTokenType::Variable, false);
        if (!var.has_value()) {
            if (chain) {
                error("Expected a value after the dot in the access chain");
                break;
            } else {
                return std::nullopt;
            }
        } else {

            if (!chain) {

                chain = std::make_unique<AccessChain>(std::vector<std::unique_ptr<Value>>());

                // parsea  struct value
                if(token_type() == LexTokenType::CharOperator && as<CharOperatorToken>()->op == '{') {
                    auto structValue = parseStructValue(var.value()->value);
                    if(structValue.has_value()) {
                        chain->values.push_back(std::move(structValue.value()));
                    } else {
                        error("expected a struct after encountering '{' after struct value declaration " + var.value()->value);
                    }
                    return chain;
                }

            }
            if(consume_op('(')) {
                std::vector<std::unique_ptr<Value>> params;
                do {
                    auto param = parseExpression();
                    if(param.has_value()) {
                        params.emplace_back(std::move(param.value()));
                    } else {
                        break;
                    }
                } while(consume_op(','));
                if(consume_op(')')) {
                    chain->values.emplace_back(std::make_unique<FunctionCall>(std::move(var.value()->value), std::move(params)));
                } else {
                    error("expected a ')' after the function call in the access chain");
                    break;
                }
            } else {
                chain->values.emplace_back(std::move(std::make_unique<VariableIdentifier>(var.value()->value)));
            }

            if(consume_op('[')){
                auto val = parseExpression();
                if(val.has_value()) {
                    // TODO index operator shouldn't take variable identifier
                    chain->values.emplace_back(std::make_unique<IndexOperator>(std::move(var.value()->value), std::move(val.value())));
                } else {
                    error("expected a value for the index operators '[]' in the access chain");
                    break;
                }
                if(!consume_op(']')) {
                    error("expected a ']' after the opening '[' in the access chain");
                    break;
                }
            }

        }
    } while (consume_op('.'));
    if(chain->values.empty()) {
        return std::nullopt;
    } else {
        return chain;
    }
}