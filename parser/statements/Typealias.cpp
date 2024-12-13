// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"
#include "ast/statements/Typealias.h"

TypealiasStatement* Parser::parseTypealiasStatement(ASTAllocator& allocator, AccessSpecifier specifier) {
    auto& tok = *token;
    if(tok.type == TokenType::TypealiasKw) {
        token++;
        readWhitespace();
        auto id = consumeIdentifierOrKeyword();
        if(!id) {
            error("expected a type for typealias statement");
            return nullptr;
        }
        auto alias = new (allocator.allocate<TypealiasStatement>()) TypealiasStatement(loc_id(id), nullptr, parent_node, loc_single(tok), specifier);
        annotate(alias);
        lexWhitespaceToken();
        if(!consumeToken(TokenType::EqualSym)) {
            error("expected '=' after the type tokens");
        }
        lexWhitespaceToken();
        auto type = parseType(allocator);
        if(type) {
            alias->actual_type = type;
        } else {
            error("expected a type after '='");
            return alias;
        }
        return alias;
    } else {
        return nullptr;
    }
}