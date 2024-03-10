// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include <utility>
#include <vector>
#include "SourceProvider.h"
#include "lexer/model/tokens/LexToken.h"
#include "lexer/model/tokens/NumberToken.h"
#include "lexer/model/LexError.h"
#include "lexer/model/TokenPosition.h"
#include "ast/utils/Operation.h"
#include <memory>
#include <optional>
#include <functional>

class Lexer {
public:

    SourceProvider &provider;

    std::string path;

    std::vector<std::unique_ptr<LexToken>> tokens;

    std::vector<LexError> errors;

    inline bool isDebug() { return true; };

    inline bool shouldAddWhitespaceToken() { return false; }

    explicit Lexer(SourceProvider &provider, std::string path) : provider(provider), path(std::move(path)) {

    }

    /**
     * lex everything to LexTokens, tokens go into 'tokens' member property
     * @return
     */
    virtual void lex();

    /**
     * lexes a number as a string
     * @return
     */
    std::string lexNumber();

    /**
     * read whitespaces at the current pos
     * @return the number of whitespaces ' ' read
     */
    unsigned int readWhitespaces();

    /**
     * lexes anything as long as when lambda returns true
     * @return
     */
    template<typename TFunc>
    std::string lexAnything(TFunc when);

    /**
     * lex all characters into a string until char occurs
     * @return the string that was found
     */
    std::string lexAnything(char until = ' ');

    /**
     * lex a string that contains alphabetical characters only
     * @return alphabetical string or empty if not found
     */
    std::string lexAlpha();

    /**
     * lex a alpha numeric string until until character occurs
     * @return
     */
    std::string lexAlphaNum();

    /**
     * lexes an identifier
     * it doesn't add it as a token use lex Identifier token for that
     * @return
     */
    std::string lexIdentifier();

    /**
     * lex an identifier token into tokens until the until character occurs
     * only lexes the token if the identifier is not empty
     * @return
     */
    bool lexIdentifierToken();

    /**
     * this lexes an access chain like x.y.z or just simply an identifier
     * @return
     */
    bool lexAccessChain();

    /**
     * lex allowDeclarations or initialization tokens
     * like var x : int; or var x : int = 5;
     * @param allowDeclarations when true, it will allow allowDeclarations only without the value initialization
     * like #var x : int; when false however, it'll be strict initialization
     * @return whether it was able to lex the tokens for the statement
     */
    bool lexVarInitializationTokens(bool allowDeclarations = true);

    /**
     * lex assignment tokens
     * like x = 5;
     * @return whether it was able to lex teh tokens for the statement
     */
    bool lexAssignmentTokens();

    /**
     * This lexes a operation token in between two values
     * for example x (token) y -> x + y or x - y
     * @return whether the language operator token has been lexed
     */
    bool lexLanguageOperatorToken();

    /**
     * This lexes a operation token before assignment '='
     * for example +=, -=
     * in this case, equal sign is ignored and operation is determined solely based on the token before it
     * @return whether the language operator token has been lexed
     */
    bool lexAssignmentOperatorToken();

    /**
     * lex type tokens
     */
    bool lexTypeTokens();

    /**
     * lexes a single statement (of any type)
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
    bool lexOperatorToken(const std::string &op);

    /**
     * lexes the given operator as length 1 character operator token
     * @param op
     * @return whether the token was found
     */
    bool lexOperatorToken(char token, Operation op);

    /**
     * lexes the given operator as a string operator token
     * @param op
     * @return whether the token was found
     */
    bool lexOperatorToken(const std::string &token, Operation op);

    /**
     * lexes a keyword token for the given keyword
     * @param keyword
     * @return  whether the keyword was found
     */
    bool lexKeywordToken(const std::string &keyword);

    /**
     * this lexes the tokens inside the body of a structure
     * this basically lexes multiple statements
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
     * lexes import statement
     * @return
     */
    bool lexImportStatement();

    /**
     * lexes return statement
     * @return
     */
    bool lexReturnStatement();

    /**
     * lexes break statement
     * @return
     */
    bool lexBreakStatement();

    /**
     * lexes continue statement
     * @return
     */
    bool lexContinueStatement();

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
     * lex do while block
     */
    bool lexDoWhileBlockTokens();

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
     * lexes interface block, this means { member(s) }
     * without the `interface` keyword and name identifier
     * @return
     */
    void lexInterfaceBlockTokens();

    /**
     * lexes a interface structure
     * @return
     */
    bool lexInterfaceStructureTokens();

    /**
     * lexes a single member of the struct
     * @return
     */
    bool lexStructMemberTokens();

    /**
     * lexes struct block, this means { member(s) }
     * without the `struct` keyword and name identifier
     * @return
     */
    bool lexStructBlockTokens();

    /**
     * lexes a struct block
     * @return
     */
    bool lexStructStructureTokens();

    /**
     * lexes a impl block tokens
     * @return
     */
    void lexImplBlockTokens();

    /**
     * lexes a impl block
     * @return
     */
    bool lexImplTokens();

    /**
     * lexes an enum block, this means { enum(s) }
     * without the `enum` keyword and name identifier
     * @return
     */
    bool lexEnumBlockTokens();

    /**
     * lexes a enum block
     * @return
     */
    bool lexEnumStructureTokens();

    /**
     * This method is called when a scope beings
     */
    virtual void scope_begins() {

    }

    /**
     * This method is called when a scope ends
     */
    virtual void scope_ends() {

    }

    /**
     * lex whitespace tokens
     * @return
     */
    bool lexWhitespaceToken();

    /**
     * a utility function to lex whitespace tokens and also skip new lines
     */
    void lexWhitespaceAndNewLines();

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
     * lex hash macro
     * @return
     */
    bool lexHashMacro();

    /**
     * lexes a bool, true or false
     * @return whether a bool has been lexed
     */
    bool lexBoolToken();

    /**
     * lex an number token
     * @return whether a token was lexed or not
     */
    bool lexNumberToken();

    /**
     * lexes value tokens like integer, string
     */
    bool lexValueToken();

    /**
     * lexes array syntax values like [1,2,3,4]
     * for easy array creation
     * @return
     */
    bool lexArrayInit();

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
    void error(const std::string &message);

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

    unsigned int modifiers = 0;

    /**
     * -----------------------------------------
     * Developer Note:
     * when the member bool starts with "is" (e.g isLexReturnStatement)
     * it means this variable is a state, in case of return statement
     * when a function block is found, this variable is set to true
     * so that statements inside that block can contain return statement
     * when the block finishes this variable is set to false, to disallow return statements
     * so these variables can be switched on and off in the middle of the lexing
     * -----------------------------------------
     */

    /**
     * when true, return statements will be lexed
     */
    bool isLexReturnStatement = false;

    /**
     * when true, continue statements will be lexed
     */
    bool isLexContinueStatement = false;

    /**
     * when true, break statements will be lexed
     */
    bool isLexBreakStatement = false;

    /**
     * when true, import statements will be lexed
     */
    bool isLexImportStatement = true;

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
template<typename TFunc>
std::string Lexer::lexAnything(TFunc when) {
    std::string str;
    while (!provider.eof() && when()) {
        str.append(1, provider.readCharacter());
    }
    return str;
}