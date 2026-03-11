// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <cstdint>

class ASTNode;

class SymbolResolver;

class SymResLinkBody;

class ASTBuilder;

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

    void SymbolResolverdeclare_or_shadow(SymbolResolver* resolver, chem::string_view* name, ASTNode* node);

    void SymbolResolverscope_start(SymbolResolver* resolver);

    void SymbolResolverscope_end(SymbolResolver* resolver);

    void SymbolResolverdeclare_exported(SymbolResolver* resolver, chem::string_view* name, ASTNode* node);

    void SymbolResolvergetJobBuilder(ASTBuilder* out_builder, SymbolResolver* resolver);

    void SymbolResolvergetModBuilder(ASTBuilder* out_builder, SymbolResolver* resolver);

    void SymbolResolvergetFileBuilder(ASTBuilder* out_builder, SymbolResolver* resolver);

    // SymResLinkBody

    SymbolResolver* SymResLinkBodygetSymbolResolver(SymResLinkBody* visitor);

    void SymResLinkBodyvisitNode(SymResLinkBody* visitor, EmbeddedNode* node);

    void SymResLinkBodyvisitValue(SymResLinkBody* visitor, EmbeddedValue* value);

}