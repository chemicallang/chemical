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
#include <memory>
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

    /**
     * lex declaration tokens
     * @param tokens
     */
    void lexVarInitializationTokens(std::vector<std::unique_ptr<LexToken>> &tokens);

    /**
     * lex type tokens
     * @param tokens
     */
    void lexTypeTokens(std::vector<std::unique_ptr<LexToken>> &tokens);

    /**
     * lex preprocess
     * @param tokens
     */
    bool lexHashOperator(std::vector<std::unique_ptr<LexToken>> &tokens);

    /**
     * lexes a single statement (of any type)
     * @param tokens
     */
    void lexStatementTokens(std::vector<std::unique_ptr<LexToken>> &tokens);

    /**
     * lex whitespace tokens
     * @param tokens
     * @return
     */
    void lexWhitespaceToken(std::vector<std::unique_ptr<LexToken>> &tokens);

    /**
     * lex an integer token
     * @param tokens
     * @return
     */
    bool lexIntToken(std::vector<std::unique_ptr<LexToken>> &tokens);

    /**
     * gets the line number from the provider
     * @return
     */
    inline unsigned int lineNumber() {
        return provider.getLineNumber();
    }

private:
    bool lexHash = true;

};


#endif //COMPILER_LEXER_H
