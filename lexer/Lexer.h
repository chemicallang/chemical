// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include <utility>
#include <vector>
#include "stream/SourceProvider.h"
#include "lexer/model/tokens/LexToken.h"
#include "lexer/model/tokens/NumberToken.h"
#include "ast/utils/Operation.h"
#include "common/Diagnostic.h"
#include "ast/base/BaseType.h"
#include "ast/structures/StructDefinition.h"
#include <memory>
#include <optional>
#include <functional>

class Lexer {
public:

    SourceProvider &provider;

    std::string path;

    std::vector<std::unique_ptr<LexToken>> tokens;

    std::vector<Diagnostic> errors;

    /**
     * this is the interpret scope used by the lexer to interpret
     * things in between lexing
     */
    GlobalInterpretScope interpret_scope;

    /**
     * this is a flag, that would be set to true, if errors are detected during lexing
     */
    bool has_errors = false;

    inline bool isDebug() { return true; };

    explicit Lexer(SourceProvider &provider, std::string path);

    /**
     * lex everything to LexTokens, tokens go into 'tokens' member property
     * @return
     */
    virtual void lex();

    /**
     * lexes a number as a string
     * @return
     */
    inline std::string lexNumber() {
        return provider.readNumber();
    }

    /**
     * read whitespaces at the current pos
     * @return the number of whitespaces ' ' read
     */
    inline unsigned int readWhitespaces() {
        return provider.readWhitespaces();
    }

    /**
     * lexes anything as long as when lambda returns true
     * @return
     */
    template<typename TFunc>
    inline std::string lexAnything(TFunc when) {
        return provider.readAnything(when);
    }

    /**
     * lex all characters into a string until char occurs
     * @return the string that was found
     */
    inline std::string lexAnything(char until = ' ') {
        return provider.readAnything(until);
    }

    /**
     * lex a string that contains alphabetical characters only
     * @return alphabetical string or empty if not found
     */
    inline std::string lexAlpha() {
        return provider.readAlpha();
    }

    /**
     * lex a alpha numeric string until until character occurs
     * @return
     */
    inline std::string lexAlphaNum() {
        return provider.readAlphaNum();
    }

    /**
     * lexes an identifier
     * it doesn't add it as a token use lex Identifier token for that
     * @return
     */
    inline std::string lexIdentifier() {
        return provider.readIdentifier();
    }

    /**
     * consumes a identifier token
     * @return true if identifier is not empty, false if it is
     */
    bool storeIdentifier(const std::string &identifier, bool access);

    /**
     * lex an identifier token into tokens until the until character occurs
     * only lexes the token if the identifier is not empty
     * @param access is this token being lexed in an access statement instead of assignment
     * @return
     */
    inline bool lexIdentifierToken(bool access) {
        return storeIdentifier(lexIdentifier(), access);
    }

    /**
     * this lexes an access chain like x.y.z or just simply an identifier
     * @param access is this token being lexed in an access statement instead of assignment
     * @param lexStruct also lex a struct if found -> StructName { v1, v2 }
     * @return
     */
    bool lexAccessChain(bool access, bool lexStruct = false);

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
     * lexes a single top level statement, top level means in file scope, These include
     * functions, structs, interfaces, implementations, enum, annotations
     * comments, variable initialization with value, constants
     * @return
     */
    bool lexTopLevelStatementTokens();

    /**
     * lexes a single nested level statement, nested level means not top level (must not be in file scope)
     * These exclude functions, enum, structs, interfaces, implementations in nested scopes
     */
    bool lexNestedLevelStatementTokens();

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
     * All top levels statements lexed, These include
     * functions, structs, interfaces, implementations
     * comments, variable initialization with value, constants
     */
    void lexTopLevelMultipleStatementsTokens();

    /**
     * lexes a multiple nested level statement, nested level means not top level (must not be in file scope)
     * These exclude functions, enum, structs, interfaces, implementations in nested scopes
     */
    void lexNestedLevelMultipleStatementsTokens();

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
    bool lexBraceBlock(const std::string &forThing = "");

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
     * @param allow_declaration allows a declaration, without body of the function that is
     * @return
     */
    bool lexFunctionStructureTokens(bool allow_declaration = false);

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
     * this will try to collect current struct as a lexer
     * @param start is the start position inside the tokens vector
     */
    bool collectStructAsLexer(unsigned int start, unsigned int end);

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
    bool lexAnnotationMacro();

