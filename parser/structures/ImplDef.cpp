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

            // `impl <T> I for S<T>` is a generic implementation. Nested implementations
            // (inside a struct/union/variant) keep rejecting their own parameters: the
            // enclosing type already owns the parameter names, and shadowing them has no
            // defined semantics (see lang/docs/generic-impls-implementation-plan.md).
            const auto parent_kind = prev_parent_node ? prev_parent_node->kind() : ASTNodeKind::FileScope;
            const auto nested = parent_kind == ASTNodeKind::StructDecl ||
                                parent_kind == ASTNodeKind::UnionDecl ||
                                parent_kind == ASTNodeKind::VariantDecl ||
                                parent_kind == ASTNodeKind::GenericStructDecl ||
                                parent_kind == ASTNodeKind::GenericUnionDecl ||
                                parent_kind == ASTNodeKind::GenericVariantDecl;

            if(nested) {

                error("implementations inside a type don't declare generic parameters, "
                      "use a top-level implementation for a generic type");

                // consume the parameter list, so the rest of the declaration parses normally
                std::vector<GenericTypeParameter*> discarded_params;
                parseGenericParametersList(allocator, discarded_params);

                parent_node = impl;

            } else {

                const auto gen_decl = new (allocator.allocate<GenericImplDecl>()) GenericImplDecl(
                        impl, prev_parent_node, location
                );

                parent_node = gen_decl;

                parseGenericParametersList(allocator, gen_decl->generic_params, &gen_decl->lifetime_params);

                impl->generic_parent = gen_decl;
                final_decl = gen_decl;

            }

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
                // the 'for' keyword was consumed, so a type must follow. Leaving the
                // target type null here would make the declaration look like an
                // implementation without a target type, which no later pass expects.
                error("expected a type after 'for' in an implementation");
                impl->struct_type = getErroredType(allocator);
            }
        } else {
            // every implementation must name the type it is implemented for
            error("expected 'for' after the interface name in an implementation");
            impl->struct_type = getErroredType(allocator);
        }
        if (!consumeToken(TokenType::LBrace)) {
            error("expected a '{' when starting an implementation");
            return final_decl;
        }

        // members (and therefore their functions) belong to the ImplDefinition, not
        // to the generic wrapper. Only the header was parsed in the wrapper scope.
        parent_node = impl;

        // bodies of functions will be allocated on the passed allocator only if
        // containing is a generic impl, otherwise we can allocate on mod_allocator
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