// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/EnumMember.h"
#include "ast/values/IntValue.h"

EnumDeclaration* Parser::parseEnumStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier) {
    auto& start_tok = *token;
    if(start_tok.type == TokenType::EnumKw) {
        token++;
        readWhitespace();
        auto id = consumeIdentifierOrKeyword();
        if(!id) {
            error("expected a identifier as enum name");
            return nullptr;
        }
        auto decl = new (allocator.allocate<EnumDeclaration>()) EnumDeclaration(loc_id(allocator, id), {}, parent_node, loc_single(start_tok), specifier);

        annotate(decl);

        lexWhitespaceToken();
        if(!consumeToken(TokenType::LBrace)) {
            error("expected a '{' for after the enum name");
            return decl;
        }
        unsigned int index = 0;
        while(true) {
            lexWhitespaceAndNewLines();
            auto memberId = consumeIdentifierOrKeyword();
            if(memberId) {
                auto member = new (allocator.allocate<EnumMember>()) EnumMember(memberId->value.str(), index, nullptr, decl, loc_single(memberId));
                decl->members[memberId->value.str()] = member;
                lexWhitespaceToken();
                if(consumeToken(TokenType::EqualSym)) {
                    lexWhitespaceToken();
                    auto expr = parseExpression(allocator);
                    if(expr) {
                        member->init_value = expr;
                    } else {
                        error("expected a value after '=' operator");
                        return decl;
                    }
                } else {
                    member->init_value = new (allocator.allocate<IntValue>()) IntValue((int) index, loc_single(memberId));;
                }
                index++;
                if(consumeToken(TokenType::CommaSym)) {
                    continue;
                } else {
                    lexWhitespaceAndNewLines();
                    break;
                }
            } else {
                auto singly = parseSingleLineComment(allocator);
                if(singly) {
                    continue;
                } else {
                    auto multily = parseMultiLineComment(allocator);
                    if(multily) {
                        continue;
                    }
                }
                break;
            }
        };
        if(!consumeToken(TokenType::RBrace)) {
            error("expected a closing bracket '}' in enum block");
            return decl;
        }
        return decl;
    } else {
        return nullptr;
    }
}