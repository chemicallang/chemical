// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 11/03/2024.
//

#pragma once

#include "LibLsp/lsp/textDocument/foldingRange.h"
#include "cst/base/CSTVisitor.h"
#include "cst/base/CSTToken.h"

class FoldingRangeAnalyzer : public CSTVisitor {
public:

    /**
     * constructor
     * @param tokens
     */
    FoldingRangeAnalyzer() {

    }

    /**
     * all the folding ranges found
     */
    std::vector<FoldingRange> ranges;

    /**
     * The function that analyzes
     */
    void analyze(std::vector<CSTToken*>& tokens);

    /**
     * will add a folding range from start to end token
     */
    void folding_range(CSTToken* start, CSTToken* end, bool comment = false);

    /**
     * will add a folding range for this CSTToken
     */
    void folding_range(CSTToken* inside, bool comment = false) {
        folding_range(inside->start_token(), inside->end_token(), comment);
    }

    // Visitor functions

    void visitVarInit(CSTToken* varInit) override;

    void visitReturn(CSTToken* returnCst) override;

    void visitFunctionCall(CSTToken* call) override;

    void visitAssignment(CSTToken* assignment) override;

    void visitAccessChain(AccessChainCST *accessChain) override;

    void visitSwitch(CSTToken* switchCst) override;

    void visitEnumDecl(CSTToken* enumDecl) override;

    void visitInterface(CSTToken* interface) override;

    void visitStructDef(CSTToken* structDef) override;

    void visitImpl(CSTToken* impl) override;

    void visitIf(CSTToken* ifCst) override;

    void visitForLoop(CSTToken* forLoop) override;

    void visitWhile(CSTToken* whileCst) override;

    void visitDoWhile(CSTToken* doWhileCst) override;

    void visitFunction(CSTToken* function) override;

    void visitLambda(CSTToken* cst) override;

    void visitMultilineComment(CSTToken *token) override;

    void visitBody(CSTToken* bodyCst) override;

    void visitStructValue(CSTToken* structValueCst) override;

    void visitArrayValue(CSTToken* arrayValue) override;


};