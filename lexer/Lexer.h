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
#include "integration/common/Diagnostic.h"
#include "ast/base/BaseType.h"
#include "ast/structures/StructDefinition.h"
#include "cst/base/CompoundCSTToken.h"
#include <memory>
#include <optional>
#include "utils/fwd/functional.h"
#include "model/CompilerBinder.h"
#include "cst/base/CSTDiagnoser.h"

/**
 * A function that is called upon encountering an annotation
 */
typedef void(*AnnotationModifierFn)(Lexer *lexer, CSTToken* token);

/**
 * a value creator function
 */
typedef void(*ValueCreatorFn)(Lexer *lexer);

/**
 * a function that lexes something inside a macro
 */
typedef void(*MacroLexerFn)(Lexer *lexer);

class Lexer : public CSTDiagnoser {
public:

    /**
     * provides access to the source code to lex
     */
    SourceProvider &provider;

    /**
     * tokens are tokens that have been lexed from source code
     */
    std::vector<std::unique_ptr<CSTToken>> tokens;

    /**
     * the binder that will be used to compile binding code
     * if not present, cbi is considered disabled
     */
    CompilerBinder* binder;

    /**
     * a pointer to lexer cbi that will be passed to cbi functions
     */
    LexerCBI* cbi;

    /**
     * the cbi used for collection
     */
    std::string current_cbi;

    /**
     * when a struct / function is to be collected by cbi
     */
    bool isCBICollecting = false;

    /**
     * when a struct / function is to be collected by cbi globally
     */
    bool isCBICollectingGlobal = false;

    /**
     * when true, cbi won't be turned off after a single node has been collected
     */
    bool isCBIKeepCollecting = false;

    /**
     * initialize the lexer with this provider and path
     */
    Lexer(
        SourceProvider &provider,
        CompilerBinder* binder = nullptr,
        LexerCBI* cbi = nullptr
    );

    /**
     * lex everything to LexTokens, tokens go into 'tokens' member property
     */
    void lex();

    /**
     * reset the lexer, for re-lexing a new file, if it has lexed a file before
     */
    void reset();

