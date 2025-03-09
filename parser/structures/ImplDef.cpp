// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 09/03/2024.
//

#include "parser/Parser.h"
#include "ast/structures/ImplDefinition.h"

ImplDefinition* Parser::parseImplTokens(ASTAllocator& allocator, AccessSpecifier specifier) {

    auto& tok = *token;

    if (tok.type == TokenType::ImplKw) {

        token++;

        auto impl = new (allocator.allocate<ImplDefinition>()) ImplDefinition(parent_node, loc_single(tok));

        annotate(impl);

        auto type = parseLinkedOrGenericType(allocator);
        if(type) {
            impl->interface_type = type;
        } else {
            return impl;
        }
        if(consumeWSOfType(TokenType::ForKw)) {
            auto type = parseLinkedOrGenericType(allocator);
            if(type) {
                impl->struct_type = type;
            } else {
                return impl;
            }
        } else {
            impl->struct_type = nullptr;
        }
        if (!consumeToken(TokenType::LBrace)) {
            error("expected a '{' when starting an implementation");
            return impl;
        }

        auto prev_parent_node = parent_node;
        parent_node = impl;
        do {
            consumeNewLines();
            if(parseVariableAndFunctionInto(impl, allocator, AccessSpecifier::Public)) {
                consumeToken(TokenType::SemiColonSym);
            } else {
                break;
            }
        } while(token->type != TokenType::RBrace);
        parent_node = prev_parent_node;

        if (!consumeToken(TokenType::RBrace)) {
            error("expected a '}' when ending an implementation");
            return impl;
        }
        return impl;
    }
    return nullptr;
}