    /**
     * lexes a bool, true or false
     * @return whether a bool has been lexed
     */
    bool lexBoolToken();

    /**
      * lex a unsigned int as number token
      */
    bool lexUnsignedIntAsNumberToken();

    /**
     * lex an number token
     * @return whether a token was lexed or not
     */
    bool lexNumberToken();

    /**
     * lexes tokens for a complete struct object initialization
     * @return
     */
    bool lexStructValueTokens();

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
    bool lexAccessChainOrValue(bool lexStruct = false);

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
    bool lexExpressionTokens(bool lexStruct = false);

    /**
     * check if there's a new line at current position
     * @return true if there's a newline otherwise false
     */
    inline bool hasNewLine() {
        return provider.hasNewLine();
    }

    /**
     * All the chars that cause new line
     * for example \n \r
     * @return
     */
    inline bool lexNewLineChars() {
        return provider.readNewLineChars();
    }

    /**
     * adds an error at the current stream position (which is the end), starting from the last token's end,
     * @param position the position (in the tokens vector) of the token at end of which error started
     * @param message the message for the error
     */
    void diagnostic(Position start, const std::string &message, DiagSeverity severity);

    /**
     * This just calls the diagnostic method above giving it the position
     */
    inline void diagnostic(unsigned int position, const std::string &message, DiagSeverity severity) {
        auto token = tokens[position].get();
        auto &pos = token->position;
        diagnostic({pos.line, pos.character + token->length()}, message, severity);
    }

    /**
     * This just calls the diagnostic method above
     * It has a range, so it will start at the last token that was consumed
     * and up until the current stream position
     * @param message
     * @param severity
     */
    inline void diagnostic(const std::string &message, DiagSeverity severity) {
        diagnostic(tokens.size() - 1, message + " got \"" + tokens[tokens.size() - 1]->representation() + "\"",
                   severity);
    }

    /**
     * This just calls the diagnostic method above with DiagSeverity::Warning
     */
    inline void warning(const std::string &message) {
        diagnostic(message, DiagSeverity::Warning);
    }

    /**
     * This just calls the diagnostic method above with DiagSeverity::Information
     */
    inline void info(const std::string &message) {
        diagnostic(message, DiagSeverity::Information);
    }

    /**
     * This just calls the diagnostic method above with DiagSeverity::Hint
     */
    inline void hint(const std::string &message) {
        diagnostic(message, DiagSeverity::Hint);
    }

    /**
     * This just calls the diagnostic method above with DiagSeverity::Error
     */
    inline void error(const std::string &message) {
        diagnostic(message, DiagSeverity::Error);
    }

    /**
     * returns the token position at the very current position
     * @return
     */
    inline Position position() {
        return provider.position();
    }

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
    inline Position backPosition(unsigned int back) {
        return provider.backPosition(back);
    }

    /**
     * gets the line number from the provider
     * @return
     */
    inline unsigned int lineNumber() {
        return provider.getLineNumber();
    }

protected:

    /**
     * called by constructor
     */
    void init_annotation_modifiers();

    /**
     * collected token modifiers, when annotation modifiers like deprecated are detected
     * they set bits into this unsigned int, which will be later collected by a declaration like
     * struct / function
     */
    unsigned int modifiers = 0;

    /**
     * just a map between annotations and their functions, for example
     * key:deprecated -> value:{ modifiers = deprecated; }
     */
    std::unordered_map<std::string, std::function<void(Lexer *lexer)>> annotation_modifiers;

    /**
     * structs declared as lexer by the user
     */
    std::unordered_map<std::string, std::unique_ptr<StructDefinition>> lexer_structs;

    /**
     * collected nodes that should be destroyed when the lexer is destroyed
     */
    std::unordered_map<std::string, std::unique_ptr<ASTNode>> collected;

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
     * when true, a struct will be lexed as a lexer, which will defines
     * a lexer implementation, that lexes tokens found in preprocess directives
     */
    bool isLexCompTimeLexer = false;

    /**
     * when true, a struct / function will be lexed as lexer scoped
     * which means, it will be parsed at lex time
     */
    bool isLexerScoped = false;

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