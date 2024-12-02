// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "parser/Parser.h"

bool Parser::lexRemainingExpression(unsigned start) {

    lexWhitespaceToken();
    bool compounded = false;
    if (lexWSKeywordToken(TokenType::AsKw)) {
        if (!lexTypeTokens()) {
            error("expected a type for casting after 'as' in expression");
            return false;
        }
        compound_from(start, LexTokenType::CompCastValue);
        lexWhitespaceToken();
        compounded = true;
    } else if(lexWSKeywordToken(TokenType::IsKw)) {
        if (!lexTypeTokens()) {
            error("expected a type after 'is' or '!is' in expression");
            return false;
        }
        compound_from(start, LexTokenType::CompIsValue);
        lexWhitespaceToken();
        compounded = true;
    }
    if (!lexLanguageOperatorToken()) {
        return compounded;
    }
    lexWhitespaceToken();
    if (!lexExpressionTokens(false, false)) {
        error("expected an expression after the operator token in the expression");
        return false;
    }

    compound_from(start, LexTokenType::CompExpression);
    return true;

}

// lexes lambda after comma which occurs after a parameter param : type,  <-----
// this can be called after lparen to lex lambda, if it has no parameter
bool condLexLambdaAfterComma(Parser *lexer, unsigned int start) {
    lexer->lexNewLineChars();
    if (!lexer->lexOperatorToken(TokenType::RParen)) {
        return false;
    }
    lexer->lexLambdaAfterParamsList(start);
    return true;
}

void lexLambdaAfterComma(Parser *lexer, unsigned int start) {
    if (!condLexLambdaAfterComma(lexer, start)) {
        lexer->mal_value(start, "expected ')' after the lambda parameter list in parenthesized expression");
    }
}

bool Parser::lexLambdaOrExprAfterLParen() {
    unsigned int start = tokens_size() - 1;

    lexWhitespaceToken();

    // a lambda with no params
    if (condLexLambdaAfterComma(this, start)) {
        return true;
    }

    if (!lexVariableToken()) {
        return false;
    }

    bool has_whitespace = lexWhitespaceToken();

    if (token->type == TokenType::RParen) {
        compound_from(start + 1, LexTokenType::CompFunctionParam);
        lexOperatorToken(TokenType::RParen);
        lexLambdaAfterParamsList(start);
        return true;
    } else if (lexOperatorToken(TokenType::ColonSym)) {
        lexWhitespaceToken();
        if (lexTypeTokens()) {
            lexWhitespaceToken();
        } else {
            error("expected a type after ':' when lexing a lambda in parenthesized expression");
        }
        compound_from(start + 1, LexTokenType::CompFunctionParam);
        if (lexOperatorToken(TokenType::CommaSym)) {
            lexParameterList(true, false);
        }
        lexLambdaAfterComma(this, start);
        return true;
    } else if (token->type == TokenType::CommaSym) {
        compound_from(start + 1, LexTokenType::CompFunctionParam);
        lexOperatorToken(TokenType::CommaSym);
        lexParameterList(true, false);
        lexLambdaAfterComma(this, start);
        return true;
    }

    if(has_whitespace) {
        lexRemainingExpression(start + 1);
        if(lexOperatorToken(TokenType::RParen)) {
            if(!lexRemainingExpression(start)) {
                compound_from(start, LexTokenType::CompExpression);
            }
            return true;
        } else if(lexRemainingExpression(start + 1) && lexOperatorToken(TokenType::RParen)) {
            compound_from(start, LexTokenType::CompExpression);
        } else {
            error("expected ')' after the nested parenthesized expression");
        }
        return true;
    } else {
        lexAccessChainAfterId();
        if(!unit.tokens[start + 1]->is_struct_value()) {
            compound_from(start + 1, LexTokenType::CompAccessChain);
        }
        lexRemainingExpression(start);
        if(!lexOperatorToken(TokenType::RParen)) {
            error("expected a ')' after the access chain");
        }
        return true;
    }

}

bool Parser::lexParenExpressionAfterLParen() {

    if (!lexExpressionTokens(false, false)) {
        error("expected a nested expression after starting parenthesis ( in the expression");
        return false;
    };

    if (!lexOperatorToken(TokenType::RParen)) {
        error("missing ) in the expression");
        return false;
    }

    return true;

}

bool Parser::lexParenExpression() {
    if (lexOperatorToken(TokenType::LParen)) {
        lexParenExpressionAfterLParen();
        return true;
    } else {
        return false;
    }
}

Value* Parser::parseExpression(ASTAllocator& allocator, bool parseStruct, bool parseLambda) {

    // TODO
    return nullptr;

}

bool Parser::lexExpressionTokens(bool lexStruct, bool lambda) {

    if (lexOperatorToken(TokenType::MinusSym)) {
        auto start = tokens_size() - 1;
        if (!(lexParenExpression() || lexAccessChainOrValue(false))) {
            error("expected an expression after '-' negative");
            return false;
        }
        compound_from(start, LexTokenType::CompNegative);
        lexRemainingExpression(start);
        return true;
    }

    if (lexOperatorToken(TokenType::NotSym)) {
        auto start = tokens_size() - 1;
        if (!(lexParenExpression() || lexAccessChainOrValue(false))) {
            error("expected an expression after '!' not");
            return false;
        }
        compound_from(start, LexTokenType::CompNot);
        lexRemainingExpression(start);
        return true;
    }

    if (lexOperatorToken(TokenType::LParen)) {
        unsigned start = tokens_size() - 1;
        if (lambda && lexLambdaOrExprAfterLParen()) {
            return true;
        }
        if(lexParenExpressionAfterLParen() && !lexRemainingExpression(start)) {
            compound_from(start, LexTokenType::CompExpression);
        }
        return true;
    }

    if (!lexAccessChainOrValue(lexStruct)) {
        return false;
    } else if(lexStruct && unit.tokens[tokens_size() - 1]->is_struct_value()) {
        return true;
    }

    lexWhitespaceToken();

    if (token->type == TokenType::LessThanSym && isGenericEndAhead()) {
        auto start = tokens_size() - 1;
        lexFunctionCallWithGenericArgsList();
        if(lexOperatorToken(TokenType::DotSym) && !lexAccessChainRecursive(false)) {
            error("expected a identifier after the dot . in the access chain");
            return false;
        }
        compound_from(start, LexTokenType::CompAccessChain);
        return true;
    }

    lexRemainingExpression(tokens_size() - 1);

    return true;

}