// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/Scope.h"

Namespace* Parser::parseNamespace(ASTAllocator& allocator, AccessSpecifier specifier) {
    auto& tok = *token;
    if(tok.type == TokenType::NamespaceKw) {
        token++;
        readWhitespace();
        auto id = consumeIdentifierOrKeyword();
        if(!id) {
            error("expected identifier for namespace name");
            return nullptr;
        }
        auto ns = new (allocator.allocate<Namespace>()) Namespace(id->value.str(), parent_node, loc_single(tok), specifier);
        annotate(ns);
        auto prev_parent_node = parent_node;
        parent_node = ns;
        auto result = parseTopLevelBraceBlock(allocator, "namespace");
        if(result.has_value()) {
            ns->nodes = std::move(result.value().nodes);
        }
        parent_node = prev_parent_node;
        return ns;
    }
    return nullptr;
}