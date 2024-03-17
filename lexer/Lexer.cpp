// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#include "Lexer.h"
#include "lexer/model/tokens/KeywordToken.h"

void Lexer::lexTopLevelMultipleStatementsTokens() {

    // lex whitespace and new lines to reach a statement
    // lex a statement and then optional whitespace, lex semicolon

    while (true) {
        lexWhitespaceAndNewLines();
        if (!lexStatementTokens()) {
            if (provider.eof() || provider.peek() == -1) {
                break;
            } else {
                // skip to new line
                auto last_readable_token = tokens.size() - 1;
                while (!lexNewLineChars() && !(provider.eof() || provider.peek() == -1)) {
                    provider.readCharacter();
                }
                error(last_readable_token, "Skipped due to invalid syntax before it");
                continue;
            }
        }
        lexWhitespaceToken();
        lexOperatorToken(';');
    }
}


void Lexer::lex() {
    lexTopLevelMultipleStatementsTokens();
    tokens.shrink_to_fit();
}

TokenPosition Lexer::position() {
    return {provider.getLineNumber(), provider.getLineCharNumber(), provider.position()};
}

TokenPosition Lexer::backPosition(unsigned int back) {
    return {provider.getLineNumber(), provider.getLineCharNumber() - back, provider.position() - back};
}

void Lexer::error(unsigned int position, const std::string &message) {
    auto token = tokens[position].get();
    auto &pos = token->position;
    auto len = token->length();
    errors.emplace_back(
            Range{
                    {pos.lineNumber,           pos.lineCharNumber + len},
                    {provider.getLineNumber(), provider.getLineCharNumber()}
            },
            DiagSeverity::Error,
            std::nullopt,
            message
    );
}