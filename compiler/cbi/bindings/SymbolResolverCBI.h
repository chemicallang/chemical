// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <cstdint>

class ASTNode;

class SymbolResolver;

class AnnotationController;

namespace chem {
    class string_view;
}

extern "C" {

    AnnotationController* SymbolResolvergetAnnotationController(SymbolResolver* resolver);

    ASTNode* SymbolResolverfind(SymbolResolver* resolver, chem::string_view* name);

    void SymbolResolverdeclare(SymbolResolver* resolver, chem::string_view* name, ASTNode* node);

}