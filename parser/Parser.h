// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "lexer/model/tokens/LexToken.h"
#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/statements/VarInit.h"
#include "ast/values/IntValue.h"
#include "lexer/model/tokens/KeywordToken.h"
#include "lexer/utils/TypeUtils.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include <optional>
#include <iostream>

template<typename T>
using lex_ptr = std::optional<std::unique_ptr<T>>;

class Parser {
public:

    /**
     * These are tokens that are given by the user
     */
    std::vector<std::unique_ptr<LexToken>> tokens;

    /**
     * These are parsed nodes
     * When done this result can be used
     * Please note that nested nodes are present inside their parent nodes
     */
    std::vector<std::unique_ptr<ASTNode>> nodes;

    /**
     * this value is set to the error encountered during parsing
     * if an error occurs
     */
    std::optional<std::string> parseError;

    /**
     * Constructor with the tokens to parse
     * @param tokens
     */
    Parser(std::vector<std::unique_ptr<LexToken>> tokens) : tokens(std::move(tokens)) {
        // nothing here
    }

    /**
     * This parses the given tokens from the lexer
     * converts it into a ast and then returns it
     * @param tokens
     * @return
     */
    void parse();

    /**
     * parses a value of type int, integer !
     * @return
     */
    std::optional<IntValue> parseIntNode();

    /**
     * parses a single value, which can be an expression, int, float...
     * @return
     */
    std::optional<Value> parseValueNode();

    /**
     * This parses a single variable initialization statement
     * @return
     */
    void parseVariableInitStatement();

    /**
     * This will erase all whitespace tokens
     */
    void eraseAllWhitespaceTokens();

    /**
     * consume a character operator token
     * @param token
     * @return character operator token
     */
    lex_ptr<CharOperatorToken> consumeOperator(char token);

    /**
     * it will consume and return a keyword token if found
     * @param keyword
     * @return a keyword token
     */
    lex_ptr<KeywordToken> consume(const std::string& keyword);

    /**
     * sets the given error in the parseError and also prints it
     * @param err
     */
    void error(const std::string& err, const TokenPosition tPos) {
        parseError = "[Parser]" + err;
        std::cout << "Error occurred at " << std::to_string(tPos.lineNumber) << ':' << std::to_string(tPos.lineCharNumber);
    }

    void error(const std::string& err) {
        error("[Parser] " + err, tokens[position]->position);
    }

    /**
     * consumes a token of type, moves position to focus on the next position
     * Please note that after consuming the token, the caller gets the token ownership
     * This function is also inlined
     */
    template<typename T>
    inline std::unique_ptr<T> consume();

    /**
     *
     * @tparam T
     * @return
     */
    template<typename T>
    inline lex_ptr<T> consumeOfType(LexTokenType type);

    /**
     * this will return a raw pointer to the token at position
     * as derived token of class given type parameter T
     * This will fail if the token at current position is not of derived class of given type parameter
     * The raw pointer may become dangling, if unique_ptr is destroyed !
     */
     template<typename T>
     inline T* as();


private:

    /**
     * this is the position in tokens variable
     * the current token being looked at !
     */
    int position = 0;

};

template<typename T>
std::unique_ptr<T> Parser::consume() {
    return std::unique_ptr<T>(static_cast<T*>(tokens[position++].release()));
}

template<typename T>
T* Parser::as() {
    return static_cast<T*>(tokens[position].get());
}

template<typename T>
inline lex_ptr<T> Parser::consumeOfType(LexTokenType type) {
    if (tokens.size() != position) {
        if (tokens[position]->type() == type) {
            return consume<T>();
        } else {
            error("expected a " + toTypeString(type) + " token, got " + toTypeString(tokens[position]->type()));
        }
    } else {
        error("expected a " + toTypeString(type) + " token but there are no tokens left");
    }
    return std::nullopt;
}