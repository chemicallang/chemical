// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#include "Lexer.h"
#include "Impls.h"
#include "lexer/model/tokens/KeywordToken.h"
#include "ast/utils/GlobalFunctions.h"
#include "ast/types/ReferencedType.h"
#include "lexer/model/tokens/BoolToken.h"
#include "lexer/model/tokens/NullToken.h"

Lexer::Lexer(SourceProvider &provider, std::string path) : provider(provider), path(std::move(path)), interpret_scope(
        GlobalInterpretScope(nullptr, nullptr, nullptr, path)
) {
#ifdef DEBUG
    interpret_scope.warn_no_nodes = false;
#endif
    define_all(interpret_scope);
    init_annotation_modifiers();
    init_value_creators();
}

void Lexer::init_annotation_modifiers() {
    annotation_modifiers["lexer"] = [](Lexer *lexer) -> void { lexer->isLexCompTimeLexer = true; };
    annotation_modifiers["scope:lexer"] = [](Lexer *lexer) -> void { lexer->isLexerScoped = true; };
}

void Lexer::init_value_creators() {
    value_creators["null"] = [](Lexer *lexer) -> void {
        lexer->tokens.emplace_back(std::make_unique<NullToken>(lexer->backPosition(4)));
    };
    value_creators["true"] = [](Lexer *lexer) -> void {
        lexer->tokens.emplace_back(std::make_unique<BoolToken>(lexer->backPosition(4), "true"));
    };
    value_creators["false"] = [](Lexer *lexer) -> void {
        lexer->tokens.emplace_back(std::make_unique<BoolToken>(lexer->backPosition(5), "false"));
    };
}

void Lexer::lexTopLevelMultipleImportStatements() {
    // lex whitespace and new lines to reach a statement
    // lex a statement and then optional whitespace, lex semicolon
    while (true) {
        lexWhitespaceAndNewLines();
        if (!lexImportStatement()) {
            break;
        }
        lexWhitespaceToken();
        lexOperatorToken(';');
    }
}

void Lexer::lexTopLevelMultipleStatementsTokens() {

    // lex whitespace and new lines to reach a statement
    // lex a statement and then optional whitespace, lex semicolon

    while (true) {
        lexWhitespaceAndNewLines();
        if (!lexTopLevelStatementTokens()) {
            if (provider.eof() || provider.peek() == -1) {
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
    lexTopLevelMultipleStatementsTokens();
    tokens.shrink_to_fit();
}

void Lexer::switch_path(const std::string& new_path) {
    path = new_path;
}

void Lexer::diagnostic(Position start, const std::string &message, DiagSeverity severity) {
    if(severity == DiagSeverity::Error) {
        has_errors = true;
    }
    errors.emplace_back(
            Range{
                    start,
                    {provider.getLineNumber(), provider.getLineCharNumber()}
            },
            severity,
            std::nullopt,
            message
    );
}