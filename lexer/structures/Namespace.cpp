// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"
#include "cst/structures/NamespaceCST.h"

bool Lexer::lexNamespaceTokens() {
    if(lexKeywordToken("namespace")) {
        auto start = tokens.size() - 1;
        lexWhitespaceToken();
        if(!lexIdentifierToken()) {
            error("expected identifier for namespace name");
            return true;
        }
        auto result = lexBraceBlock("namespace", [](Lexer* lexer){
            lexer->lexTopLevelMultipleStatementsTokens(true);
        });
        if(result) {
            compound_from<NamespaceCST>(start, LexTokenType::CompNamespace);
        }
        return result;
    }
    return false;
}