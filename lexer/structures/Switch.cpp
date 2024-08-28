// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"

bool Lexer::lexSwitchStatementBlock(bool is_value, bool lex_value_node) {
    if (lexWSKeywordToken("switch", '(')) {
        auto start = tokens_size() - 1;
        if (lexOperatorToken('(')) {
            if (!lexExpressionTokens()) {
                error("expected an expression tokens in switch statement");
            }
            if (!lexOperatorToken(')')) {
                error("expected ')' in switch statement");
            }
        } else {
            error("expect '(' after keyword 'switch' for the expression");
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
                            error("expected a brace block after the '=>' in the switch default case");
                            break;
                        }
                    } else {
                        error("expected ':' or '=>' after 'default' in switch statement");
                        break;
                    }
                } else {
                    if(lexWSKeywordToken("case")) {
                        if (!lexSwitchCaseValue()) {
                            error("expected a value after 'case' in switch");
                            break;
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
                            error("expected a brace block after the '=>' in the switch case");
                            break;
                        }
                    } else {
                        error("expected ':' or '=>' after 'case' in switch statement");
                        break;
                    }
                }
            }
            if(!lexOperatorToken('}')) {
                error("expected '}' for ending the switch block");
            }
        } else {
            error("expected '{' after switch");
        }
        compound_from(start, is_value ? LexTokenType::CompSwitchValue : LexTokenType::CompSwitch);
        return true;
    }
    return false;
}