// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"

bool Lexer::lexMultipleSwitchCaseValues() {
    bool has_single = false;
    do {
        if(lexSwitchCaseValue()) {
            has_single = true;
        }
        lexWhitespaceToken();
    } while(lexOperatorToken('|'));
    return has_single;
}

bool Lexer::lexSwitchStatementBlock(bool is_value, bool lex_value_node) {
    if (lexWSKeywordToken("switch", '(')) {
        auto start = tokens_size() - 1;

        if (lexOperatorToken('(')) {
            if (!lexExpressionTokens()) {
                mal_value_or_node(start, "expected an expression tokens in switch statement", is_value);
                return true;
            }
            if (!lexOperatorToken(')')) {
                mal_value_or_node(start, "expected ')' in switch statement", is_value);
                return true;
            }
        } else {
            mal_value_or_node(start, "expect '(' after keyword 'switch' for the expression", is_value);
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
                            mal_value_or_node(start, "expected a brace block after the '=>' in the switch default case", is_value);
                            return true;
                        }
                    } else {
                        mal_value_or_node(start, "expected ':' or '=>' after 'default' in switch statement", is_value);
                        return true;
                    }
                } else {
                    if (!lexMultipleSwitchCaseValues()) {
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
                            mal_value_or_node(start, "expected a brace block after the '=>' in the switch case", is_value);
                            return true;
                        }
                    } else {
                        mal_value_or_node(start, "expected ':' or '=>' after 'case' in switch statement", is_value);
                        return true;
                    }
                }
            }
            if(!lexOperatorToken('}')) {
                mal_value_or_node(start, "expected '}' for ending the switch block", is_value);
                return true;
            }
        } else {
            mal_value_or_node(start, "expected '{' after switch", is_value);
            return true;
        }
        compound_from(start, is_value ? LexTokenType::CompSwitchValue : LexTokenType::CompSwitch);
        return true;
    }
    return false;
}