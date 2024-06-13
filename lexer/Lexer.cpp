// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#include "Lexer.h"
#include "lexer/model/tokens/KeywordToken.h"
#include "ast/utils/GlobalFunctions.h"
#include "ast/types/ReferencedType.h"
#include "lexer/model/tokens/BoolToken.h"
#include "lexer/model/tokens/NullToken.h"
#include "lexer/model/CompilerBinderTCC.h"

Lexer::Lexer(SourceProvider &provider, std::string path) : provider(provider), path(std::move(path)), cbi() {

}

void Lexer::init_complete(const std::string& exe_path) {
    init_annotation_modifiers();
    init_value_creators();
    init_macro_lexers();
    init_cbi(exe_path);
}

void Lexer::init_cbi(const std::string& exe_path) {
    binder = std::make_unique<CompilerBinderTCC>(this, exe_path);
    init_lexer_cbi(&cbi, this, &provider_cbi);
}

std::string annotation_str_param(unsigned index, CSTToken* token);

void Lexer::init_annotation_modifiers() {
    annotation_modifiers["cbi:global"] = [](Lexer *lexer, CSTToken* token) -> void {
        if(!lexer->isCBIEnabled) return;
        lexer->isCBICollecting = true;
        lexer->isCBICollectingGlobal = true;
        lexer->current_cbi = annotation_str_param(0, token);;
    };
    annotation_modifiers["cbi:create"] = [](Lexer *lexer, CSTToken* token) -> void {
        if(!lexer->isCBIEnabled) return;
        auto n = annotation_str_param(0,token);
        if(n.empty()) {
            lexer->error("cbi:create called with invalid parameters : " + n);
            return;
        }
        lexer->binder->create_cbi(n);
    };
    annotation_modifiers["cbi:import"] = [](Lexer *lexer, CSTToken* token) -> void {
        if(!lexer->isCBIEnabled) return;
        auto a = annotation_str_param(0,token);
        auto b = annotation_str_param(1,token);
        if(a.empty() || b.empty()) {
            lexer->error("cbi:import called with invalid parameters : " + a + " , " + b);
            return;
        }
        lexer->binder->import_container(a, b);
    };
    annotation_modifiers["cbi:to"] = [](Lexer *lexer, CSTToken* token) -> void {
        if(!lexer->isCBIEnabled) return;
        lexer->isCBICollecting = true;
        lexer->current_cbi = annotation_str_param(0, token);
    };
    annotation_modifiers["cbi:compile"] = [](Lexer *lexer, CSTToken* token) -> void {
        if(!lexer->isCBIEnabled) return;
        auto a = annotation_str_param(0,token);
        if(a.empty()) {
            lexer->error("cbi:compiler called with invalid parameters : " + a);
            return;
        }
        lexer->binder->compile(a);
    };
}

void Lexer::init_value_creators() {
    value_creators["null"] = [](Lexer *lexer) -> void {
        lexer->tokens.emplace_back(std::make_unique<NullToken>(lexer->backPosition(4), "null"));
    };
    value_creators["true"] = [](Lexer *lexer) -> void {
        lexer->tokens.emplace_back(std::make_unique<BoolToken>(lexer->backPosition(4), "true"));
    };
    value_creators["false"] = [](Lexer *lexer) -> void {
        lexer->tokens.emplace_back(std::make_unique<BoolToken>(lexer->backPosition(5), "false"));
    };
}

void Lexer::init_macro_lexers() {
    macro_lexers["eval"] = [](Lexer *lexer) -> void {
        lexer->lexExpressionTokens(false, false);
    };
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