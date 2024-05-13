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
    folding_range(structDef->tokens[start]->start_token(), structDef->tokens[structDef->tokens.size() - 1]->start_token());
};

void FoldingRangeAnalyzer::visitVarInit(CompoundCSTToken *varInit) {
    varInit->tokens[varInit->tokens.size() - 1]->accept(this);
}

void FoldingRangeAnalyzer::visitIf(CompoundCSTToken *ifCst) {
    ::visit(this, ifCst->tokens);
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

void FoldingRangeAnalyzer::visitEnumDecl(CompoundCSTToken *enumDecl) {
    folding_range(enumDecl->tokens[2]->start_token(), enumDecl->tokens[enumDecl->tokens.size() - 1]->end_token());
}

void FoldingRangeAnalyzer::visitLambda(CompoundCSTToken *cst) {
    cst->tokens[cst->tokens.size() - 1]->accept(this);
};

void FoldingRangeAnalyzer::visitInterface(CompoundCSTToken *interface) {
    folding_range(interface->tokens[2]->start_token(), interface->tokens[interface->tokens.size() - 1]->end_token());
}

void FoldingRangeAnalyzer::visitImpl(CompoundCSTToken *impl) {
    bool no_for = is_char_op(impl->tokens[2].get(), '{');
    folding_range(impl->tokens[no_for ? 2 : 4]->start_token(), impl->tokens[impl->tokens.size() - 1]->end_token());
}

void FoldingRangeAnalyzer::visit(MultilineCommentToken *token) {
    // TODO represent multi line token with at least 3 tokens
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