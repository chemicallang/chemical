// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 2/19/2024.
//

#include "parser/Parser.h"

bool Parser::lexLambdaTypeTokens(unsigned int start) {
    if(lexOperatorToken(TokenType::LParen)) {
        lexParameterList();
        if(!lexOperatorToken(TokenType::RParen)) {
            error("expected a ')' after the ')' in lambda function type");
        }
        lexWhitespaceToken();
        if(lexOperatorToken(TokenType::LambdaSym)) {
            lexWhitespaceToken();
            if(!lexTypeTokens()) {
                error("expected a return type for lambda function type");
            }
        } else {
            error("expected '=>' for lambda function type");
        }
        compound_from(start, LexTokenType::CompFunctionType);
        return true;
    } else {
        return false;
    }
}

bool Parser::lexGenericTypeAfterId(unsigned int start) {
    if(lexOperatorToken(TokenType::LessThanSym)) {
        do {
            lexWhitespaceToken();
            if(!lexTypeTokens()) {
                break;
            }
            lexWhitespaceToken();
        } while(lexOperatorToken(TokenType::CommaSym));
        lexWhitespaceToken();
        if(!lexOperatorToken(TokenType::GreaterThanSym)) {
            error("expected '>' for generic type");
        }
        compound_from(start, LexTokenType::CompGenericType);
        return true;
    } else {
        return false;
    }
}

bool Parser::lexRefOrGenericType() {
    unsigned start = tokens_size();
    auto id = consumeIdentifierOrKeyword();
    if(!id) {
        error("missing struct / interface name in inheritance list of the struct");
        return false;
    }
    emplace(LexTokenType::Type, id->position, std::string(id->value));
    lexWhitespaceToken();
    lexGenericTypeAfterId(start);
    return true;
}

void Parser::lexArrayAndPointerTypesAfterTypeId(unsigned int start) {
    if(lexOperatorToken(TokenType::LBracket)) {
        // optional array size
        lexExpressionTokens();
        if(!lexOperatorToken(TokenType::RBracket)) {
            error("expected ']' for array type");
            return;
        }
        compound_from(start, LexTokenType::CompArrayType);
    }
    while(lexOperatorToken(TokenType::MultiplySym)) {
        warning("deprecated syntax, pointer should be before type");
        compound_from(start, LexTokenType::CompPointerType);
    }
    if(lexOperatorToken(TokenType::AmpersandSym)) {
        warning("deprecated syntax, reference should be before type");
        compound_from(start, LexTokenType::CompReferenceType);
    }
}

bool Parser::lexTypeId(Token* type, unsigned int start) {
    bool has_multiple = false;
    while(true) {
        if(token->type == TokenType::DoubleColonSym) {
            emplace(LexTokenType::Variable, type->position, std::string(type->value));
            lexOperatorToken(TokenType::DoubleColonSym);
            auto new_type = consumeIdentifierOrKeyword();
            if(!new_type) {
                error("expected an identifier after '" + std::string(type->value) + "::' for a type");
                return false;
            } else {
                has_multiple = true;
                type = new_type;
            }
        } else {
            if(has_multiple) {
                emplace(LexTokenType::Variable, type->position, std::string(type->value));
                compound_from(start, LexTokenType::CompAccessChain);
                compound_from(start, LexTokenType::CompLinkedValueType);
            } else {
                emplace(LexTokenType::Type,type->position, std::string(type->value));
            }
            break;
        }
    }
    return true;
}

bool Parser::lexTypeTokens() {

    if(lexOperatorToken(TokenType::LBracket)) {
        unsigned start = tokens_size() - 1;
        if(!lexOperatorToken(TokenType::RBracket)) {
            error("expected ']' after '[' for lambda type");
            return true;
        }
        lexWhitespaceToken();
        if(!lexLambdaTypeTokens(start)) {
            error("expected a lambda type after '[]'");
        }
        return true;
    }

    if(lexLambdaTypeTokens(tokens_size())) {
        return true;
    }

    if(lexOperatorToken(TokenType::MultiplySym)) {
        unsigned start = tokens_size() - 1;
        if(!lexTypeTokens()) {
            error("expected a type after the *");
            return false;
        }
        compound_from(start, LexTokenType::CompPointerType);
        return true;
    } else if(lexOperatorToken(TokenType::AmpersandSym)) {
        unsigned start = tokens_size() - 1;
        if(!lexTypeTokens()) {
            error("expected a type after the &");
            return false;
        }
        compound_from(start, LexTokenType::CompReferenceType);
        return true;
    }

    if(lexWSKeywordToken(TokenType::DynKw) || lexWSKeywordToken(TokenType::MutKw)) {
        unsigned start = tokens_size() - 1;
        if(!lexTypeTokens()) {
            error("expected a type after the qualifier");
            return false;
        }
        compound_from(start, LexTokenType::CompQualifiedType);
        return true;
    }

    auto type = consumeIdentifierOrKeyword();
    if(!type) return false;
    unsigned start = tokens_size();
    if(!lexTypeId(type, start)) {
        return true;
    }
    lexGenericTypeAfterId(start);
    lexArrayAndPointerTypesAfterTypeId(start);

    return true;

}