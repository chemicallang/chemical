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
        parseContainerMembersInto(decl, allocator, allocator, AccessSpecifier::Public, false);
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

ASTNode* Parser::parseUnionStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier) {

    if(consumeToken(TokenType::UnionKw)) {

        auto identifier = consumeIdentifierOrKeyword();
        if (!identifier) {
            error("expected a identifier as struct name");
            return nullptr;
        }

        auto decl = new (allocator.allocate<UnionDef>()) UnionDef(loc_id(allocator, identifier), parent_node, loc_single(identifier), specifier);
        annotate(decl);

#ifdef LSP_BUILD
        identifier->linked = decl;
#endif

        auto prev_parent_node = parent_node;

        ASTNode* finalDecl = decl;

        if(token->type == TokenType::LessThanSym) {

            const auto gen_decl = new(allocator.allocate<GenericUnionDecl>()) GenericUnionDecl(
                    decl, prev_parent_node, loc_single(identifier)
            );

            parent_node = gen_decl;

            parseGenericParametersList(allocator, gen_decl->generic_params);

            decl->generic_parent = gen_decl;

            finalDecl = gen_decl;

        } else {

            parent_node = decl;

        }

        if(!consumeToken(TokenType::LBrace)) {
            error("expected a '{' for union block");
            return finalDecl;
        }

        bool use_allocator = false;
        if (decl->is_body_retained()) {
            use_allocator = true;
        } else if (finalDecl->kind() == ASTNodeKind::GenericUnionDecl) {
            use_allocator = true;
            decl->set_is_body_retained(true);
        }

        // bodies of functions will be allocated on the passed allocator only if
        // containing is a generic struct, otherwise we can allocate on mod_allocator
        auto& body_allocator = use_allocator ? allocator : mod_allocator;

        parseContainerMembersInto(decl, allocator, body_allocator, AccessSpecifier::Public, false, true);

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