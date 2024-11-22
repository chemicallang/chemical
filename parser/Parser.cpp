// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#include "Parser.h"
#include "ast/types/LinkedType.h"

Parser::Parser(
        std::string file_path,
        SourceProvider &provider,
        CompilerBinder* binder
) : file_path(std::move(file_path)), provider(provider), binder(binder), unit() {
    unit.init();
}

void Parser::lexTopLevelMultipleImportStatements() {
    while (true) {
        lexWhitespaceAndNewLines();
        if (!lexImportStatement() && !lexSingleLineCommentTokens() && !lexMultiLineCommentTokens()) {
            break;
        }
        lexWhitespaceToken();
        lexOperatorToken(';');
    }
}

void Parser::lexTopLevelMultipleStatementsTokens(bool break_at_no_stmt) {

    // lex whitespace and new lines to reach a statement
    // lex a statement and then optional whitespace, lex semicolon

    while (true) {
        lexWhitespaceAndNewLines();
        if (!lexTopLevelStatementTokens()) {
            if (break_at_no_stmt || provider.eof() || provider.peek() == -1) {
                break;
            } else {
                // skip to new line
                auto from_position = provider.getStreamPosition();
                while (!lexNewLineChars() && !(provider.eof() || provider.peek() == -1)) {
                    provider.readCharacter();
                }
                diagnostic({ from_position.line, from_position.character }, "skipped due to invalid syntax before it", DiagSeverity::Error);
                continue;
            }
        }
        lexWhitespaceToken();
        lexOperatorToken(';');
    }
}


void Parser::lex() {
    lexTopLevelMultipleImportStatements();
    lexTopLevelMultipleStatementsTokens();
}

void Parser::reset() {
    unit.reset();
    provider.reset();
}

void Parser::diagnostic(Position start, const std::string &message, DiagSeverity severity) {
    if(severity == DiagSeverity::Error) {
        has_errors = true;
    }
    diagnostics.emplace_back(
            Range{
                    start,
                    {provider.getLineNumber(), provider.getLineCharNumber()}
            },
            severity,
            std::nullopt,
            message
    );
}