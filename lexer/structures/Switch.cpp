// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"

bool Lexer::lexSwitchStatementBlock(bool is_value, bool lex_value_node) {
    if (lexWSKeywordToken("switch", '(')) {
        auto start = tokens_size() - 1;

        void (Lexer::*malformed)(unsigned int, const std::string&);
        malformed = is_value ? &Lexer::mal_value : &Lexer::mal_node;

        if (lexOperatorToken('(')) {
            if (!lexExpressionTokens()) {
                (this->*malformed)(start, "expected an expression tokens in switch statement");
                return true;
            }
            if (!lexOperatorToken(')')) {
                (this->*malformed)(start, "expected ')' in switch statement");
                return true;
            }
        } else {
            (this->*malformed)(start, "expect '(' after keyword 'switch' for the expression");
            return true;
        }
        lexWhitespaceAndNewLines();
        if (lexOperatorToken('{')) {
            while(true) {
                lexWhitespaceAndNewLines();
                if(lexWSKeywordToken("default", ':')) {
                    if (lexOperatorToken(':')) {
                        auto bStart = tokens_size();
                        lexNestedLevelMultipleStatementsTokens();
                        compound_from(bStart, LexTokenType::CompBody);
                    } else if (lexOperatorToken("=>")) {
                        if(!lexBraceBlockOrSingleStmt("switch-default", is_value, lex_value_node)) {
                            (this->*malformed)(start, "expected a brace block after the '=>' in the switch default case");
                            return true;
                        }
                    } else {
                        (this->*malformed)(start, "expected ':' or '=>' after 'default' in switch statement");
                        return true;
                    }
                } else {
                    if(lexWSKeywordToken("case")) {
                        if (!lexSwitchCaseValue()) {
                            (this->*malformed)(start, "expected a value after 'case' in switch");
                            return true;
                        }
                    } else if (!lexSwitchCaseValue()) {
                        break;
                    }
                    lexWhitespaceToken();
                    if (lexOperatorToken(':')) {
                        auto bStart = tokens_size();
                        lexNestedLevelMultipleStatementsTokens();
                        compound_from(bStart, LexTokenType::CompBody);
                        continue;
                    } else if (lexOperatorToken("=>")) {
                        if(!lexBraceBlockOrSingleStmt("switch-case", is_value, lex_value_node)) {
                            (this->*malformed)(start, "expected a brace block after the '=>' in the switch case");
                            return true;
                        }
                    } else {
                        (this->*malformed)(start, "expected ':' or '=>' after 'case' in switch statement");
                        return true;
                    }
                }
            }
            if(!lexOperatorToken('}')) {
                (this->*malformed)(start, "expected '}' for ending the switch block");
                return true;
            }
        } else {
            (this->*malformed)(start, "expected '{' after switch");
            return true;
        }
        compound_from(start, is_value ? LexTokenType::CompSwitchValue : LexTokenType::CompSwitch);
        return true;
    }
    return false;
}