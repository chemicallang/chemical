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
#include "ast/structures/Scope.h"
#include "ast/values/AccessChain.h"
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
     * This parses multiple statements, creates a Scope and moves nodes into the scope
     * and returns it
     */
    std::optional<Scope> parseScope();

    /**
    * This parses multiple statements as ASTNode(s) into nodes
     */
    void parseMultipleStatements();

    /**
     * parses a value of type int, integer !
     * @return
     */
    std::optional<std::unique_ptr<IntValue>> parseIntNode();

    /**
     * parses a single value, which can be an expression, int, float...
     * @return
     */
    std::optional<std::unique_ptr<Value>> parseValueNode();

    /**
     * parse an access chain
     */
    std::optional<std::unique_ptr<AccessChain>> parseAccessChain();

    /**
     * parse the access chain or the value
     * @return
     */
    std::optional<std::unique_ptr<Value>> parseAccessChainOrValue();

    /**
     * parses a variable assignment state
     * @return whether a node was added
     */
    bool parseVarAssignStatement();

    /**
     * This parses a single variable initialization statement
     * @return
     */
    bool parseVariableInitStatement();

    /**
     * This will erase all whitespace tokens
     */
    void eraseAllWhitespaceTokens();

    /**
     * print what current token
     */
    void print_got();

    /**
     * consumes a character operator token
     * @param token value of the token
     * @param errorOut whether to error out if the token is not present
     * @return true if the token has been consumed, false otherwise
     */
    bool consume_op(char token);

    /**
     * it will consume and return a keyword token if found
     * @param keyword
     * @return a keyword token
     */
    lex_ptr<KeywordToken> consume(const std::string &keyword, bool errorOut = true);

    /**
     * sets the given error in the parseError and also prints it
     * @param err
     */
    void error(const std::string &err, int tokenPosition) {
        std::string errStr;
        errStr = "[Parser] " + err;
        if (tokenPosition < tokens.size()) {
            auto t = tokens[tokenPosition].get();
            errStr += " at " + std::to_string(t->lineNumber()) + ':' + std::to_string(t->lineCharNumber()) +
                      " stopped at " + t->type_string();
        }
        errStr.append(1, '\n');
        parseError = errStr;
    }

    void error(const std::string &err) {
        error(err, position);
    }

    /**
     * consumes a token of type, moves position to focus on the next position
     * Please note that after consuming the token, the caller gets the token ownership
     * This function is also inlined
     */
    template<typename T>
    inline std::unique_ptr<T> consume();

    /**
     * this will consume the token at current position
     * actually this just increments the position, since
     * no access to current token as a derived class is required, this is a good alternative
     */
    inline void increment();

    /**
     * checks the type of the token
     * @param type LexTokenType to match
     * @param errorOut will error out if true
     * @return true if matched
     */
    bool check_type(LexTokenType type, bool errorOut = true);

    /**
     *
     * @tparam T
     * @return
     */
    template<typename T>
    lex_ptr<T> consumeOfType(LexTokenType type, bool errorOut = true);

    /**
     * this will return a raw pointer to the token at position
     * as derived token of class given type parameter T
     * This will fail if the token at current position is not of derived class of given type parameter
     * The raw pointer may become dangling, if unique_ptr is destroyed !
     */
    template<typename T>
    inline T *as();

    /**
     * this is used to check if the lexer is in the debug mode
     * @return
     */
    inline bool isDebug() {
        return true;
    }

private:

    /**
     * this is the position in tokens variable
     * the current token being looked at !
     */
    int position = 0;

};

template<typename T>
std::unique_ptr<T> Parser::consume() {
    return std::unique_ptr<T>(static_cast<T *>(tokens[position++].release()));
}

void Parser::increment() {
    position++;
}

template<typename T>
T *Parser::as() {
    return static_cast<T *>(tokens[position].get());
}

template<typename T>
lex_ptr<T> Parser::consumeOfType(LexTokenType type, bool errorOut) {
    if (check_type(type, errorOut)) {
        return consume<T>();
    } else {
        return std::nullopt;
    }
}