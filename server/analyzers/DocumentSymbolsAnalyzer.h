// Copyright (c) Qinetik 2024.

#pragma once

#include "LibLsp/lsp/symbol.h"
#include "ast/base/Visitor.h"

#include "ast/base/ASTNode.h"

class LexResult;

class LocationManager;

class DocumentSymbolsAnalyzer : public Visitor {
public:

    /**
     * the manager used to decode locations
     */
    LocationManager& loc_man;

    /**
     * the symbols collected
     */
    std::vector<lsDocumentSymbol> symbols;

    /**
     * the constructor
     */
    DocumentSymbolsAnalyzer(LocationManager& loc_man) : loc_man(loc_man) {

    }

    /**
     * will create a range for the given location
     */
    lsRange range(SourceLocation location);

    /**
     * put a symbol with name, kind and range
     */
    void put(const std::string& name, lsSymbolKind kind, lsRange range, lsRange selRange);

    /**
     * will analyzer the given top level nodes, to put symbols for the
     * current document
     */
    inline void analyze(std::vector<ASTNode*>& nodes) {
        for(auto node : nodes) {
            node->accept(this);
        }
    }

    //---------------------------
    //------- Visitors-----------
    //---------------------------

    void visit(FunctionDeclaration *decl) override;

    void visit(StructDefinition *def) override;

    void visit(InterfaceDefinition *def) override;

    void visit(TypealiasStatement *def) override;

    void visit(EnumDeclaration *def) override;

    void visit(VarInitStatement *init) override;

};