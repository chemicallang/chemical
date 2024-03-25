// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#include "parser/Parser.h"

lex_ptr<StructDefinition> Parser::parseStructDefinition() {
    if (consume("struct")) {
        auto value = consumeOfType<AbstractStringToken>(LexTokenType::Struct);
        if (value != nullptr) {
            std::optional<std::string> overrides = std::nullopt;
            if(consume_op(':')) {
                auto over = consumeOfType<AbstractStringToken>(LexTokenType::Interface);
                if(over != nullptr) {
                    overrides = over->value;
                } else {
                    error("expected an interface identifier after the ':' when declaring a struct");
                }
            }
            if (consume_op('{')) {
                std::unordered_map<std::string, std::unique_ptr<VarInitStatement>> variables;
                std::unordered_map<std::string, std::unique_ptr<FunctionDeclaration>> functions;
                while(true) {
                    auto init = parseVariableInitStatement();
                    if (init.has_value()) {
                        consume_op(';');
                        variables[init.value()->identifier] = std::move(init.value());
                    } else {
                        auto func = parseFunctionDefinition();
                        if(func.has_value()) {
                            functions[func.value()->name] = std::move(func.value());
                        }else {
                            break;
                        }
                    }
                }
                if(!consume_op('}')) {
                    error("expected a '}' after the struct definition");
                }
                return std::make_unique<StructDefinition>(value->value, std::move(variables), std::move(functions), overrides);
            } else {
                error("expected '{' after the struct name in struct definition");
            }
        } else {
            error("expected a struct name in the struct definition");
        }
    }
    return std::nullopt;
}