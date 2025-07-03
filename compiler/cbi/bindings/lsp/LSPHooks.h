// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <cstdint>

class SemanticTokensAnalyzer;
class FoldingRangeAnalyzer;
class Position;
struct Token;

extern "C" {

    void SemanticTokensAnalyzerputAuto(SemanticTokensAnalyzer* analyzer, Token* token);

    void SemanticTokensAnalyzerput(
            SemanticTokensAnalyzer* analyzer,
            uint32_t lineNumber,
            uint32_t lineCharNumber,
            uint32_t length,
            uint32_t tokenType,
            uint32_t tokenModifiers
    );

    void SemanticTokensAnalyzerputToken(
            SemanticTokensAnalyzer* analyzer,
            Token* token,
            uint32_t tokenType,
            uint32_t tokenModifiers
    );

    void FoldingRangeAnalyzerput(
        FoldingRangeAnalyzer* analyzer,
        Position* start,
        Position* end,
        bool comment
    );

    void FoldingRangeAnalyzerstackPush(FoldingRangeAnalyzer* analyzer, Position* position);

    bool FoldingRangeAnalyzerstackEmpty(FoldingRangeAnalyzer* analyzer);

    void FoldingRangeAnalyzerstackPop(Position* outPos, FoldingRangeAnalyzer* analyzer);

}