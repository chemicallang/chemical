// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"

bool Lexer::lexSwitchStatementBlock() {
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
                if (lexWSKeywordToken("case")) {
                    if (!lexSwitchCaseValue()) {
                        error("expected a value after 'case' in switch");
                        break;
                    }
                    lexWhitespaceToken();
                    if (lexOperatorToken(':')) {
                        auto bStart = tokens_size();
                        lexNestedLevelMultipleStatementsTokens();
                        compound_from(bStart, LexTokenType::CompBody);
                        continue;
                    } else if (lexOperatorToken("=>")) {
                        lexWhitespaceAndNewLines();
                        if(!lexBraceBlock("switch-case")) {
                            error("expected a brace block after the '=>' in the switch case");
                            break;
                        }
                    } else {
                        error("expected ':' or '=>' after 'case' in switch statement");
                        break;
                    }
                } else if(lexWSKeywordToken("default", ':')) {
                    if (lexOperatorToken(':')) {
                        auto bStart = tokens_size();
                        lexNestedLevelMultipleStatementsTokens();
                        compound_from(bStart, LexTokenType::CompBody);
                    } else if (lexOperatorToken("=>")) {
                        lexWhitespaceAndNewLines();
                        if(!lexBraceBlock("switch-default")) {
                            error("expected a brace block after the '=>' in the switch default case");
                            break;
                        }
                    } else {
                        error("expected ':' or '=>' after 'default' in switch statement");
                        break;
                    }
                } else {
                    break;
                }
            }
            if(!lexOperatorToken('}')) {
                error("expected '}' for ending the switch block");
            }
        } else {
            error("expected '{' after switch");
        }
        compound_from(start, LexTokenType::CompSwitch);
        return true;
    }
    return false;
}