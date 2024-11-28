// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#include "Parser.h"
#include "ast/types/LinkedType.h"
#include "cst/LocationManager.h"

Parser::Parser(
        unsigned int file_id,
        std::string_view file_path,
        Token* start_token,
        LocationManager& loc_man,
        ASTAllocator& global_allocator,
        ASTAllocator& mod_allocator,
        CompilerBinder* binder
) : file_id(file_id), stored_file_path(file_path), token(start_token), beginning_token(start_token), loc_man(loc_man), global_allocator(global_allocator), mod_allocator(mod_allocator), binder(binder), unit() {
    unit.init();
}

std::string_view Parser::file_path() {
    return stored_file_path;
}

uint64_t Parser::loc(Position& start, Position& end) {
    return loc_man.addLocation(file_id, start.line, start.character, end.line, end.character);
}

uint64_t Parser::loc_single(Token* t) {
    auto& pos = t->position;
    return loc_man.addLocation(file_id, pos.line, pos.character, pos.line, pos.character + token->value.size());
}

void Parser::lexTopLevelMultipleImportStatements() {
    while (true) {
        lexWhitespaceAndNewLines();
        if (!lexImportStatement() && !lexSingleLineCommentTokens() && !lexMultiLineCommentTokens()) {
            break;
        }
        lexWhitespaceToken();
        lexOperatorToken(TokenType::SemiColonSym);
    }
}

void Parser::lexTopLevelMultipleStatementsTokens(bool break_at_no_stmt) {

    // lex whitespace and new lines to reach a statement
    // lex a statement and then optional whitespace, lex semicolon

    while (true) {
        lexWhitespaceAndNewLines();
        if (!lexTopLevelStatementTokens()) {
            if (break_at_no_stmt || token->type == TokenType::EndOfFile) {
                break;
            } else {
                // skip the current token
                diagnostic(token->position, "skipped due to invalid syntax before it", DiagSeverity::Error);
                token++;
                continue;
            }
        }
        lexWhitespaceToken();
        lexOperatorToken(TokenType::SemiColonSym);
    }
}


void Parser::lex() {
    lexTopLevelMultipleImportStatements();
    lexTopLevelMultipleStatementsTokens();
}

void Parser::reset() {
    unit.reset();
    token = beginning_token;
}