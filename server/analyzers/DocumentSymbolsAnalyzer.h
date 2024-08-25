// Copyright (c) Qinetik 2024.

#pragma once

#include "LibLsp/lsp/symbol.h"
#include "cst/base/CSTVisitor.h"

class LexResult;

class DocumentSymbolsAnalyzer : public CSTVisitor {
public:

    /**
     * the symbols collected
     */
    std::vector<lsDocumentSymbol> symbols;

    /**
     * put a symbol with name, kind and range
     */
    void put(const std::string& name, lsSymbolKind kind, lsRange range, lsRange selRange);

    /**
     * will analyze the given lex result
     */
    void analyze(std::vector<CSTToken*>& tokens);

    //---------------------------
    //------- Visitors-----------
    //---------------------------

    void visitCompoundCommon(CSTToken* compound) override;

    void visitFunction(CSTToken* function) override;

    void visitStructDef(CSTToken* structDef) override;

    void visitInterface(CSTToken* interface) override;

    void visitTypealias(CSTToken* alias) override;

    void visitEnumDecl(CSTToken* enumDecl) override;

    void visitVarInit(CSTToken* varInit) override;

};