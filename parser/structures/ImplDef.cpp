// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 09/03/2024.
//

#include "parser/Parser.h"
#include "ast/structures/ImplDefinition.h"

ImplDefinition* Parser::parseImplTokens(ASTAllocator& allocator, AccessSpecifier specifier) {
    if (consumeWSOfType(TokenType::ImplKw)) {

        auto impl = new (allocator.allocate<ImplDefinition>()) ImplDefinition(parent_node, 0);

        annotate(impl);

        parseGenericParametersList(allocator, impl->generic_params);
        lexWhitespaceToken();
        auto type = parseLinkedOrGenericType(allocator);
        if(type) {
            impl->interface_type = type;
        } else {
            return impl;
        }
        lexWhitespaceToken();
        if(consumeWSOfType(TokenType::ForKw)) {
            auto type = parseLinkedOrGenericType(allocator);
            if(type) {
                impl->struct_type = type;
            } else {
                return impl;
            }
            lexWhitespaceToken();
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
            lexWhitespaceAndNewLines();
            if(parseVariableAndFunctionInto(impl, allocator, AccessSpecifier::Public)) {
                lexWhitespaceToken();
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