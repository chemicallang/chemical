// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#include "parser/Parser.h"
#include "ast/structures/EnumDeclaration.h"

lex_ptr<EnumDeclaration> Parser::parseEnumDeclaration() {
    if (consume("enum")) {
        auto name = consumeOfType<AbstractStringToken>(LexTokenType::Variable);
        if (name != nullptr) {
            if (consume_op('{')) {
                std::unordered_map<std::string, unsigned int> members;
                unsigned member_pos = 0;
                do {
                    auto member = consumeOfType<AbstractStringToken>(LexTokenType::Variable);
                    if (member != nullptr) {
                        members[member->value] = member_pos;
                        member_pos++;
                    } else {
                        break;
                    }
                } while (consume_op(','));
                if (consume_op('}')) {
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

bool Parser::parseEnumDeclarationBool() {
    return parse_return_bool([&]() -> lex_ptr<EnumDeclaration> {
        return parseEnumDeclaration();
    });
}