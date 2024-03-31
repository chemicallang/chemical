// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"

bool Lexer::lexSwitchStatementBlock() {
    if (lexKeywordToken("switch")) {
        lexWhitespaceToken();
        if (lexOperatorToken('(')) {
            if (!lexExpressionTokens()) {
                error("expected an expression tokens in switch statement");
            }
            if (!lexOperatorToken(')')) {
                error("expected ')' in switch statement");
            }
        }
        lexWhitespaceAndNewLines();
        if (lexOperatorToken('{')) {
            while(true) {
                lexWhitespaceAndNewLines();
                if (lexKeywordToken("case")) {
                    lexWhitespaceToken();
                    if (!lexValueToken()) {
                        error("expected a value token");
                    }
                    if (lexOperatorToken(':')) {
                        lexNestedLevelMultipleStatementsTokens();
                    } else if (lexOperatorToken("->")) {
                        lexWhitespaceAndNewLines();
                        if(!lexBraceBlock("switch-case")) {
                            error("expected a brace block after the '->' in the switch case");
                        }
                    } else {
                        error("expected ':' or '->' after 'case' in switch statement");
                    }
                    if(!lexOperatorToken('}')) {
                        error("expected '}' for ending the 'switch' block");
                    }
                } else if(lexKeywordToken("default")) {
                    if (lexOperatorToken(':')) {
                        lexNestedLevelMultipleStatementsTokens();
                    } else if (lexOperatorToken("->")) {
                        lexWhitespaceAndNewLines();
                        if(!lexBraceBlock("switch-default")) {
                            error("expected a brace block after the '->' in the switch default case");
                        }
                    } else {
                        error("expected ':' or '->' after 'default' in switch statement");
                    }
                } else {
                    break;
                }
            }
        } else {
            error("expected '{' after switch");
        }
        return true;
    }
    return false;
}