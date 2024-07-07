// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#include "Lexer.h"
#include "lexer/model/tokens/KeywordToken.h"
#include "ast/types/ReferencedType.h"
#include "lexer/model/CompilerBinderTCC.h"

Lexer::Lexer(SourceProvider &provider, std::string path) : provider(provider), path(std::move(path)), cbi() {

}

void Lexer::init_complete(const std::string& exe_path) {
    init_cbi(exe_path);
}

void Lexer::init_cbi(const std::string& exe_path) {
    binder = std::make_unique<CompilerBinderTCC>(this, exe_path);
    init_lexer_cbi(&cbi, this, &provider_cbi);
}

void Lexer::lexTopLevelMultipleImportStatements() {
    while (true) {
        lexWhitespaceAndNewLines();
        if (!lexImportStatement()) {
            break;
        }
        lexWhitespaceToken();
        lexOperatorToken(';');
    }
}

void Lexer::lexTopLevelMultipleStatementsTokens(bool break_at_no_stmt) {

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


void Lexer::lex() {
    lexTopLevelMultipleImportStatements();
    lexTopLevelMultipleStatementsTokens();
    tokens.shrink_to_fit();
}

void Lexer::switch_path(const std::string& new_path) {
    path = new_path;
}

void Lexer::reset() {
    tokens.clear();
    provider.reset();
}

void Lexer::diagnostic(Position start, const std::string &message, DiagSeverity severity) {
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