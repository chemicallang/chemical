// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 09/03/2024.
//

#include "parser/Parser.h"

lex_ptr<ImplDefinition> Parser::parseImplementationDefinition() {
    if(consume("impl")) {
        auto interfaceName = consumeOfType<AbstractStringToken>(LexTokenType::Interface);
        if(interfaceName == nullptr) {
            error("interface name missing for implementation declaration");
            return std::nullopt;
        }
        if(!consume("for")) {
            error("missing 'for' keyword after the implementation declaration");
            return std::nullopt;
        }
        auto structName = consumeOfType<AbstractStringToken>(LexTokenType::Struct);
        if(structName == nullptr) {
            error("expected a struct name for implementation declaration for interface " + interfaceName->value);
            return std::nullopt;
        }
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
                error("expected '}' after the impl definition");
            }
            return std::make_unique<ImplDefinition>(structName->value, interfaceName->value, std::move(members));
        }
    }
    return std::nullopt;
}