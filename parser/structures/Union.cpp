// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/UnnamedUnion.h"

UnnamedUnion* Parser::parseUnnamedUnion(ASTAllocator& allocator, AccessSpecifier specifier) {

    if(consumeWSOfType(TokenType::UnionKw)) {


        auto decl = new (allocator.allocate<UnnamedUnion>()) UnnamedUnion("", parent_node, 0, specifier);

        annotate(decl);

        if(!consumeToken(TokenType::LBrace)) {
            error("expected a '{' for union block");
            return decl;
        }

        auto prev_parent_node = parent_node;
        parent_node = decl;
        do {
            consumeNewLines();
            if(parseVariableMemberInto(decl, allocator, AccessSpecifier::Public)) {
                consumeToken(TokenType::SemiColonSym);
            } else {
                break;
            }
        } while(token->type != TokenType::RBrace);
        parent_node = prev_parent_node;

        if(!consumeToken(TokenType::RBrace)) {
            error("expected a closing bracket '}' for union block");
            return decl;
        }
        auto id = consumeIdentifierOrKeyword();
        if(id) {
            decl->name = allocate_view(allocator, id->value);
        } else {
            error("expected an identifier after the '}' for anonymous union definition");
            return decl;
        }
        return decl;
    } else {
        return nullptr;
    }

}

const auto GENv2 = true;

ASTNode* Parser::parseUnionStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier) {

    if(consumeWSOfType(TokenType::UnionKw)) {

        auto identifier = consumeIdentifierOrKeyword();
        if (!identifier) {
            error("expected a identifier as struct name");
            return nullptr;
        }

        auto decl = new (allocator.allocate<UnionDef>()) UnionDef(loc_id(allocator, identifier), parent_node, 0, specifier);

        annotate(decl);

        std::vector<GenericTypeParameter*> gen_params;

        parseGenericParametersList(allocator, gen_params);

        ASTNode* finalDecl = decl;

        if(GENv2 && !gen_params.empty()) {

            const auto gen_decl = new (allocator.allocate<GenericUnionDecl>()) GenericUnionDecl(
                    decl, parent_node, loc_single(identifier)
            );

            gen_decl->generic_params = std::move(gen_params);

            finalDecl = gen_decl;

        } else {
            decl->generic_params = std::move(gen_params);
        }

        if(!consumeToken(TokenType::LBrace)) {
            error("expected a '{' for union block");
            return finalDecl;
        }

        auto prev_parent_node = parent_node;
        parent_node = decl;

        do {
            consumeNewLines();
            if(parseVariableAndFunctionInto(decl, allocator, AccessSpecifier::Public)) {
                consumeToken(TokenType::SemiColonSym);
            } else {
                break;
            }
        } while(token->type != TokenType::RBrace);

        parent_node = prev_parent_node;

        if(!consumeToken(TokenType::RBrace)) {
            error("expected a closing bracket '}' for union block");
            return finalDecl;
        }

        return finalDecl;
    } else {
        return nullptr;
    }

}