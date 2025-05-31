// Copyright (c) Chemical Language Foundation 2025.

#include "parser/Parser.h"
#include "ast/statements/Typealias.h"
#include "ast/structures/GenericTypeDecl.h"
#include "ast/base/TypeBuilder.h"

ASTNode* Parser::parseTypealiasStatement(ASTAllocator& allocator, AccessSpecifier specifier) {
    auto& tok = *token;
    if(tok.type == TokenType::TypeKw) {
        token++;

        auto id = consumeIdentifierOrKeyword();
        if(!id) {
            error("expected a type for typealias statement");
            return nullptr;
        }

        const auto loc = loc_single(tok);
        const auto alias = new (allocator.allocate<TypealiasStatement>()) TypealiasStatement(
                loc_id(allocator, id),
                { (BaseType*) typeBuilder.getVoidType(), ZERO_LOC },
                parent_node,
                loc,
                specifier
        );
        annotate(alias);

#ifdef LSP_BUILD
        id->linked = alias;
#endif

        ASTNode* final_decl = alias;

        if(token->type == TokenType::LessThanSym) {

            std::vector<GenericTypeParameter*> gen_params;
            parseGenericParametersList(allocator, gen_params);

            if (!gen_params.empty()) {

                const auto gen_decl = new(allocator.allocate<GenericTypeDecl>()) GenericTypeDecl(
                        alias, parent_node, loc
                );

                final_decl = gen_decl;

                gen_decl->generic_params = std::move(gen_params);

            }

        }

        if(!consumeToken(TokenType::EqualSym)) {
            error("expected '=' after the type tokens");
        }

        // parsing the actual type
        const auto type = parseTypeLoc(allocator);
        if (type) {

            const auto final_type = parseExpressionType(allocator, type);
            alias->actual_type = { final_type, type.getLocation() };

        } else {
            error("expected a type after '='");
        }

        return final_decl;
    } else {
        return nullptr;
    }
}