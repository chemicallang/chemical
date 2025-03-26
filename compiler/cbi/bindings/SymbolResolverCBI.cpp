// Copyright (c) Qinetik 2025.

#include "./SymbolResolverCBI.h"
#include "compiler/SymbolResolver.h"

ASTNode* SymbolResolverfind(SymbolResolver* resolver, chem::string_view* name) {
    return resolver->find(*name);
}