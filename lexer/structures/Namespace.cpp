// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"

bool Lexer::lexNamespaceTokens(unsigned start) {
    if(lexWSKeywordToken("namespace")) {
        if(!lexIdentifierToken()) {
            error("expected identifier for namespace name");
            return true;
        }
        auto result = lexTopLevelBraceBlock("namespace");
        if(result) {
            compound_from(start, LexTokenType::CompNamespace);
        }
        return result;
    }
    return false;
}