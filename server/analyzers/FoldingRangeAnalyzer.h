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

    void visitVarInit(CSTToken* varInit) final;

    void visitReturn(CSTToken* returnCst) final;

    void visitFunctionCall(CSTToken* call) final;

    void visitAssignment(CSTToken* assignment) final;

    void visitAccessChain(CSTToken *accessChain) final;

    void visitSwitch(CSTToken* switchCst) final;

    void visitEnumDecl(CSTToken* enumDecl) final;

    void visitInterface(CSTToken* interface) final;

    void visitStructDef(CSTToken* structDef) final;

    void visitImpl(CSTToken* impl) final;

    void visitIf(CSTToken* ifCst) final;

    void visitForLoop(CSTToken* forLoop) final;

    void visitWhile(CSTToken* whileCst) final;

    void visitDoWhile(CSTToken* doWhileCst) final;

    void visitFunction(CSTToken* function) final;

    void visitLambda(CSTToken* cst) final;

    void visitMultilineComment(CSTToken *token) final;

    void visitBody(CSTToken* bodyCst) final;

    void visitStructValue(CSTToken* structValueCst) final;

    void visitArrayValue(CSTToken* arrayValue) final;


};