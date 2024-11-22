// Copyright (c) Qinetik 2024.

#include "Lexer.h"

Lexer::Lexer(
        std::string file_path,
        SourceProvider &provider,
        CompilerBinder* binder
) : file_path(std::move(file_path)), provider(provider), binder(binder) {

}

char* Lexer::putSingleChar(char c) {
    auto d = multi_str.mutable_data();
    multi_str.append(c);
    multi_str.append('\0');
    return d;
}

Token Lexer::getNextToken() {
    auto pos = provider.position();
    auto c = provider.readCharacter();
    switch(c) {
        case '{':
            return Token(TokenType::LBrace, { nullptr, 0 }, pos);
        case '}':
            return Token(TokenType::RBrace, { nullptr, 0 }, pos);
        case '(':
            return Token(TokenType::LParen, { nullptr, 0 }, pos);
        case ')':
            return Token(TokenType::RParen, { nullptr, 0 }, pos);
        case '[':
            return Token(TokenType::LBracket, { nullptr, 0 }, pos);
        case ']':
            return Token(TokenType::RBracket, { nullptr, 0 }, pos);
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case '!':
        case '.':
            return Token(TokenType::Operator, { nullptr, 0 }, pos);
        case -1:
            return Token(TokenType::EndOfFile, { nullptr, 0 }, pos);
        default:
            break;
    }
    if(std::isdigit(c)) {

    } else if(c == '_' || std::isalpha(c)) {

    }
}

void Lexer::getUnit(LexUnit& unit) {
    unit.allocated_multi_str = std::move(multi_str);
    unit.tokens.reserve(250);
    while(true) {
        auto token = getNextToken();
        if (token.type == TokenType::EndOfFile) {
            return;
        } else {
            unit.tokens.emplace_back(token);
        }
    }
}