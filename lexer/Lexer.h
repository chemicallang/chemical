//
// Created by wakaz on 10/12/2023.
//

#ifndef COMPILER_LEXER_H
#define COMPILER_LEXER_H


#include <vector>
#include "SourceProvider.h"
#include "model/LexToken.h"
#include "model/IntToken.h"
#include "LexConfig.h"
#include <optional>

class Lexer {
public:

    SourceProvider &provider;

    explicit Lexer(SourceProvider &provider1) : provider(provider1) {

    }

    /**
     * lex everything to LexTokens
     * @return
     */
    virtual std::vector<std::unique_ptr<LexToken>> lex(const LexConfig &config);

    /**
     * lex an optional integer at the current stream
     * @param intOnly
     * @return
     */
    std::optional<int> lexInt(bool intOnly = false);

    /**
     * lex whitespaces at the current position
     * @return the number of whitespaces ' ' read
     */
    int lexWhitespace();

    /**
     * lex a string until space occurs
     * @return
     */
    std::string lexString();

private:

    /**
     * line numbers start from 0
     * line number currently being lexed
     */
    int lineNumber = 0;

    bool lexingString = false;
    bool lexingWhitespace = false;
    bool lexedVariableName = false;
    bool lexingEqual = false;

};


#endif //COMPILER_LEXER_H
