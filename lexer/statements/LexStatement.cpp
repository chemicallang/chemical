//
// Created by wakaz on 16/02/2024.
//

#include <memory>
#include <vector>
#include "Lexer/model/LexToken.h"
#include "lexer/Lexer.h"

void Lexer::lexStatementTokens(std::vector<std::unique_ptr<LexToken>> &tokens) {
    if (!lexHash || lexHashOperator(tokens)) {
        lexDeclarationTokens(tokens);
    }
}
