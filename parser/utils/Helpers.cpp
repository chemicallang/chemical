// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "parser/Parser.h"

static bool read_gen_type_token(Parser& parser);

static bool read_arr_type_token(Parser& parser);

static bool read_pointer_type(Parser& parser);

static bool read_type_involving_token(Parser& parser) {
    const auto type = parser.token->type;
    if(type == TokenType::MultiplySym || type == TokenType::AmpersandSym || type == TokenType::DynKw || type == TokenType::MutKw) {
        parser.token++;
        read_type_involving_token(parser);
        return true;
    }
    if(type == TokenType::Identifier || Token::isKeyword(type)) {
        parser.token++;
        while(parser.token->type == TokenType::DoubleColonSym || parser.token->type == TokenType::DotSym) {
            parser.token++;
            const auto next = parser.token->type;
            if(next == TokenType::Identifier || Token::isKeyword(next)) {
                parser.token++;
            }
        }
        read_gen_type_token(parser) || read_arr_type_token(parser) || read_pointer_type(parser);
        return true;
    } else {
        return false;
    }
}

static bool read_pointer_type(Parser& parser) {
    if(parser.consumeToken(TokenType::MultiplySym)) {
        while(parser.consumeToken(TokenType::MultiplySym)) {
            // do nothing
        }
        return true;
    } else {
        return false;
    }
}

static bool read_arr_type_token(Parser& parser) {
    if(parser.consumeToken(TokenType::LBracket)) {
        parser.consumeToken(TokenType::Number);
        if(!parser.consumeToken(TokenType::RBracket)) {
            parser.error("unknown token in look ahead operation for generics, expected '>'");
        }
        return true;
    } else {
        return false;
    }
}

static bool read_gen_type_token(Parser& parser) {
    if(parser.consumeToken(TokenType::LessThanSym)) {
        do {
            if(!read_type_involving_token(parser)) {
                break;
            }
        } while(parser.consumeToken(TokenType::CommaSym));
        if(!parser.consumeGenericClose()) {
            parser.error("unknown token in look ahead operation for generics, expected '>'");
        }
        return true;
    } else {
        return false;
    }
}

bool Parser::isGenericEndAhead() {
    auto current_token = token;
    consumeToken(TokenType::LessThanSym);
    if(consumeGenericClose()) {
        token = current_token;
        return true;
    }
    auto& lexer = *this;
    do {
        if (!read_type_involving_token(lexer)) {
            token = current_token;
            return false;
        }
    } while (consumeToken(TokenType::CommaSym));
    const auto is_generic = consumeGenericClose();
    token = current_token;
    return is_generic;
}

std::optional<AccessSpecifier> Parser::parseAccessSpecifier() {
    switch(token->type) {
        case TokenType::PublicKw:
            token++;
            return AccessSpecifier::Public;
        case TokenType::PrivateKw:
            token++;
            return AccessSpecifier::Private;
        case TokenType::InternalKw:
            token++;
            return AccessSpecifier::Internal;
        case TokenType::ProtectedKw:
            token++;
            return AccessSpecifier::Protected;
        default:
            return std::nullopt;
    }
}