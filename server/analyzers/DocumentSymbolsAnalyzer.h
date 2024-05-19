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
    void analyze(std::vector<std::unique_ptr<CSTToken>>& tokens);

    //---------------------------
    //------- Visitors-----------
    //---------------------------

    void visitCompoundCommon(CompoundCSTToken *compound) override;

    void visitFunction(CompoundCSTToken *function) override;

    void visitStructDef(CompoundCSTToken *structDef) override;

    void visitInterface(CompoundCSTToken *interface) override;

    void visitTypealias(CompoundCSTToken *alias) override;

    void visitEnumDecl(CompoundCSTToken *enumDecl) override;

    void visitVarInit(CompoundCSTToken *varInit) override;

};