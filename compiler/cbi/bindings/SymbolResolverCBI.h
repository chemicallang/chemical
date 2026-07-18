// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <cstdint>

class ASTNode;

class Value;

class SymbolResolver;

class ASTDiagnoser;

class SymResLinkBody;
class TopLevelLinkSignature;
class SymbolTable;

class ASTBuilder;

class EmbeddedNode;
class EmbeddedValue;

class AnnotationController;

namespace chem {
    class string_view;
}

extern "C" {

    AnnotationController* SymbolResolvergetAnnotationController(SymbolResolver* resolver);

    ASTNode* SymbolResolverresolve(SymbolResolver* resolver, chem::string_view* name);

    void SymbolResolverdeclare(SymbolResolver* resolver, chem::string_view* name, ASTNode* node);

    void SymbolResolverdeclare_tld_default(SymbolResolver* resolver, chem::string_view* name, ASTNode* node);

    void SymbolResolverdeclare_or_shadow(SymbolResolver* resolver, chem::string_view* name, ASTNode* node);

    void SymbolResolverscope_start(SymbolResolver* resolver);

    void SymbolResolverscope_end(SymbolResolver* resolver);

    void SymbolResolvergetJobBuilder(ASTBuilder* out_builder, SymbolResolver* resolver);

    void SymbolResolvergetModBuilder(ASTBuilder* out_builder, SymbolResolver* resolver);

    void SymbolResolvergetFileBuilder(ASTBuilder* out_builder, SymbolResolver* resolver);

    // SymResLinkBody

    SymbolResolver* SymResLinkBodygetSymbolResolver(SymResLinkBody* visitor);

    ASTDiagnoser* SymResLinkBodygetAstDiagnoser(SymResLinkBody* visitor);

    SymbolTable* SymResLinkBodygetSymbolTable(SymResLinkBody* visitor);

    void SymResLinkBodyvisitNode(SymResLinkBody* visitor, ASTNode* node);

    void SymResLinkBodyvisitValue(SymResLinkBody* visitor, Value* value);

    void SymResLinkBodyvisitEmbeddedNode(SymResLinkBody* visitor, EmbeddedNode* node);

    void SymResLinkBodyvisitEmbeddedValue(SymResLinkBody* visitor, EmbeddedValue* value);

    // SymbolTable

    void SymbolTabledeclare(SymbolTable* table, chem::string_view* name, ASTNode* node);

    void SymbolTabledeclare_no_shadow(SymbolTable* table, chem::string_view* name, ASTNode* node);

    void SymbolTablescope_start(SymbolTable* table);

    unsigned long SymbolTablescope_start_index(SymbolTable* table);

    void SymbolTablescope_end(SymbolTable* table);

    ASTNode* SymbolTableresolve(SymbolTable* table, chem::string_view* name);

}