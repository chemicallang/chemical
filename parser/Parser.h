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
#include "ast/values/Expression.h"
#include "ast/structures/If.h"
#include "ast/structures/WhileLoop.h"
#include "ast/structures/DoWhileLoop.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/ForLoop.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/values/BoolValue.h"
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
     * An error is stored in the vector
     */
    std::vector<std::string> errors;

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
    Scope parseScope();

    /**
    * This parses multiple statements as ASTNode(s) into nodes
     */
    void parseMultipleStatements();

    /**
     * parses a value of type int, integer !
     * @return
     */
    lex_ptr<IntValue> parseIntValue();

    /**
     * parses a bool value, true or false
     * @return
     */
    lex_ptr<BoolValue> parseBoolValue();

    /**
     * parses a single value, which can be an expression, int, float...
     * @return
     */
    lex_ptr<Value> parseValue();

    /**
     * parses a single operation
     * @return
     */
    std::optional<Operation> parseOperation();

    /**
     * parse remaining expression
     * @return if parsed, new expression containing the firstValue operation secondValue is returned
     * otherwise firstValue is returned
     */
    std::unique_ptr<Value> parseRemainingExpression(std::unique_ptr<Value> firstValue);

    /**
     * parses an expression
     */
     lex_ptr<Value> parseExpression();

    /**
     * parse an access chain
     */
    lex_ptr<AccessChain> parseAccessChain();

    /**
     * parse the access chain or the value
     * @return
     */
    lex_ptr<Value> parseAccessChainOrValue();

    /**
     * parses a variable assignment state
     * @return whether a node was added
     */
    bool parseVarAssignStatement();

    /**
     * This parses a single variable initialization statement
     * @return
     */
    lex_ptr<VarInitStatement> parseVariableInitStatement();

    /**
     * This parses a single variable initialization statement
     * @return true if parsed
     */
    bool parseVariableInitStatementBool();

    /**
     * Parse a single if statement
     * @return
     */
    lex_ptr<IfStatement> parseIfStatement();

    /**
     * parses an if statement into nodes, if found return strue
     * @return
     */
    bool parseIfStatementBool();

    /**
     * Parse a single for loop
     * @return
     */
    lex_ptr<ForLoop> parseForLoop();

    /**
     * parses a single for loop
     * @return true if parsed
     */
    bool parseForLoopBool();

    /**
     * Parse a single while loop
     * @return
     */
    lex_ptr<WhileLoop> parseWhileLoop();

    /**
     * parses a while loop
     * @return true, if parsed
     */
    bool parseWhileLoopBool();

    /**
     * Parse a single do while loop
     * @return
     */
    lex_ptr<DoWhileLoop> parseDoWhileLoop();

    /**
     * parses a single do while loop
     * @return true if parsed
     */
    bool parseDoWhileLoopBool();

    /**
     * Parse a single function definition
     * @return
     */
    lex_ptr<FunctionDeclaration> parseFunctionDefinition();

    /**
     * Parse a single function definition
     * @return true if parsed
     */
    bool parseFunctionDefinitionBool();

    /**
     * Parse a single enum declaration
     * @return
     */
    lex_ptr<EnumDeclaration> parseEnumDeclaration();

    /**
     * Parse a single enum declaration
     * @return true if parsed
     */
    bool parseEnumDeclarationBool();

    /**
     * Parse a single interface definition
     * @return
     */
    lex_ptr<EnumDeclaration> parseInterfaceDefinition();

    /**
     * Parse a single interface definition
     * @return true if parsed
     */
    bool parseInterfaceDefinitionBool();

    /**
     * Parse a single struct definition
     * @return
     */
    lex_ptr<EnumDeclaration> parseStructDefinition();

    /**
     * Parse a single struct definition
     * @return true if parsed
     */
    bool parseStructDefinitionBool();

    /**
     * This will erase all whitespace tokens
     */
    void eraseAllWhitespaceAndMultilineCommentTokens();

    /**
     * print what current token
     */
    void print_got();

    /**
     * gets current operator token if its a char operator of course
     * @return
     */
    std::optional<char> get_op_token();

    /**
     * consume a char token
     * @return
     */
    std::optional<char> consume_char_token();

    /**
     * consume any string operator token
     * @return
     */
    std::optional<std::string> consume_str_op();

    /**
     * consumes a character operator token
     * @param token value of the token
     * @param errorOut whether to error out if the token is not present
     * @return true if the token has been consumed, false otherwise
     */
    bool consume_op(char token);

    /**
     * it will consume and return true if found the keyword
     * @param keyword
     * @return
     */
    bool consume(const std::string &keyword);

    /**
     * it will consume and return a keyword token if found
     * @param keyword
     * @return a keyword token
     */
    lex_ptr<KeywordToken> consume(const std::string &keyword, bool errorOut);

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
        errors.push_back(errStr);
    }

    void error(const std::string &err) {
        error(err, position);
    }

    /**
     * get current token type
     * @return
     */
    inline LexTokenType token_type() {
        return tokens[position]->type();
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