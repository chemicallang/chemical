// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <cstdint>

class ASTNode;

class SymbolResolver;

class SymResLinkBody;

class EmbeddedNode;
class EmbeddedValue;

class AnnotationController;

namespace chem {
    class string_view;
}

extern "C" {

    AnnotationController* SymbolResolvergetAnnotationController(SymbolResolver* resolver);

    ASTNode* SymbolResolverfind(SymbolResolver* resolver, chem::string_view* name);

    void SymbolResolverdeclare(SymbolResolver* resolver, chem::string_view* name, ASTNode* node);

    void SymbolResolverdeclare_exported(SymbolResolver* resolver, chem::string_view* name, ASTNode* node);

    // SymResLinkBody

    SymbolResolver* SymResLinkBodygetSymbolResolver(SymResLinkBody* visitor);

    void SymResLinkBodyvisitNode(SymResLinkBody* visitor, EmbeddedNode* node);

    void SymResLinkBodyvisitValue(SymResLinkBody* visitor, EmbeddedValue* value);

}