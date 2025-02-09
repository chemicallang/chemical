// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"
#include "ast/statements/Typealias.h"

TypealiasStatement* Parser::parseTypealiasStatement(ASTAllocator& allocator, AccessSpecifier specifier) {
    auto& tok = *token;
    if(tok.type == TokenType::TypeKw) {
        token++;
        auto id = consumeIdentifierOrKeyword();
        if(!id) {
            error("expected a type for typealias statement");
            return nullptr;
        }
        if(!consumeToken(TokenType::EqualSym)) {
            error("expected '=' after the type tokens");
        }
        auto type = parseType(allocator);
        if (!type) {
            error("expected a type after '='");
        }
        auto alias = new (allocator.allocate<TypealiasStatement>()) TypealiasStatement(loc_id(allocator, id), type, parent_node, loc_single(tok), specifier);
        annotate(alias);
        return alias;
    } else {
        return nullptr;
    }
}