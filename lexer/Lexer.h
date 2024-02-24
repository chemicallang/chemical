// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include <utility>
#include <vector>
#include "SourceProvider.h"
#include "lexer/model/tokens/LexToken.h"
#include "lexer/model/tokens/IntToken.h"
#include "LexConfig.h"
#include "lexer/model/LexError.h"
#include <memory>
#include <optional>

class Lexer {
public:

    SourceProvider &provider;
    std::string path;

    explicit Lexer(SourceProvider &provider, std::string  path) : provider(provider), path(std::move(path)) {

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
     * lex whitespaces at the current pos
     * @return the number of whitespaces ' ' read
     */
    int lexWhitespace();

    /**
     * lex a string until space occurs
     * @return
     */
    std::string lexString();

    /**
     * lex declaration or initialization tokens
     * @param tokens
     */
    std::optional<LexError> lexVarInitializationTokens(std::vector<std::unique_ptr<LexToken>> &tokens);

    /**
     * lex type tokens
     * @param tokens
     */
    void lexTypeTokens(std::vector<std::unique_ptr<LexToken>> &tokens);

    /**
     * lex preprocess
     * @param tokens
     * @return whether the has token was lexed or not
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
     * @return whether a token was lexed or not
     */
    bool lexIntToken(std::vector<std::unique_ptr<LexToken>> &tokens);

    /**
     * returns a lexing error at current position with the path of current file being lexed
     * @return
     */
    LexError error(std::string& message);

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