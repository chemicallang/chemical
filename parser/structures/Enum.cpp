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

EnumDeclaration* Parser::parseEnumStructureTokens(ASTAllocator& passed_allocator, AccessSpecifier specifier) {
    auto& start_tok = *token;
    if(start_tok.type == TokenType::EnumKw) {
        token++;

        auto id = consumeIdentifierOrKeyword();
        if(!id) {
            error("expected a identifier as enum name");
            return nullptr;
        }

        // all the structs are allocated on global allocator
        // WHY? because when used with imported public generics, the generics tend to instantiate with types
        // referencing the internal structs, which now must be declared inside another module
        // because generics don't check whether the type being used with it is valid in another module
        // once we can be sure which instantiations of generics are being used in module, we can eliminate this
        auto& allocator = global_allocator;

        auto loc = loc_single(start_tok);
        auto decl = new (allocator.allocate<EnumDeclaration>()) EnumDeclaration(loc_id(allocator, id), nullptr, parent_node, loc, specifier);
        annotate(decl);

#ifdef LSP_BUILD
        id->linked = decl;
#endif

        if(consumeToken(TokenType::ColonSym)) {
            auto type = parseTypeLoc(allocator);
            if(type) {
                decl->underlying_type = type;
            } else {
                error("expected a type after ':' in enum declaration");
                return decl;
            }
        } else {
            decl->underlying_type = {new(allocator.allocate<IntType>()) IntType(), loc};
        }

        if(!consumeToken(TokenType::LBrace)) {
            error("expected a '{' for after the enum name");
            return decl;
        }
        int index = 0;
        while(true) {
            consumeNewLines();
            auto memberId = consumeIdentifierOrKeyword();
            if(memberId) {
                const auto member_name = allocate_view(allocator, memberId->value);
                auto member = new (allocator.allocate<EnumMember>()) EnumMember(member_name, index, nullptr, decl, loc_single(memberId));
#ifdef LSP_BUILD
                memberId->linked = member;
#endif
                const auto found = decl->members.find(member_name);
                if(found != decl->members.end()) {
                    error() << "enum already has a member with name " << member_name;
                }
                decl->members[member_name] = member;
                if(consumeToken(TokenType::EqualSym)) {
                    const auto expr = parseExpression(allocator, false, false);
                    if(expr) {
                        auto num = expr->get_number();
                        if(num.has_value()) {
                            // user wants to change the starting index
                            index = static_cast<int>(num.value());
                            // update the index in the member
                            member->set_index_dirty(index);
                        } else {
                            member->init_value = expr;
                        }
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
                break;
            }
        };
        decl->next_start = index;
        if(!consumeToken(TokenType::RBrace)) {
            error("expected a closing bracket '}' in enum block");
            return decl;
        }
        return decl;
    } else {
        return nullptr;
    }
}