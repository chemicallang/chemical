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

    void visit(StructDefCST *structDef) override;

    void visit(ForLoopCST *forLoop) override;

    void visit(WhileCST *whileCst) override;

    void visit(DoWhileCST *doWhileCst) override;

    void visit(FunctionCST *function) override;

    void visit(LambdaCST *cst) override;

    void visit(MultilineCommentToken *token) override;

    void visit(BodyCST *bodyCst) override;


};