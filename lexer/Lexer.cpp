// Copyright (c) Qinetik 2024.

#include "Lexer.h"

Lexer::Lexer(
        std::string file_path,
        SourceProvider &provider,
        CompilerBinder* binder
) : file_path(std::move(file_path)), provider(provider), binder(binder), allocator(1000) {

}

bool isWhitespace(char p) {
    return p == ' ' || p == '\t' || p == '\n' || p == '\r';
}

Token Lexer::getNextToken() {
    auto pos = provider.position();
    auto c = provider.readCharacter();
    switch(c) {
        case '{':
            return Token(TokenType::LBrace, { nullptr, 1 }, pos);
        case '}':
            return Token(TokenType::RBrace, { nullptr, 1 }, pos);
        case '(':
            return Token(TokenType::LParen, { nullptr, 1 }, pos);
        case ')':
            return Token(TokenType::RParen, { nullptr, 1 }, pos);
        case '[':
            return Token(TokenType::LBracket, { nullptr, 1 }, pos);
        case ']':
            return Token(TokenType::RBracket, { nullptr, 1 }, pos);
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case '!':
        case '.':
        case ',':
        case ';':
            return Token(TokenType::Operator, { nullptr, 1 }, pos);
        case -1:
            return Token(TokenType::EndOfFile, { nullptr, 0 }, pos);
        case ' ':
        case '\t':
            return Token(TokenType::Whitespace, { nullptr, provider.readWhitespaces() + 1 }, pos);
        case '\n':
            return Token(TokenType::NewLine, { nullptr, 1 }, pos);
        case '\r':
            if(provider.peek() == '\n') {
                provider.readCharacter();
                return Token(TokenType::NewLine, { nullptr, 2 }, pos);
            } else {
                return Token(TokenType::NewLine, { nullptr, 1 }, pos);
            }
        default:
            break;
    }
    if(std::isdigit(c)) {
        auto p = provider.peek();
        if(isWhitespace(p)) {
            return Token(TokenType::Number, { allocator.char_ptr(p), 1 }, pos);
        } else {

        }
    } else if(c == '_' || std::isalpha(c)) {

    }
    return Token(TokenType::Unexpected, { nullptr, 0 }, pos);
}

void Lexer::getUnit(LexUnit& unit) {
    unit.allocator = std::move(allocator);
    unit.tokens.reserve(250);
    while(true) {
        auto token = getNextToken();
        switch(token.type){
            case TokenType::EndOfFile:
                return;
            case TokenType::Unexpected:
                // perform any resetting of lexer state here
                // reset();
                break;
            default:
                unit.tokens.emplace_back(token);
                break;
        }
    }
}