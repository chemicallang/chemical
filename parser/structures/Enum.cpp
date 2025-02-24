// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/EnumMember.h"
#include "ast/values/IntValue.h"
#include "ast/values/NumberValue.h"
#include "ast/types/IntType.h"
#include "parser/utils/parse_num.h"

EnumDeclaration* Parser::parseEnumStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier) {
    auto& start_tok = *token;
    if(start_tok.type == TokenType::EnumKw) {
        token++;
        auto id = consumeIdentifierOrKeyword();
        if(!id) {
            error("expected a identifier as enum name");
            return nullptr;
        }

        auto loc = loc_single(start_tok);
        auto decl = new (allocator.allocate<EnumDeclaration>()) EnumDeclaration(loc_id(allocator, id), nullptr, parent_node, loc, specifier);

        annotate(decl);

        if(consumeToken(TokenType::ColonSym)) {
            auto type = parseType(allocator);
            if(type) {
                decl->underlying_type = type;
            } else {
                error("expected a type after ':' in enum declaration");
                return decl;
            }
        } else {
            decl->underlying_type = new (allocator.allocate<IntType>()) IntType(loc);
        }

        if(!consumeToken(TokenType::LBrace)) {
            error("expected a '{' for after the enum name");
            return decl;
        }
        unsigned int index = 0;
        while(true) {
            consumeNewLines();
            auto memberId = consumeIdentifierOrKeyword();
            if(memberId) {
                const auto member_name = allocate_view(allocator, memberId->value);
                auto member = new (allocator.allocate<EnumMember>()) EnumMember(member_name, index, nullptr, decl, loc_single(memberId));
                const auto found = decl->members.find(member_name);
                if(found != decl->members.end()) {
                    error("enum already has a member with name " + member_name.str());
                }
                decl->members[member_name] = member;
                if(consumeToken(TokenType::EqualSym)) {
                    const auto expr = parseExpression(allocator, false, false);
                    if(expr) {
                        member->init_value = expr;
                    } else {
                        error("expected a value after '=' operator");
                        return decl;
                    }
                }
                index++;
                if(consumeToken(TokenType::CommaSym)) {
                    continue;
                } else {
                    consumeNewLines();
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