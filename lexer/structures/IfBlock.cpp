// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexIfExprAndBlock(unsigned start, bool is_value, bool lex_value_node, bool top_level) {

    void (Lexer::*malformed)(unsigned int, const std::string&);
    malformed = is_value ? &Lexer::mal_value : &Lexer::mal_node;

    if (!lexOperatorToken('(')) {
        (this->*malformed)(start, "expected a starting parenthesis ( when lexing a if block");
        return false;
    }

    if (!lexExpressionTokens()) {
        (this->*malformed)(start, "expected a conditional expression when lexing a if block");
        return false;
    }

    if (!lexOperatorToken(')')) {
        (this->*malformed)(start, "expected a ending parenthesis ) when lexing a if block");
        return false;
    }

    if(top_level) {
        if (!lexTopLevelBraceBlock("else")) {
            (this->*malformed)(start, "expected a brace block after the else while lexing an if statement");
            return false;
        }
    } else {
        if (!lexBraceBlockOrSingleStmt("if", is_value, lex_value_node)) {
            (this->*malformed)(start, "expected a brace block when lexing a brace block");
            return false;
        }
    }

    return true;

}

bool Lexer::lexIfBlockTokens(bool is_value, bool lex_value_node, bool top_level) {

    if(!lexWSKeywordToken("if", '(')) {
        return false;
    }

    auto start = tokens_size() - 1;

    if(!lexIfExprAndBlock(start, is_value, lex_value_node, top_level)) {
        return true;
    }

    // lex whitespace
    lexWhitespaceAndNewLines();

    // keep lexing else if blocks until last else appears
    while (lexWSKeywordToken("else", '{')) {
        lexWhitespaceAndNewLines();
        if(lexWSKeywordToken("if", '(')) {
            if(!lexIfExprAndBlock(start, is_value, lex_value_node, top_level)) {
                return true;
            }
            lexWhitespaceToken();
        } else {
            if(top_level) {
                if (!lexTopLevelBraceBlock("else")) {
                    mal_node(start,"expected a brace block after the else while lexing an if statement");
                    return true;
                }
            } else {
                if (!lexBraceBlockOrSingleStmt("else", is_value, lex_value_node)) {
                    mal_node(start,"expected a brace block after the else while lexing an if statement");
                    return true;
                }
            }
            compound_from(start, is_value ? LexTokenType::CompIfValue :  LexTokenType::CompIf);
            return true;
        }
    }

    compound_from(start, is_value ? LexTokenType::CompIfValue :  LexTokenType::CompIf);

    return true;

}