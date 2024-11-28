// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"

void Parser::lexTypeList() {
    do {
        lexWhitespaceToken();
        if (!lexTypeTokens()) {
            break;
        }
        lexWhitespaceToken();
    } while (lexOperatorToken(TokenType::CommaSym));
}

void Parser::lexIdentifierList() {
    do {
        lexWhitespaceToken();
        if (!lexIdentifierToken()) {
            break;
        }
        lexWhitespaceToken();
    } while (lexOperatorToken(TokenType::CommaSym));
}

bool Parser::lexLambdaAfterParamsList(unsigned int start) {
    lexWhitespaceToken();

    if (!lexOperatorToken(TokenType::LambdaSym)) {
        mal_value(start, "expected '=>' for a lambda");
        return false;
    }

    lexWhitespaceToken();

    if (!(lexBraceBlock("lambda") || lexExpressionTokens())) {
        mal_value(start, "expected lambda body");
        return false;
    }

    compound_from(start, LexTokenType::CompLambda);
    return true;
}

bool Parser::lexLambdaValue() {
    if (lexOperatorToken(TokenType::LBracket)) {

        auto start = tokens_size() - 1;

        do {
            lexWhitespaceAndNewLines();
            bool lexed_amp = lexOperatorToken(TokenType::AmpersandSym);
            if (!lexVariableToken()) {
                if(lexed_amp) {
                    error("expected identifier after '&'");
                    return true;
                }
            }
            lexWhitespaceToken();
        } while (lexOperatorToken(TokenType::CommaSym));

        if (!lexOperatorToken(TokenType::RBracket)) {
            error("expected ']' after lambda function capture list");
            return true;
        }

        if (!lexOperatorToken(TokenType::LParen)) {
            error("expected '(' for lambda parameter list");
            return true;
        }

        if(!lexParameterList(true, false)) {
            return true;
        }

        lexNewLineChars();

        if (!lexOperatorToken(TokenType::RParen)) {
            mal_value(start, "expected ')' after the lambda parameter list");
            return true;
        }

        if(!lexLambdaAfterParamsList(start)) {
            return true;
        }

        return true;
    } else {
        return false;
    }
}