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
#include <functional>

class Lexer {
public:

    SourceProvider &provider;

    std::string path;

    std::vector<std::unique_ptr<LexToken>> tokens;

    std::optional<LexError> lexError = std::nullopt;

    inline bool isDebug(){ return true; };

    inline bool shouldAddWhitespaceToken() { return false; }

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
     * lexes anything as long as when lambda returns true
     * @return
     */
    template <typename TFunc>
    std::string lexAnything(TFunc when);

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
     * this lexes an access chain like x.y.z or just simply an identifier
     * @return
     */
    bool lexAccessChain();

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
     * lex allowDeclarations or initialization tokens
     * like var x : int; or var x : int = 5;
     * @param tokens
     * @param allowDeclarations when true, it will allow allowDeclarations only without the value initialization
     * like #var x : int; when false however, it'll be strict initialization
     * @return whether it was able to lex the tokens for the statement
     */
    bool lexVarInitializationTokens(bool allowDeclarations = true);

    /**
     * lex assignment tokens
     * like x = 5;
     * @param tokens
     * @return whether it was able to lex teh tokens for the statement
     */
    bool lexAssignmentTokens();

    /**
     * lexes a single language operator token
     * like +, -
     * @return whether the language operator token has been lexed
     */
    bool lexLanguageOperatorToken();

    /**
     * lex type tokens
     * @param tokens
     */
    bool lexTypeTokens();

    /**
     * lex preprocess
     * @param tokens
     * @return whether the has token was lexed or not
     */
    inline bool lexHashOperator() {
        return lexKeywordToken("#");
    }

    /**
     * lexes a single statement (of any type)
     * @param tokens
     * @return whether a statement was lexed successfully
     */
    bool lexStatementTokens();

    /**
     * lexes the given operator as length 1 character operator token
     * @param op
     * @return whether the token was found
     */
    bool lexOperatorToken(char op);

    /**
     * lexes the given operator as a string operator token
     * @param op
     * @return whether the token was found
     */
    bool lexOperatorToken(const std::string& op);

    /**
     * lexes a keyword token for the given keyword
     * @param keyword
     * @return  whether the keyword was found
     */
    bool lexKeywordToken(const std::string& keyword);

    /**
     * lexes a conditional operator like >,<,>=,<=
     * @return whether the conditional operator was found
     */
    bool lexConditionalOperator();

    /**
     * lexes a conditional statement at the current position
     * @return whether the conditional statement was found
     */
    bool lexConditionalStatement();

    /**
     * this lexes the tokens inside the body of a structure
     * this basically lexes multiple statements
     * @param tokens
     */
    void lexMultipleStatementsTokens();

    /**
     * lex single comment comment
     */
    bool lexSingleLineCommentTokens();

    /**
     * lex multi line comment tokens
     */
    bool lexMultiLineCommentTokens();

    /**
     * lexes a brace block, { statement(s) }
     */
    bool lexBraceBlock();

    /**
     * lexex an if statement without the body
     * @return
     */
    bool lexIfSignature();


    /**
     * lexes a single if statement with the body without else if or else
     * @return
     */
    bool lexSingleIf();

    /**
     * lex if block
     */
     bool lexIfBlockTokens();

     /**
      * lex while block
      */
     bool lexWhileBlockTokens();

     /**
      * lex for block tokens
      */
     bool lexForBlockTokens();

     /**
      * lex parameter list
      */
     void lexParameterList();

    /**
    * lexes a function signature with parameters
    * @return
    */
    bool lexFunctionSignatureTokens();

     /**
      * lexes a function block with parameters
      * @return
      */
     bool lexFunctionStructureTokens();

    /**
     * lex whitespace tokens
     * @param tokens
     * @return
     */
    void lexWhitespaceToken();

    /**
     * lexes a string token, string is enclosed inside double quotes
     * @return whether a string has been lexed
     */
    bool lexStringToken();

    /**
     * lexes a char token, char is enclosed inside single quotes
     * @return whether a char has been lexed
     */
    bool lexCharToken();

    /**
     * lexes a bool, true or false
     * @return whether a bool has been lexed
     */
    bool lexBoolToken();

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
     * lexes access chain like x.y.z or a value like 10, could be int, string, char
     * @return
     */
    bool lexAccessChainOrValue();

    /**
     * lexes remaining expression, this is used by lexExpressionTokens
     * this lexes the expression tokens after the first identifier / value
     * for example in expression a + b, after lexing a + b will lexed by this function
     */
    void lexRemainingExpression();

    /**
     * lexes an expression token which can contain access chain and values
     * @return whether an expression has been lexed, the expression can also be a single identifier or value
     */
    bool lexExpressionTokens();

    /**
     * check if there's a new line at current position
     * @return true if there's a newline otherwise false
     */
    bool hasNewLine();

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

/**
 * when given 'n' as the value, it will return \\n
 * @param value
 * @return
 */
char escape_sequence(char value);

/**
 * The implementation for lexAnything
 * This is required in header because of template usage
 */
template <typename TFunc>
std::string Lexer::lexAnything(TFunc when) {
    std::string str;
    while (!provider.eof() && when()) {
        char x = provider.readCharacter();
        str.append(1, x);
    }
    return str;
}