// Copyright (c) Chemical Language Foundation 2025.

#include "./SymbolResolverCBI.h"
#include "compiler/SymbolResolver.h"
#include "compiler/symres/SymResLinkBody.h"
#include "ast/statements/EmbeddedNode.h"
#include "ast/values/EmbeddedValue.h"

AnnotationController* SymbolResolvergetAnnotationController(SymbolResolver* resolver) {
    return &resolver->controller;
}

ASTNode* SymbolResolverfind(SymbolResolver* resolver, chem::string_view* name) {
    return resolver->find(*name);
}

void SymbolResolverdeclare(SymbolResolver* resolver, chem::string_view* name, ASTNode* node) {
    resolver->declare(*name, node);
}

void SymbolResolverdeclare_exported(SymbolResolver* resolver, chem::string_view* name, ASTNode* node) {
    resolver->declare_exported(*name, node);
}

SymbolResolver* SymResLinkBodygetSymbolResolver(SymResLinkBody* visitor) {
    return &visitor->linker;
}

void SymResLinkBodyvisitNode(SymResLinkBody* visitor, EmbeddedNode* node) {
    auto& linker = visitor->linker;
    for(const auto child_node : node->chemical_nodes) {
        linker.scope_start();
        visitor->visit(child_node);
        linker.scope_end();
    }
    for(const auto child_val : node->chemical_values) {
        visitor->visit(child_val);
    }
}

void SymResLinkBodyvisitValue(SymResLinkBody* visitor, EmbeddedValue* value) {
    auto& linker = visitor->linker;
    for(const auto child_node : value->chemical_nodes) {
        linker.scope_start();
        visitor->visit(child_node);
        linker.scope_end();
    }
    for(const auto child_val : value->chemical_values) {
        visitor->visit(child_val);
    }
}