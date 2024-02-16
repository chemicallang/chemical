//
// Created by wakaz on 10/12/2023.
//

#include <iostream>
#include "Lexer.h"
#include "model/KeywordToken.h"
#include "model/WhitespaceToken.h"
#include "model/IdentifierToken.h"
#include "model/AssignmentOperatorToken.h"
#include "model/SemiColonToken.h"


std::vector<std::unique_ptr<LexToken>> Lexer::lex(const LexConfig &config) {
    std::vector<std::unique_ptr<LexToken>> tokens;
    while (!provider.eof() && provider.peek() != EOF) {
//        std::cout << "Lex Token Session, Character : " << provider.peek() << std::endl;
        lexStatementTokens(tokens);
        char unhandled = provider.readCharacter();
        std::cout << "[Lexer] Unhandled Character : " << unhandled << std::endl;
    }
    return tokens;
}
