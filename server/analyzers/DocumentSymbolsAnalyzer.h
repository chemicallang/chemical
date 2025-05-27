// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "lsp/types.h"
#include "preprocess/visitors/NonRecursiveVisitor.h"

#include "ast/base/ASTNode.h"

class LexResult;

class LocationManager;

class DocumentSymbolsAnalyzer : public NonRecursiveVisitor<DocumentSymbolsAnalyzer> {
public:

    /**
     * the manager used to decode locations
     */
    LocationManager& loc_man;

    /**
     * the symbols collected
     */
    std::vector<lsp::DocumentSymbol> symbols;

    /**
     * the constructor
     */
    DocumentSymbolsAnalyzer(LocationManager& loc_man) : loc_man(loc_man) {

    }

    /**
     * will create a range for the given location
     */
    lsp::Range range(SourceLocation location);

    /**
     * put a symbol with given name and range
     */
    void put(const chem::string_view& name, lsp::SymbolKind kind, lsp::Range range);

    /**
     * put a symbol with given name and location
     */
    void put(const chem::string_view& name, lsp::SymbolKind kind, SourceLocation location);

    /**
     * put a symbol with name, kind and range
     */
    void put(const chem::string_view& name, lsp::SymbolKind kind, lsp::Range range, lsp::Range selRange);

    /**
     * will analyzer the given top level nodes, to put symbols for the
     * current document
     */
    inline void analyze(std::vector<ASTNode*>& nodes) {
        for(const auto node : nodes) {
            visit(node);
        }
    }

    //---------------------------
    //------- Visitors-----------
    //---------------------------

    void VisitFunctionDecl(FunctionDeclaration* node);

    void VisitStructDecl(StructDefinition* node);

    void VisitUnionDecl(UnionDef* node);

    void VisitVariantDecl(VariantDefinition* node);

    void VisitInterfaceDecl(InterfaceDefinition* node);

    void VisitTypealiasStmt(TypealiasStatement* node);

    void VisitEnumDecl(EnumDeclaration* node);

    void VisitVarInitStmt(VarInitStatement* node);

};