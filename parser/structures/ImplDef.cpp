// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 09/03/2024.
//

#include "parser/Parser.h"
#include "ast/base/TypeBuilder.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/GenericImplDecl.h"

ASTNode* Parser::parseImplTokens(ASTAllocator& allocator, ASTAllocator& body_allocator, AccessSpecifier specifier) {

    auto& tok = *token;

    if (tok.type == TokenType::ImplKw) {

        token++;

        const auto location = loc_single(tok);

        auto impl = new (allocator.allocate<ImplDefinition>()) ImplDefinition(parent_node, location, specifier);
        annotate(impl);

        ASTNode* final_decl = impl;

        auto prev_parent_node = parent_node;

        if(token->type == TokenType::LessThanSym) {

            const auto gen_decl = new(allocator.allocate<GenericImplDecl>()) GenericImplDecl(
                    impl, prev_parent_node, location
            );

            parent_node = gen_decl;

            parseGenericParametersList(allocator, gen_decl->generic_params);

            final_decl = gen_decl;

            impl->generic_parent = gen_decl;

        } else {

            parent_node = impl;

        }

        auto typeToken = token;
        const auto idType = typeToken->type;
        if(Token::isKeywordOrId(idType)) {
            token++;
        } else {
            error("expected keyword or identifier for interface type in impl");
            impl->interface_type = { (BaseType*) typeBuilder.getVoidType(), ZERO_LOC };
            return final_decl;
        }
        const auto typeLoc = loc_single(typeToken);
        BaseType* type;
        if(token->type == TokenType::DoubleColonSym || token->type == TokenType::DotSym) {

            const auto linkedValueType = parseLinkedValueType(allocator, typeToken, typeLoc);

            type = parseGenericTypeAfterId(allocator, (BaseType*) linkedValueType);

        } else {

            const auto namedLinkedType = new(allocator.allocate<NamedLinkedType>()) NamedLinkedType(allocate_view(allocator, typeToken->value));
            type = parseGenericTypeAfterId(allocator, namedLinkedType);

        }
        impl->interface_type = {type, typeLoc};

        if(consumeToken(TokenType::ForKw)) {
            auto structType = parseTypeLoc(allocator);
            if(structType) {
                impl->struct_type = structType;
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

        // bodies of functions will be allocated on the passed allocator only if
        // containing is a generic struct, otherwise we can allocate on mod_allocator
        auto& body_allocator_final = final_decl->kind() == ASTNodeKind::GenericImplDecl ? allocator : body_allocator;

        parseContainerMembersInto(impl, allocator, body_allocator_final, AccessSpecifier::Public, false);
        parent_node = prev_parent_node;

        if (!consumeToken(TokenType::RBrace)) {
            error("expected a '}' when ending an implementation");
            return final_decl;
        }
        return final_decl;
    }
    return nullptr;
}