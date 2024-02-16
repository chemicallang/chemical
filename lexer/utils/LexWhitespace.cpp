//
// Created by wakaz on 16/02/2024.
//
#include "lexer/Lexer.h"
#include "lexer/model/WhitespaceToken.h"
#include <memory>

int Lexer::lexWhitespace() {
    int whitespaces = 0;
    while (!provider.eof() && provider.peek() == ' ') {
        provider.readCharacter();
        whitespaces++;
    }
    return whitespaces;
}

void Lexer::lexWhitespaceToken(std::vector<std::unique_ptr<LexToken>> &tokens) {
    auto whitespace = lexWhitespace();
    if (whitespace > 0) {
        tokens.emplace_back(std::make_unique<WhitespaceToken>(provider.position() - whitespace, whitespace, lineNumber()));
    }
}
