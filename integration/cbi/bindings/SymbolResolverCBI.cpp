// Copyright (c) Qinetik 2025.

#include "./SymbolResolverCBI.h"
#include "compiler/SymbolResolver.h"

ASTNode* SymbolResolverfind(SymbolResolver* resolver, chem::string_view* name) {
    return resolver->find(*name);
}

ASTNode* SymbolResolverfind_in_current_file(SymbolResolver* resolver, chem::string_view* name) {
    return resolver->find_in_current_file(*name);
}