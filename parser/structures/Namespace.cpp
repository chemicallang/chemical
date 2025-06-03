// Copyright (c) Chemical Language Foundation 2025.

#include "parser/Parser.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/Scope.h"

Namespace* Parser::parseNamespace(ASTAllocator& allocator, AccessSpecifier specifier) {
    auto& tok = *token;
    if(tok.type == TokenType::NamespaceKw) {
        token++;
        auto id = consumeIdentifierOrKeyword();
        if(!id) {
            unexpected_error("expected identifier for namespace name");
            return nullptr;
        }
        auto ns = new (allocator.allocate<Namespace>()) Namespace(loc_id(allocator, id), parent_node, loc_single(tok), specifier);
        annotate(ns);

#ifdef LSP_BUILD
        id->linked = ns;
#endif

        auto prev_parent_node = parent_node;
        parent_node = ns;
        // we pass the module allocator
        // because public nodes inside namespace still use global allocator and
        // non public nodes will use this module allocator, because we explicitly
        // remove these nodes when the module has generated code
        auto result = parseTopLevelBraceBlock(mod_allocator, "namespace");
        if(result.has_value()) {
            ns->nodes = std::move(result.value().nodes);
        }
        parent_node = prev_parent_node;
        return ns;
    }
    return nullptr;
}