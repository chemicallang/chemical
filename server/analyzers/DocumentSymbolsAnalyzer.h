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

    void visitCompoundCommon(CSTToken* compound) final;

    void visitFunction(CSTToken* function) final;

    void visitStructDef(CSTToken* structDef) final;

    void visitInterface(CSTToken* interface) final;

    void visitTypealias(CSTToken* alias) final;

    void visitEnumDecl(CSTToken* enumDecl) final;

    void visitVarInit(CSTToken* varInit) final;

};