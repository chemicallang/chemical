// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#include "parser/Parser.h"
#include "ast/values/StringValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/IndexOperator.h"

std::optional<std::unique_ptr<AccessChain>> Parser::parseAccessChain() {
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
            }

            chain->values.emplace_back(std::move(std::make_unique<StringValue>(var.value()->value)));

            if(consume_op('(')) {
                std::vector<std::unique_ptr<Value>> params;
                do {
                    auto param = parseAccessChainOrValue();
                    if(param.has_value()) {
                        params.emplace_back(std::move(param.value()));
                    } else {
                        break;
                    }
                } while(consume_op(','));
                if(consume_op(')')) {
                    chain->values.emplace_back(std::make_unique<FunctionCall>(std::move(params)));
                } else {
                    error("expected a ')' after the function call in the access chain");
                    break;
                }
            }

            if(consume_op('[')){
                auto val = parseValueNode();
                if(val.has_value()) {
                    chain->values.emplace_back(std::make_unique<IndexOperator>(std::move(val.value())));
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