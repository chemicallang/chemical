// Copyright (c) Chemical Language Foundation 2025.

#include "./SymbolResolverCBI.h"
#include "compiler/SymbolResolver.h"

AnnotationController* SymbolResolvergetAnnotationController(SymbolResolver* resolver) {
    return &resolver->controller;
}

ASTNode* SymbolResolverfind(SymbolResolver* resolver, chem::string_view* name) {
    return resolver->find(*name);
}

void SymbolResolverdeclare(SymbolResolver* resolver, chem::string_view* name, ASTNode* node) {
    resolver->declare(*name, node);
}