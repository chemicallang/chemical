// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"

void Parser::lexNestedLevelMultipleStatementsTokens(bool is_value, bool lex_value_node) {

    // lex whitespace and new lines to reach a statement
    // lex a statement and then optional whitespace, lex semicolon

    while(true) {
        lexWhitespaceAndNewLines();
        if(!lexNestedLevelStatementTokens(is_value, lex_value_node))  {
            break;
        }
        lexWhitespaceToken();
        lexOperatorToken(TokenType::SemiColonSym);
    }
}

void Parser::lexMultipleStatementsTokens() {

    // lex whitespace and new lines to reach a statement
    // lex a statement and then optional whitespace, lex semicolon

    while(true) {
        lexWhitespaceAndNewLines();
        if(!lexStatementTokens())  {
            break;
        }
        lexWhitespaceToken();
        lexOperatorToken(TokenType::SemiColonSym);
    }
}

bool Parser::lexBraceBlock(const std::string &forThing, void(*nested_lexer)(Parser*)) {

    // whitespace and new lines
    lexWhitespaceAndNewLines();

    unsigned start = tokens_size();

    // starting brace
    if (!lexOperatorToken(TokenType::LBrace)) {
        return false;
    }

    // multiple statements
    auto prevImportState = isLexImportStatement;
    isLexImportStatement = false;
    nested_lexer(this);
    isLexImportStatement = prevImportState;

    // ending brace
    if (!lexOperatorToken(TokenType::RBrace)) {
        error("expected a closing brace '}' for [" + forThing + "]");
        return true;
    }

    compound_from(start, LexTokenType::CompBody);

    return true;

}

bool Parser::lexBraceBlock(const std::string &forThing) {

    // whitespace and new lines
    lexWhitespaceAndNewLines();

    unsigned start = tokens_size();

    // starting brace
    if (!lexOperatorToken(TokenType::LBrace)) {
        return false;
    }

    // multiple statements
    auto prevImportState = isLexImportStatement;
    isLexImportStatement = false;
    lexNestedLevelMultipleStatementsTokens();
    isLexImportStatement = prevImportState;

    // ending brace
    if (!lexOperatorToken(TokenType::RBrace)) {
        error("expected a closing brace '}' for [" + forThing + "]");
        return true;
    }

    compound_from(start, LexTokenType::CompBody);

    return true;
}

bool Parser::lexBraceBlockOrSingleStmt(const std::string &forThing, bool is_value, bool lex_value_node) {

    // whitespace and new lines
    lexWhitespaceAndNewLines();

    // starting brace
    if (!lexOperatorToken(TokenType::LBrace)) {
        if(lexNestedLevelStatementTokens(is_value, lex_value_node) || (lex_value_node && lexValueNode())) {
            lexWhitespaceAndNewLines();
            if (lexOperatorToken(TokenType::SemiColonSym)) {
                lexWhitespaceAndNewLines();
            }
            return true;
        };
        return false;
    }

    unsigned start = tokens_size() - 1;

    // multiple statements
    auto prevImportState = isLexImportStatement;
    isLexImportStatement = false;
    lexNestedLevelMultipleStatementsTokens(false, lex_value_node);
    isLexImportStatement = prevImportState;

    // ending brace
    if (!lexOperatorToken(TokenType::RBrace)) {
        error("expected a closing brace '}' for [" + forThing + "]");
        return true;
    }

    compound_from(start, LexTokenType::CompBody);

    return true;
}