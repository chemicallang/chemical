// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"
#include "cst/statements/SwitchCST.h"
#include "cst/structures/BodyCST.h"

bool Lexer::lexSwitchStatementBlock() {
    if (lexKeywordToken("switch")) {
        auto start = tokens.size() - 1;
        lexWhitespaceToken();
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
                if (lexKeywordToken("case")) {
                    lexWhitespaceToken();
                    if (!lexValueToken()) {
                        error("expected a value after 'case' in switch");
                        break;
                    }
                    lexWhitespaceToken();
                    if (lexOperatorToken(':')) {
                        auto bStart = tokens.size();
                        lexNestedLevelMultipleStatementsTokens();
                        compound_from<BodyCST>(bStart);
                        continue;
                    } else if (lexOperatorToken("->")) {
                        lexWhitespaceAndNewLines();
                        if(!lexBraceBlock("switch-case")) {
                            error("expected a brace block after the '->' in the switch case");
                            break;
                        }
                    } else {
                        error("expected ':' or '->' after 'case' in switch statement");
                        break;
                    }
                } else if(lexKeywordToken("default")) {
                    lexWhitespaceToken();
                    if (lexOperatorToken(':')) {
                        auto bStart = tokens.size();
                        lexNestedLevelMultipleStatementsTokens();
                        compound_from<BodyCST>(bStart);
                    } else if (lexOperatorToken("->")) {
                        lexWhitespaceAndNewLines();
                        if(!lexBraceBlock("switch-default")) {
                            error("expected a brace block after the '->' in the switch default case");
                            break;
                        }
                    } else {
                        error("expected ':' or '->' after 'default' in switch statement");
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
        compound_from<SwitchCST>(start);
        return true;
    }
    return false;
}