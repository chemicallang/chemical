// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#include "parser/Parser.h"

lex_ptr<InterfaceDefinition> Parser::parseInterfaceDefinition() {
    if(consume("interface")){
        auto name = consumeOfType<AbstractStringToken>(LexTokenType::Interface);
        if(name != nullptr) {
            if(consume_op('{')) {
                std::vector<std::unique_ptr<ASTNode>> members;
                while(true) {
                    auto init = parseVariableInitStatement();
                    if(init.has_value()) {
                        members.emplace_back(std::move(init.value()));
                        consume_op(';');
                    } else {
                        auto fun = parseFunctionDefinition(true);
                        if(fun.has_value()) {
                            members.emplace_back(std::move(fun.value()));
                            consume_op(';');
                        } else {
                            break;
                        }
                    }
                }
                if(!consume_op('}')) {
                    error("expected '}' after the interface definition");
                }
                return std::make_unique<InterfaceDefinition>(name->value, std::move(members));
            }
        } else {
            error("expected a interface name identifier after the keyword interface");
        }
    }
    return std::nullopt;
}