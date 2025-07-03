// Copyright (c) Chemical Language Foundation 2025.

#include "LSPHooks.h"

#include "server/analyzers/SemanticTokensAnalyzer.h"
#include "server/analyzers/FoldingRangeAnalyzer.h"

void SemanticTokensAnalyzerputAuto(SemanticTokensAnalyzer* analyzer, Token* token) {
    analyzer->put_auto(token);
}

Token** SemanticTokensAnalyzergetCurrentTokenPtr(SemanticTokensAnalyzer* analyzer) {
    return &analyzer->current_token;
}

Token* SemanticTokensAnalyzergetEndToken(SemanticTokensAnalyzer* analyzer) {
    return analyzer->end_token;
}

void SemanticTokensAnalyzerput(
        SemanticTokensAnalyzer* analyzer,
        uint32_t lineNumber,
        uint32_t lineCharNumber,
        uint32_t length,
        uint32_t tokenType,
        uint32_t tokenModifiers
) {
    analyzer->put(lineNumber, lineCharNumber, length, tokenType, tokenModifiers);
}

void SemanticTokensAnalyzerputToken(
        SemanticTokensAnalyzer* analyzer,
        Token* token,
        uint32_t tokenType,
        uint32_t tokenModifiers
) {
    analyzer->put(token, tokenType, tokenModifiers);
}

void FoldingRangeAnalyzerput(
        FoldingRangeAnalyzer* analyzer,
        Position* start,
        Position* end,
        bool comment
) {
    analyzer->folding_range(*start, *end, comment);
}

void FoldingRangeAnalyzerstackPush(FoldingRangeAnalyzer* analyzer, Position* position) {
    analyzer->bracesStack.emplace_back(*position);
}

bool FoldingRangeAnalyzerstackEmpty(FoldingRangeAnalyzer* analyzer) {
    return analyzer->bracesStack.empty();
}

void FoldingRangeAnalyzerstackPop(Position* outPos, FoldingRangeAnalyzer* analyzer) {
    if(analyzer->bracesStack.empty()) {
        outPos->line = 0;
        outPos->character = 0;
        return;
    }
    auto& back = analyzer->bracesStack.back();
    *outPos = back;
    analyzer->bracesStack.pop_back();
}