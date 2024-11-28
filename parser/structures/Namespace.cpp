// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"

bool Parser::lexNamespaceTokens(unsigned start) {
    if(lexWSKeywordToken(TokenType::NamespaceKw)) {
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