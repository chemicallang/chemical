// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/EnumMember.h"
#include "ast/values/IntValue.h"
#include "ast/types/IntType.h"
#include "parser/utils/parse_num.h"

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

        auto loc = loc_single(start_tok);
        auto decl = new (allocator.allocate<EnumDeclaration>()) EnumDeclaration(loc_id(allocator, id), nullptr, parent_node, loc, specifier);

        annotate(decl);

        lexWhitespaceToken();

        if(consumeToken(TokenType::ColonSym)) {
            lexWhitespaceToken();
            auto type = parseType(allocator);
            if(type) {
                const auto k = type->kind();
                if(k == BaseTypeKind::IntN || k == BaseTypeKind::Char || k == BaseTypeKind::UChar) {
                    decl->underlying_type = (IntNType*) type;
                } else {
                    error("expected a integer type after ':' in enum declaration");
                };
            } else {
                error("expected a type after ':' in enum declaration");
                return decl;
            }
        } else {
            decl->underlying_type = new (allocator.allocate<IntType>()) IntType(loc);
        }

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
                const auto member_name = allocate_view(allocator, memberId->value);
                auto member = new (allocator.allocate<EnumMember>()) EnumMember(member_name, index, nullptr, decl, loc_single(memberId));
                decl->members[member_name] = member;
                lexWhitespaceToken();
                if(consumeToken(TokenType::EqualSym)) {
                    lexWhitespaceToken();
                    if(token->type == TokenType::Number) {
                        auto& value = token->value;
                        auto expr = parse_num(value.data(), value.size(), strtol);
                        if(expr.error.empty()) {
                            member->init_value = decl->underlying_type->create(allocator, expr.result);
                        } else {
                            error("error parsing the number in enum member '" + std::string(expr.error) + "'");
                            return decl;
                        }
                    } else {
                        error("expected a number value after '=' operator");
                        return decl;
                    }
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