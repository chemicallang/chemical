// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/InterfaceDefinition.h"

InterfaceDefinition* Parser::parseInterfaceStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier) {

    auto& tok = *token;

    if (tok.type == TokenType::InterfaceKw) {
        token++;
        auto id = consumeIdentifierOrKeyword();
        if(!id) {
            error("expected interface name after the interface keyword");
            return nullptr;
        }
        auto decl = new (allocator.allocate<InterfaceDefinition>()) InterfaceDefinition(loc_id(allocator, id), parent_node, loc_single(tok), specifier);

        annotate(decl);

        auto prev_parent_node = parent_node;
        parent_node = decl;

        parseGenericParametersList(allocator, decl->generic_params);

        if (!consumeToken(TokenType::LBrace)) {
            error("expected a '{' when starting an interface block");
            return decl;
        }

        do {
            consumeNewLines();
            if(parseVariableAndFunctionInto(decl, allocator, AccessSpecifier::Public)) {
                consumeToken(TokenType::SemiColonSym);
            } else {
                break;
            }
        } while(token->type != TokenType::RBrace);
        parent_node = prev_parent_node;

        if (!consumeToken(TokenType::RBrace)) {
            error("expected a '}' when ending an interface block");
            return decl;
        }
        return decl;
    }
    return nullptr;

}