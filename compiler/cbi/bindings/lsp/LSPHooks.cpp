// Copyright (c) Chemical Language Foundation 2025.

#include "LSPHooks.h"

#include "server/analyzers/SemanticTokensAnalyzer.h"

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
