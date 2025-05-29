// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/UnnamedUnion.h"

UnnamedUnion* Parser::parseUnnamedUnion(ASTAllocator& allocator, AccessSpecifier specifier) {

    if(consumeToken(TokenType::UnionKw)) {

        auto decl = new (allocator.allocate<UnnamedUnion>()) UnnamedUnion("", parent_node, loc_single(token), specifier);
        annotate(decl);

        if(!consumeToken(TokenType::LBrace)) {
            error("expected a '{' for union block");
            return decl;
        }

        auto prev_parent_node = parent_node;
        parent_node = decl;
        do {
            consumeNewLines();
            if(parseContainerMembersInto(decl, allocator, AccessSpecifier::Public)) {
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
#ifdef LSP_BUILD
            id->linked = decl;
#endif
            decl->set_encoded_location(loc_single(id));
            decl->name = allocate_view(allocator, id->value);
        }
        return decl;
    } else {
        return nullptr;
    }

}

ASTNode* Parser::parseUnionStructureTokens(ASTAllocator& passed_allocator, AccessSpecifier specifier) {

    if(consumeToken(TokenType::UnionKw)) {

        auto identifier = consumeIdentifierOrKeyword();
        if (!identifier) {
            error("expected a identifier as struct name");
            return nullptr;
        }

        // all the structs are allocated on global allocator
        // WHY? because when used with imported public generics, the generics tend to instantiate with types
        // referencing the internal structs, which now must be declared inside another module
        // because generics don't check whether the type being used with it is valid in another module
        // once we can be sure which instantiations of generics are being used in module, we can eliminate this
        auto& allocator = global_allocator;

        auto decl = new (allocator.allocate<UnionDef>()) UnionDef(loc_id(allocator, identifier), parent_node, loc_single(identifier), specifier);
        annotate(decl);

#ifdef LSP_BUILD
        identifier->linked = decl;
#endif

        ASTNode* finalDecl = decl;

        if(token->type == TokenType::LessThanSym) {

            std::vector<GenericTypeParameter*> gen_params;

            parseGenericParametersList(allocator, gen_params);

            if (!gen_params.empty()) {

                const auto gen_decl = new(allocator.allocate<GenericUnionDecl>()) GenericUnionDecl(
                        decl, parent_node, loc_single(identifier)
                );

                gen_decl->generic_params = std::move(gen_params);

                decl->generic_parent = gen_decl;

                finalDecl = gen_decl;

            }

        }

        if(!consumeToken(TokenType::LBrace)) {
            error("expected a '{' for union block");
            return finalDecl;
        }

        auto prev_parent_node = parent_node;
        parent_node = decl;

        do {
            consumeNewLines();
            if(parseContainerMembersInto(decl, passed_allocator, AccessSpecifier::Public)) {
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