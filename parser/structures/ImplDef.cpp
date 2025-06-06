// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 09/03/2024.
//

#include "parser/Parser.h"
#include "ast/base/TypeBuilder.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/GenericImplDecl.h"

ASTNode* Parser::parseImplTokens(ASTAllocator& allocator, AccessSpecifier specifier) {

    auto& tok = *token;

    if (tok.type == TokenType::ImplKw) {

        token++;

        const auto location = loc_single(tok);

        auto impl = new (allocator.allocate<ImplDefinition>()) ImplDefinition(parent_node, location);
        annotate(impl);

        ASTNode* final_decl = impl;

        if(token->type == TokenType::LessThanSym) {

            std::vector<GenericTypeParameter*> gen_params;

            parseGenericParametersList(allocator, gen_params);

            if(!gen_params.empty()) {

                const auto gen_decl = new(allocator.allocate<GenericImplDecl>()) GenericImplDecl(
                        impl, parent_node, location
                );

                final_decl = gen_decl;

                gen_decl->generic_params = std::move(gen_params);

                impl->generic_parent = gen_decl;

            }

        }

        const auto interfaceTypeLoc = loc_single(token);
        auto type = parseLinkedOrGenericType(allocator);
        if(type) {
            impl->interface_type = {type, interfaceTypeLoc};
        } else {
            impl->interface_type = { (BaseType*) typeBuilder.getVoidType(), ZERO_LOC };
            return final_decl;
        }
        if(consumeToken(TokenType::ForKw)) {
            const auto structTypeLoc = loc_single(token);
            auto structType = parseLinkedOrGenericType(allocator);
            if(structType) {
                impl->struct_type = {structType, structTypeLoc};
            } else {
                return final_decl;
            }
        } else {
            impl->struct_type = nullptr;
        }
        if (!consumeToken(TokenType::LBrace)) {
            error("expected a '{' when starting an implementation");
            return final_decl;
        }

        auto prev_parent_node = parent_node;
        parent_node = impl;
        do {
            consumeNewLines();
            if(parseContainerMembersInto(impl, allocator, AccessSpecifier::Public)) {
                consumeToken(TokenType::SemiColonSym);
            } else {
                break;
            }
        } while(token->type != TokenType::RBrace);
        parent_node = prev_parent_node;

        if (!consumeToken(TokenType::RBrace)) {
            error("expected a '}' when ending an implementation");
            return final_decl;
        }
        return final_decl;
    }
    return nullptr;
}