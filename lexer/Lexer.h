//
// Created by wakaz on 10/12/2023.
//

#ifndef COMPILER_LEXER_H
#define COMPILER_LEXER_H


#include <vector>
#include "../SourceProvider.h"
#include "model/LexToken.h"
#include "model/IntToken.h"
#include <optional>

class Lexer {
public:

    SourceProvider &provider;

    explicit Lexer(SourceProvider &provider1) : provider(provider1) {

    }

    virtual std::vector<LexToken *> lex();

    std::optional<int> lexInt(bool intOnly = false);

};


#endif //COMPILER_LEXER_H
