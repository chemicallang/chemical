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
#include "lexer/model/TokenPosition.h"
#include <memory>
#include <optional>

class Lexer {
public:

    SourceProvider &provider;

    std::string path;

    std::vector<std::unique_ptr<LexToken>> tokens;

    std::optional<LexError> lexError = std::nullopt;

    inline bool isDebug(){ return true; };

    inline bool addWhitespaceToken() { return false; }

    explicit Lexer(SourceProvider &provider, std::string path) : provider(provider), path(std::move(path)) {

    }

    /**
     * lex everything to LexTokens, return with ownership
     *
     * Since this just moves the tokens to you, the tokens member becomes invalid
     * So accessing member tokens can lead to an error, if not initialized before using Lexer
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
    unsigned int lexWhitespace();

    /**
     * lex all characters into a string until char occurs
     * @return the string that was found
     */
    std::string lexAnything(char until = ' ');

    /**
     * lex a alpha numeric string until until character occurs
     * @return
     */
    std::string lexAlphaNum();

    /**
     * lex an identifier token into tokens until the until character occurs
     * only lexes the token if the identifier is not empty
     * @return
     */
    std::string lexIdentifierToken();

    /**
     * shortcut for lexIdentifierToken, only difference is this returns true if token was lexed
     * @param tokens
     * @param until
     * @return
     */
    inline bool lexIdentifierTokenBool() {
        return !lexIdentifierToken().empty();
    }

    /**
     * lex declaration or initialization tokens
     * like var x : int; or var x : int = 5;
     * @param tokens
     */
    void lexVarInitializationTokens();

    /**
     * lex assignment tokens
     * like x = 5;
     * @param tokens
     * @return
     */
    void lexAssignmentTokens();

    /**
     * lex type tokens
     * @param tokens
     */
    void lexTypeTokens();

    /**
     * lex preprocess
     * @param tokens
     * @return whether the has token was lexed or not
     */
    bool lexHashOperator();

    /**
     * lexes a single statement (of any type)
     * @param tokens
     */
    void lexStatementTokens();

    /**
     * this lexes the tokens inside the body of a structure
     * this basically lexes multiple statements
     * @param tokens
     */
    void lexMultipleStatementsTokens();

    /**
     * lex whitespace tokens
     * @param tokens
     * @return
     */
    void lexWhitespaceToken();

    /**
     * lex an integer token
     * @param tokens
     * @return whether a token was lexed or not
     */
    bool lexIntToken();

    /**
     * lexes value tokens like integer, string
     * @param tokens
     */
    bool lexValueToken();

    /**
     * All the chars that cause new line
     * for example \n \r
     * @return
     */
    bool lexNewLineChars();

    /**
     * returns a lexing error at current position with the path of current file being lexed
     * @return
     */
    void error(const std::string& message);

    /**
     * returns the token position at the very current position
     * @return
     */
    TokenPosition position();

    /**
     * when you have read the character from the stream, you create a position, \n\n
     * it corresponds to the position at the end of the character and not at the start \n\n
     * instead of saving the position in a variable before you read and consume characters \n
     * You should get position after reading the characters and basically subtract the length of the token \n\n
     * You can provide the length of the token to this function \n\n
     * Note that token must be on the same line
     * @param back
     * @return
     */
    TokenPosition backPosition(unsigned int back);

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