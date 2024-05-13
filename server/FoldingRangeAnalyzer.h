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
    void analyze(std::vector<std::unique_ptr<CSTToken>>& tokens);

    /**
     * will add a folding range from start to end token
     */
    void folding_range(LexToken* start, LexToken* end, bool comment = false);

    /**
     * will add a folding range for this CSTToken
     */
    void folding_range(CSTToken* inside, bool comment = false) {
        folding_range(inside->start_token(), inside->end_token(), comment);
    }

    // Visitor functions

    void visitVarInit(CompoundCSTToken *varInit) override;

    void visitReturn(CompoundCSTToken *returnCst) override;

    void visitFunctionCall(CompoundCSTToken *call) override;

    void visitAssignment(CompoundCSTToken *assignment) override;

    void visitAccessChain(AccessChainCST *accessChain) override;

    void visitEnumDecl(CompoundCSTToken *enumDecl) override;

    void visitInterface(CompoundCSTToken *interface) override;

    void visitStructDef(CompoundCSTToken *structDef) override;

    void visitImpl(CompoundCSTToken *impl) override;

    void visitIf(CompoundCSTToken *ifCst) override;

    void visitForLoop(CompoundCSTToken *forLoop) override;

    void visitWhile(CompoundCSTToken *whileCst) override;

    void visitDoWhile(CompoundCSTToken *doWhileCst) override;

    void visitFunction(CompoundCSTToken *function) override;

    void visitLambda(CompoundCSTToken *cst) override;

    void visit(MultilineCommentToken *token) override;

    void visitBody(CompoundCSTToken *bodyCst) override;


};