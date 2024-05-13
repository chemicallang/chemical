// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 11/03/2024.
//

#include "FoldingRangeAnalyzer.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include "cst/base/CompoundCSTToken.h"
#include "cst/utils/CSTUtils.h"

#define DEBUG_FOLDING_RANGE false

void FoldingRangeAnalyzer::folding_range(LexToken* start, LexToken* end, bool comment) {
    ranges.push_back(FoldingRange{
            static_cast<int>(start->position.line),
            static_cast<int>(end->position.line),
            static_cast<int>(start->position.character),
            static_cast<int>(end->position.character),
            (comment ? "comment" : "region")
    });
}

void FoldingRangeAnalyzer::visitStructDef(CompoundCSTToken *structDef) {
    auto has_override = is_char_op(structDef->tokens[3].get(), ':');
    auto start = has_override ? 4 : 2;
    unsigned i = has_override ? 5 : 3; // positioned at first node or '}'
    while (!is_char_op(structDef->tokens[i].get(), '}')) {
        i++;
    }
    folding_range(structDef->tokens[start]->start_token(), structDef->tokens[i]->start_token());
};

void FoldingRangeAnalyzer::visitIf(CompoundCSTToken *ifCst) {
    analyze(ifCst->tokens);
}

void FoldingRangeAnalyzer::visitForLoop(CompoundCSTToken *forLoop) {
    forLoop->tokens[8]->accept(this);
};

void FoldingRangeAnalyzer::visitWhile(CompoundCSTToken *whileCst) {
    whileCst->tokens[4]->accept(this);
};

void FoldingRangeAnalyzer::visitDoWhile(CompoundCSTToken *doWhileCst) {
    doWhileCst->tokens[1]->accept(this);
};

void FoldingRangeAnalyzer::visitFunction(CompoundCSTToken *function) {
    function->tokens[function->tokens.size() - 1]->accept(this);
};

void FoldingRangeAnalyzer::visitLambda(CompoundCSTToken *cst) {

};

void FoldingRangeAnalyzer::visit(MultilineCommentToken *token) {

};

void FoldingRangeAnalyzer::visitBody(CompoundCSTToken *bodyCst) {
    folding_range(bodyCst->start_token(), bodyCst->end_token());
    ::visit(this, bodyCst->tokens);
}

void FoldingRangeAnalyzer::analyze(std::vector<std::unique_ptr<CSTToken>> &tokens) {
    ::visit(this, tokens);
#if defined DEBUG_FOLDING_RANGE && DEBUG_FOLDING_RANGE
        for (const auto &range: ranges) {
            std::cout << std::to_string(range.startLine) << ':' << std::to_string(range.startCharacter) << '-'
                      << std::to_string(range.endLine) << ':' << std::to_string(range.endCharacter) << '-'
                      << range.kind << std::endl;
        }
#endif
}