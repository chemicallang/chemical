// Copyright (c) Qinetik 2024.

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
        read_gen_type_token(parser) || read_arr_type_token(parser) || read_pointer_type(parser);
        return true;
    } else {
        return false;
    }
}

static bool read_pointer_type(Parser& parser) {
    auto t = parser.consumeOfType(TokenType::MultiplySym);
    if(t) {
        while(parser.consumeOfType(TokenType::MultiplySym)) {
            // do nothing
        }
        return true;
    } else {
        return false;
    }
}

static bool read_arr_type_token(Parser& parser) {
    if(parser.consumeOfType(TokenType::LBracket)) {
        parser.consumeOfType(TokenType::Number);
        if(!parser.consumeOfType(TokenType::RBracket)) {
            parser.error("unknown token in look ahead operation for generics, expected '>'");
        }
        return true;
    } else {
        return false;
    }
}

static bool read_gen_type_token(Parser& parser) {
    if(parser.consumeOfType(TokenType::LessThanSym)) {
        read_type_involving_token(parser);
        if(!parser.consumeOfType(TokenType::GreaterThanSym)) {
            parser.error("unknown token in look ahead operation for generics, expected '>'");
        }
        return true;
    } else {
        return false;
    }
}

bool Parser::isGenericEndAhead() {
    auto current_token = token;
    consumeOfType(TokenType::LessThanSym);
    auto& lexer = *this;
    do {
        if (!read_type_involving_token(lexer)) {
            token = current_token;
            return false;
        }
    } while (consumeOfType(TokenType::CommaSym));
    const bool is_generic = consumeOfType(TokenType::GreaterThanSym);
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