// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#include "parser/Parser.h"

lex_ptr<EnumDeclaration> Parser::parseEnumDeclaration() {
    if(consume("enum")) {
        auto name = consumeOfType<AbstractStringToken>(LexTokenType::Enum);
        if(name != nullptr) {
            if(consume_op('{')) {
                std::vector<std::string> members;
                do {
                    auto member = consumeOfType<AbstractStringToken>(LexTokenType::EnumMember);
                    if(member != nullptr) {
                        members.emplace_back(member->value);
                    } else {
                        break;
                    }
                } while(consume_op(','));
                if(consume_op('}')) {
                    return std::make_unique<EnumDeclaration>(name->value, std::move(members));
                } else {
                    error("expected a '}' for closing the enum");
                }
            } else {
                error("expected a '{' operator after the enum name identifier");
            }
        } else {
            error("expected a enum name identifier");
        }
    }
    return std::nullopt;
}