    /**
     * lexes a number as a string
     */
    inline std::string lexNumber() {
        return provider.readNumber();
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
     */
    inline std::string lexAlphaNum() {
        return provider.readAlphaNum();
    }

    /**
     * lexes an identifier
     * it doesn't add it as a token use lex Identifier token for that
     */
    inline std::string lexIdentifier() {
        return provider.readIdentifier();
    }

    // ------------- Functions exposed to chemical begin here

    /**
     * consumes a identifier and store as a variable token
     * @return true if identifier is not empty, false if it is
     */
    bool storeVariable(const std::string &identifier);

    /**
     * consumes a identifier and store as an identifier token
     * @return true if identifier is not empty, false if it is
     */
    bool storeIdentifier(const std::string &identifier);

    /**
     * lex a variable token into tokens until the until character occurs
     * only lexes the token if the identifier is not empty
     */
    inline bool lexVariableToken() {
        return storeVariable(lexIdentifier());
    }

    /**
     * lex an identifier token into tokens until the until character occurs
     * only lexes the token if the identifier is not empty
     */
    inline bool lexIdentifierToken() {
        return storeIdentifier(lexIdentifier());
    }

    /**
     * it will lex generic args list, it should be called after the '<'
     * after this function a '>' should be lexed as well, and then
     * compound it into a generic args list
     */
    void lexGenericArgsList();

    /**
     * this will compound the generic args list
     * It expects '<' and then generic args and then '>'
     */
    bool lexGenericArgsListCompound();

    /**
     * lexes a function call, after the '<' for generic start
     */
    void lexFunctionCallWithGenericArgsList();

    /**
     * lexes a function call, that is args ')' without function name
     */
    bool lexFunctionCall(unsigned back_start);

    /**
     * lexes a keyword access specifier public, private, internal & (if protect is true, then protected)
     */
    bool lexAccessSpecifier(bool internal = true, bool protect = false);

    /**
     * after an identifier has been consumed
     * we call this method to lex an access chain after it
     * identifier .element1.element2.element3
     * this is the method called by lexAccessChain after finding a identifier
     * @param assChain is the access chain in an assignment
     */
    bool lexAccessChainAfterId(bool lexStruct = false, unsigned int chain_length = 1);

    /**
     * this method does not compound the access chain, so can be called recursively
     * this method is called by lexAccessChain to not compound access chains nested in it
     * @param assChain is the access chain in an assignment
     */
    bool lexAccessChainRecursive(bool lexStruct = false, unsigned int chain_length = 0);

    /**
     * this lexes an access chain like x.y.z or just simply an identifier
     * @param assChain is the access chain in an assignment
     * @param lexStruct also lex a struct if found -> StructName { v1, v2 }
     */
    bool lexAccessChain(bool lexStruct = false);

    /**
     * it lexes a access chain, but allows a '&' operator before it to get the address of value
     * so this allows a.b.c or &a.b.c
     */
    bool lexAccessChainOrAddrOf(bool lexStruct = false);

    /**
     * lex allowDeclarations or initialization tokens
     * like var x : int; or var x : int = 5;
     * @param allowDeclarations when true, it will allow allowDeclarations only without the value initialization
     * like #var x : int; when false however, it'll be strict initialization
     * @return whether it was able to lex the tokens for the statement
     */
    bool lexVarInitializationTokens(bool allowDeclarations = true, bool requiredType = false);

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
     * this is invoked by expressions , when user types sum < identifier, it can mean two things
     * 1 - is sum less than identifier  (expression)
     * 2 - sum < identifier >  (generic)
     * when we encounter a less than sign, we call this function to check if there's a generic end (>) ahead
     */
    bool isGenericEndAhead();

    /**
     * This lexes a operation token before assignment '='
     * for example +=, -=
     * in this case, equal sign is ignored and operation is determined solely based on the token before it
     * @return whether the language operator token has been lexed
     */
    bool lexAssignmentOperatorToken();

    /**
     * lex lambda type tokens
     */
    bool lexLambdaTypeTokens(unsigned int start);

    /**
     * will lex a generic type after identifier
     */
    bool lexGenericTypeAfterId(unsigned int start);

    /**
     * will lex a referenced or generic type
     */
    bool lexRefOrGenericType();

    /**
     * lex array and pointer types after type id
     */
    void lexArrayAndPointerTypesAfterTypeId(unsigned int start);

    /**
     * lex type id
     */
    bool lexTypeId(std::string& type, unsigned int start);

    /**
     * lex type tokens
     */
    bool lexTypeTokens();

    /**
     * lexes a single top level statement, top level means in file scope, These include
     * functions, structs, interfaces, implementations, enum, annotations
     * comments, variable initialization with value, constants
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
     * lex throw statement
     */
    bool lexThrowStatementTokens();

    /**
     * lexes the given operator as length 1 character operator token
     * @return whether the token was found
     */
    bool lexOperatorToken(char op);

    /**
     * lexes the given operator as a string operator token
     * @return whether the token was found
     */
    bool lexOperatorToken(const std::string &op);

    /**
     * store an operation token
     */
    void storeOperationToken(char token, Operation op);

    /**
     * lexes the given operator as length 1 character operator token
     * @return whether the token was found
     */
    bool lexOperationToken(char token, Operation op);

    /**
     * lexes the given operator as a string operator token
     * @return whether the token was found
     */
    bool lexOperatorToken(const std::string &token, Operation op);

    /**
     * lexes a keyword token for the given keyword
     * @return  whether the keyword was found
     */
    bool lexKeywordToken(const std::string &keyword);

    /**
     * lexes a whitespaces keyword, at least a single whitespace should be present after the keyword
     * @return  whether the keyword was found
     */
    bool lexWSKeywordToken(const std::string &keyword);

    /**
     * All top levels statements lexed, These include
     * functions, structs, interfaces, implementations
     * comments, variable initialization with value, constants
     */
    void lexTopLevelMultipleStatementsTokens(bool break_at_no_stmt = false);

    /**
     * All import statements defined at top level will be lexed
     * @param should cause error on invalid syntax, or stop
     */
    void lexTopLevelMultipleImportStatements();

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
    bool lexBraceBlock(const std::string &forThing, void(*nested_lexer)(Lexer*));

    /**
     * lexes a brace block, { statement(s) }
     */
    bool lexBraceBlock(const std::string &forThing = "") {
        return lexBraceBlock(forThing, [](Lexer* lexer){
            lexer->lexNestedLevelMultipleStatementsTokens();
        });
    }

    /**
     * lexes an expression for if statement including parens '(' expr ')'
     */
    void lexIfExpression();

    /**
     * lexes import identifier list example : { something, something }
     */
    bool lexImportIdentifierList();

    /**
     * lexes import statement
     */
    bool lexImportStatement();

    /**
     * lexes return statement
     */
    bool lexReturnStatement();

    /**
     * lexes break statement
     */
    bool lexBreakStatement();

    /**
     * lexes a single typealias statement
     */
    bool lexTypealiasStatement();

    /**
     * lexes continue statement
     */
    bool lexContinueStatement();

    /**
     * lexes a single if expr and the body without else if or else
     * meaning '(' expr ')' '{' body '}'
     */
    void lexIfExprAndBlock();

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
    void lexParameterList(bool optionalTypes = false, bool defValues = true);

    /**
    * lexes a function signature with parameters
    */
    bool lexFunctionSignatureTokens();

    /**
     * this occurs right after the function name
     */
    bool lexGenericParametersList();

    /**
     * lex after func keyword has been incremented
     */
    bool lexAfterFuncKeyword(bool allow_extensions = false);

    /**
     * lexes a function block with parameters
     * @param allow_declaration allows a declaration, without body of the function that is
     */
    bool lexFunctionStructureTokens(bool allow_declaration = false, bool allow_extensions = false);

    /**
     * lexes interface block, this means { member(s) }
     * without the `interface` keyword and name identifier
     */
    void lexInterfaceBlockTokens();

    /**
     * lexes a interface structure
     */
    bool lexInterfaceStructureTokens();

    /**
     * lex namespace tokens
     */
    bool lexNamespaceTokens();

    /**
     * lexes a single member of the struct
     */
    bool lexStructMemberTokens();

    /**
     * lexes struct block, this means { member(s) }
     * without the `struct` keyword and name identifier
     */
    void lexStructBlockTokens();

    /**
     * lexes a struct block
     */
    bool lexStructStructureTokens(bool unnamed = false, bool direct_init = false);

        /**
     * lexes a single member of the struct
     */
    bool lexUnionMemberTokens();

    /**
     * lexes struct block, this means { member(s) }
     * without the `struct` keyword and name identifier
     */
    void lexUnionBlockTokens();

    /**
     * lexes a struct block
     */
    bool lexUnionStructureTokens(bool unnamed = false, bool direct_init = false);

    /**
     * this will try to collect current struct as a lexer
     * @param start is the start position inside the tokens vector
     */
    bool collect_cbi_node(unsigned int start, unsigned int end);

    /**
     * lexes a impl block tokens
     */
    void lexImplBlockTokens();

    /**
     * lexes a impl block
     */
    bool lexImplTokens();

    /**
     * lexes an enum block, this means { enum(s) }
     * without the `enum` keyword and name identifier
     */
    bool lexEnumBlockTokens();

    /**
     * lexes a enum block
     */
    bool lexEnumStructureTokens();

    /**
     * reads whitespace at current position
     */
    bool readWhitespace();

    /**
     * lex whitespace tokens
     */
    bool lexWhitespaceToken();

    /**
     * a utility function to lex whitespace tokens and also skip new lines
     */
    inline void lexWhitespaceAndNewLines() {
        provider.readWhitespacesAndNewLines();
    }

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
     */
    bool lexAnnotationMacro();

    /**
     * lexes a null value
     */
    bool lexNull();

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
     */
    bool lexStructValueTokens(unsigned back_start);

    /**
     * lexes value tokens like integer, string
     */
    bool lexValueToken();

    /**
     * values like integer and string, but appearing in access chain
     */
    bool lexAccessChainValueToken();

    /**
     * lexes array syntax values like [1,2,3,4]
     * for easy array creation
     */
    bool lexArrayInit();

    /**
     * lexes access chain like x.y.z or a value like 10, could be int, string, char
     */
    bool lexAccessChainOrValue(bool lexStruct = false);

    /**
     * lexes a identifier list like id1,id2
     */
    void lexTypeList();

    /**
     * lexes a identifier list like id1,id2
     */
    void lexIdentifierList();

    /**
     * lex lambda after params list
     */
    void lexLambdaAfterParamsList(unsigned int start);

    /**
     * lexes a single lambda function (PARAM1, PARAM2)[CAP1, CAP2] => {}
     */
    bool lexLambdaValue();

    /**
     * lexes remaining expression, this is used by lexExpressionTokens
     * this lexes the expression tokens after the first identifier / value
     * for example in expression a + b, after lexing a + b will lexed by this function
     * @param start is the start of the expression, index in tokens vector !
     */
    bool lexRemainingExpression(unsigned start);

    /**
     * it will lex a lambda meaning '() => {}' in a paren expression
     * it assumes you've already consumed '('
     */
    bool lexLambdaOrExprAfterLParen();

    /**
     * it will lex a paren expression, meaning '(' expr ')'
     * it assumes you've already consumed '('
     */
    void lexParenExpressionAfterLParen();

    /**
     * lex a parenthesized expression '(x + 5)'
     */
    bool lexParenExpression();

    /**
     * lexes an expression token which can contain access chain and values
     * @return whether an expression has been lexed, the expression can also be a single identifier or value
     */
    bool lexExpressionTokens(bool lexStruct = false, bool lambda = true);

    /**
     * lexes switch block
     */
    bool lexSwitchStatementBlock();

    /**
     * lexes try catch block statements
     */
    bool lexTryCatchTokens();

    /**
     * lex using statement
     */
    bool lexUsingStatement();

    // -------------------------------- Exposed till here

    /**
     * takes elements (by removing) from tokens vector, starting from start position, till end
     */
    std::vector<std::unique_ptr<CSTToken>> take_from(unsigned int start, unsigned int end) {
        unsigned size = end - start;
        std::vector<std::unique_ptr<CSTToken>> slice;
        slice.reserve(size);
        unsigned i = start;
        while(i < end) {
            slice.push_back(std::move(tokens[i]));
            i++;
        }
        auto begin = tokens.begin() + start;
        tokens.erase(begin, begin + size);
        return slice;
    }

    /**
     * put tokens in a compound token of specified type, starting from start
     */
    void compound_from(unsigned int start, LexTokenType type) {
        tokens.emplace_back(std::make_unique<CompoundCSTToken>(take_from(start, tokens.size()), type));
    }

    /**
     * a helper function to collect
     */
    void compound_collectable(unsigned int start, LexTokenType type) {
        unsigned int size = tokens.size();
        tokens.emplace_back(std::make_unique<CompoundCSTToken>(take_from(start, size), type));
        if(isCBICollecting) {
            collect_cbi_node(start, tokens.size());
            if(!isCBIKeepCollecting) {
                isCBICollecting = false;
                isCBICollectingGlobal = false;
            }
        }
    }

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
        auto token = tokens[position]->start_token();
        auto &pos = token->position;
        diagnostic({pos.line, pos.character + token->length()}, message, severity);
    }

    /**
     * This just calls the diagnostic method above
     * It has a range, so it will start at the last token that was consumed
     * and up until the current stream position
     */
    inline void diagnostic(const std::string &message, DiagSeverity severity) {
        std::string rep;
        tokens[tokens.size() - 1]->end_token()->append_representation(rep);
        diagnostic(tokens.size() - 1, message + " got \"" + rep + "\"",
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
     */
    inline Position backPosition(unsigned int back) {
        return provider.backPosition(back);
    }

    /**
     * gets the line number from the provider
     */
    inline unsigned int lineNumber() {
        return provider.getLineNumber();
    }

protected:

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