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
            unexpected_error("expected interface name after the interface keyword");
            return nullptr;
        }

        auto decl = new (allocator.allocate<InterfaceDefinition>()) InterfaceDefinition(loc_id(allocator, id), parent_node, loc_single(tok), specifier);
        annotate(decl);

#ifdef LSP_BUILD
        id->linked = decl;
#endif

        auto prev_parent_node = parent_node;

        ASTNode* finalDecl = decl;

        if(token->type == TokenType::LessThanSym) {

            const auto gen_decl = new(allocator.allocate<GenericInterfaceDecl>()) GenericInterfaceDecl(
                    decl, prev_parent_node, loc_single(id)
            );

            parent_node = gen_decl;

            parseGenericParametersList(allocator, gen_decl->generic_params);

            decl->generic_parent = gen_decl;

            finalDecl = gen_decl;

        } else {

            parent_node = decl;

        }

        if(consumeToken(TokenType::ColonSym)) {
            do {
                auto in_spec = parseAccessSpecifier(AccessSpecifier::Public);
                const auto typeLoc = loc_single(token);
                auto type = parseLinkedOrGenericType(allocator);
                if(!type) {
                    return finalDecl;
                }
                decl->inherited.emplace_back(TypeLoc{type, typeLoc}, in_spec);
            } while(consumeToken(TokenType::CommaSym));
        }

        if (!consumeToken(TokenType::LBrace)) {
            unexpected_error("expected a '{' when starting an interface block");
            return finalDecl;
        }

        bool use_allocator = false;
        if (decl->is_body_retained()) {
            use_allocator = true;
        } else if (finalDecl->kind() == ASTNodeKind::GenericInterfaceDecl) {
            use_allocator = true;
            decl->set_is_body_retained(true);
        } else if (is_linkage_public(specifier)) {
            // default implementations exist in interface methods that must be kept cross module for instantiations
            use_allocator = true;
            decl->set_is_body_retained(false);
        }

        // bodies of functions will be allocated on the passed allocator only if
        // container is a generic interface, otherwise we can allocate on mod_allocator
        auto& body_allocator = use_allocator ? allocator : mod_allocator;

        parent_node = finalDecl;
        parseContainerMembersInto(decl, allocator, body_allocator, AccessSpecifier::Public, false);
        parent_node = prev_parent_node;

        if (!consumeToken(TokenType::RBrace)) {
            unexpected_error("expected a '}' when ending an interface block");
            return finalDecl;
        }

        return finalDecl;
    }
    return nullptr;

}