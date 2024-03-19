// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#include "parser/Parser.h"

lex_ptr<StructDefinition> Parser::parseStructDefinition() {
    if (consume("struct")) {
        auto value = consumeOfType<AbstractStringToken>(LexTokenType::Struct);
        if (value.has_value()) {
            std::optional<std::string> overrides = std::nullopt;
            if(consume_op(':')) {
                auto over = consumeOfType<AbstractStringToken>(LexTokenType::Interface);
                if(over.has_value()) {
                    overrides = over.value()->value;
                } else {
                    error("expected an interface identifier after the ':' when declaring a struct");
                }
            }
            if (consume_op('{')) {
                std::vector<std::unique_ptr<VarInitStatement>> statements;
                while(true) {
                    auto init = parseVariableInitStatement();
                    if (init.has_value()) {
                        consume_op(';');
                        statements.emplace_back(std::move(init.value()));
                    } else {
                        break;
                    }
                }
                if(!consume_op('}')) {
                    error("expected a '}' after the struct definition");
                }
                return std::make_unique<StructDefinition>(std::move(value.value()->value), std::move(statements), overrides);
            } else {
                error("expected '{' after the struct name in struct definition");
            }
        } else {
            error("expected a struct name in the struct definition");
        }
    }
    return std::nullopt;
}