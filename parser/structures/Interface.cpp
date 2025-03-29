// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/GenericInterfaceDecl.h"

ASTNode* Parser::parseInterfaceStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier) {

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

        ASTNode* finalDecl = decl;

        if(token->type == TokenType::LessThanSym) {

            std::vector<GenericTypeParameter*> gen_params;

            parseGenericParametersList(allocator, gen_params);

            if (!gen_params.empty()) {

                const auto gen_decl = new(allocator.allocate<GenericInterfaceDecl>()) GenericInterfaceDecl(
                        decl, parent_node, loc_single(id)
                );

                gen_decl->generic_params = std::move(gen_params);

                decl->generic_parent = gen_decl;

                finalDecl = gen_decl;

            }

        }

        if(consumeToken(TokenType::ColonSym)) {
            do {
                auto in_spec = parseAccessSpecifier(AccessSpecifier::Public);
                auto type = parseLinkedOrGenericType(allocator);
                if(!type) {
                    return finalDecl;
                }
                decl->inherited.emplace_back(type, in_spec);
            } while(consumeToken(TokenType::CommaSym));
        }

        if (!consumeToken(TokenType::LBrace)) {
            error("expected a '{' when starting an interface block");
            return finalDecl;
        }

        do {
            consumeNewLines();
            if(parseContainerMembersInto(decl, allocator, AccessSpecifier::Public)) {
                consumeToken(TokenType::SemiColonSym);
            } else {
                break;
            }
        } while(token->type != TokenType::RBrace);
        parent_node = prev_parent_node;

        if (!consumeToken(TokenType::RBrace)) {
            error("expected a '}' when ending an interface block");
            return finalDecl;
        }

        return finalDecl;
    }
    return nullptr;

}