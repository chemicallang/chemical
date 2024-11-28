// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include <utility>
#include <vector>
#include "stream/SourceProvider.h"
#include "cst/base/CSTUnit.h"
#include "parser/model/tokens/NumberToken.h"
#include "ast/utils/Operation.h"
#include "integration/common/Diagnostic.h"
#include <memory>
#include <optional>
#include "utils/fwd/functional.h"
#include "model/CompilerBinder.h"
#include "integration/cbi/bindings/CBI.h"
#include "cst/base/CSTDiagnoser.h"
#include "lexer/Token.h"
#include "ast/base/ASTAllocator.h"
#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"
#include "ast/base/BaseType.h"

class CompilerBinder;

class Parser;

/**
 * A function that is called upon encountering an annotation
 */
typedef void(*AnnotationModifierFn)(Parser *lexer, CSTToken* token);

/**
 * a value creator function
 */
typedef Value*(*ValueCreatorFn)(Parser *lexer, ASTAllocator& allocator, Token* token);

/**
 * a function that parses something inside a macro
 */
typedef void(*MacroLexerFn)(Parser *lexer);

/**
 * the parser that is used to parse
 */
class Parser : public CSTDiagnoser {
public:

    /**
     * stored for debugging
     * TODO remove this
     */
    std::string_view stored_file_path;

    /**
     * The file id to use with location manager
     */
    unsigned int file_id;

    /**
     * the current token
     */
    Token* token;

    /**
     * we save this to reset the parser
     */
    Token* beginning_token;

    /**
     * the location manager is used to encode locations
     */
    LocationManager& loc_man;

    /**
     * a unit contains everything that's allocated
     */
    CSTUnit unit;

    /**
     * the binder that will be used to compile binding code
     * if not present, cbi is considered disabled
     */
    CompilerBinder* const binder;

    /**
     * the global allocator for the job
     */
    ASTAllocator& global_allocator;

    /**
     * the module allocator for the job
     */
    ASTAllocator& mod_allocator;

    /**
     * the ast we're generating, is it for a 64 bit target
     */
    bool is64Bit;

    /**
     * initialize the lexer with this provider and path
     */
    Parser(
        unsigned int file_id,
        std::string_view file_path,
        Token* start_token,
        LocationManager& loc_man,
        ASTAllocator& global_allocator,
        ASTAllocator& mod_allocator,
        bool is64Bit,
        CompilerBinder* binder = nullptr
    );

    /**
     * get the file path
     */
    std::string_view file_path();

    /**
     * get a encoded location
     */
    uint64_t loc(Position& start, Position& end);

    /**
     * get location for a single token
     */
    uint64_t loc_single(Token* t);

    /**
     * lex everything to LexTokens, tokens go into 'tokens' member property
     */
    void lex();

    /**
     * reset the lexer, for re-lexing a new file, if it has lexed a file before
     */
    void reset();

    /**
     * get current tokens size
     */
    inline size_t tokens_size() {
        return unit.tokens.size();
    }

    // ------------- Functions exposed to chemical begin here

    /**
     * parses the token of type
     * will skip new line, comment token, multi line comment tokens to check for this
     */
    Token* consumeOfType(enum TokenType type);

    /**
     * check if given token type is a keyword
     */
    static inline bool isKeyword(enum TokenType type) {
        return type > TokenType::IndexKwStart && type < TokenType::IndexKwEnd;
    }

    /**
     * consume a identifier or keyword at the current location
     */
    Token* consumeIdentifierOrKeyword() {
        auto& t = *token;
        const auto type = t.type;
        if(type == TokenType::Identifier || isKeyword(type)) {
            token++;
            return &t;
        } else {
            return nullptr;
        }
    }

    /**
     * consume strictly a single identifier
     */
    Token* consumeIdentifier() {
        auto& t = *token;
        if(t.type == TokenType::Identifier) {
            token++;
            return &t;
        } else {
            return nullptr;
        }
    }

    /**
     * store the token as a identifier cst token
     * @deprecated
     */
    inline void storeIdentifier(Token* token) {
        emplace(LexTokenType::Identifier, token->position, std::string(token->value));
    }

    /**
     * store the token as a variable cst token
     * @deprecated
     */
    inline void storeVariable(Token* token) {
        emplace(LexTokenType::Variable, token->position, std::string(token->value));
    }

    /**
     * parses a variable otherwise returns nullptr
     */
    Value* parseVariableIdentifier(ASTAllocator& allocator);

    /**
     * lex a variable token into tokens until the until character occurs
     * only lexes the token if the identifier is not empty
     */
    bool lexVariableToken();

    /**
     * lex an identifier token into tokens until the until character occurs
     * only lexes the token if the identifier is not empty
     */
    bool lexIdentifierToken();

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
    bool lexAccessChain(bool lexStruct = false, bool lex_as_node = false);

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
    bool lexVarInitializationTokens(unsigned start, bool allowDeclarations = true, bool requiredType = false);

    /**
     * a helper function
     */
    bool lexVarInitializationTokens(bool allowDeclarations = true, bool requiredType = false) {
        return lexVarInitializationTokens(tokens_size(), allowDeclarations, requiredType);
    }

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
    bool lexTypeId(Token* type, unsigned int start);

    /**
     * lex type tokens
     */
    bool lexTypeTokens();

    /**
     * top level access specified declarations
     */
    bool lexTopLevelAccessSpecifiedDecls();

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
    bool lexNestedLevelStatementTokens(bool is_value = false, bool lex_value_node = false);

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
    bool lexOperatorToken(enum TokenType type);

    /**
     * store an operation token
     */
    void storeOperationToken(Token* token, Operation op);

    /**
     * lexes the given operator as length 1 character operator token
     * @return whether the token was found
     */
    bool lexOperationToken(enum TokenType type, Operation op);

    /**
     * lexes a keyword token for the given keyword
     * @return  whether the keyword was found
     */
    bool lexKeywordToken(enum TokenType type);

    /**
     * lexes a keyword token, after which whitespace is present
     */
    bool lexWSKeywordToken(enum TokenType type);

    /**
     * lex a whitespaced keyword token, which may end at the given character if not whitespace
     */
    bool lexWSKeywordToken(enum TokenType type, enum TokenType may_end_at);

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
    void lexNestedLevelMultipleStatementsTokens(bool is_value = false, bool lex_value_node = false);

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
    bool lexBraceBlock(const std::string &forThing, void(*nested_lexer)(Parser*));

    /**
     * lexes top level brace block
     */
    bool lexTopLevelBraceBlock(const std::string& forThing) {
        return lexBraceBlock(forThing, [](Parser* lexer){
            lexer->lexTopLevelMultipleStatementsTokens(true);
        });
    }

    /**
     * lexes a brace block, { statement(s) }
     */
    bool lexBraceBlock(const std::string &forThing = "");

    /**
     * lexes a brace block or a value
     */
    bool lexBraceBlockOrSingleStmt(const std::string &forThing, bool is_value, bool lex_value_node);

    /**
     * lexes import identifier list example : { something, something }
     */
    bool lexImportIdentifierList();

    /**
     * lexes import statement
     */
    bool lexImportStatement();

    /**
     * lexes a single delete statement
     */
    bool lexDestructStatement();

    /**
     * lexes return statement
     */
    bool lexReturnStatement();

    /**
     * lexes a constructor init block, this is only present in functions
     */
    bool lexConstructorInitBlock();

    /**
     * lexes an unsafe block
     */
    bool lexUnsafeBlock();

    /**
     * lexes break statement
     */
    bool lexBreakStatement();

    /**
     * lexes a unreachable statement
     */
    bool lexUnreachableStatement();

    /**
     * lexes a single typealias statement
     */
    bool lexTypealiasStatement(unsigned start);

    /**
     * a helper function
     */
    bool lexTypealiasStatement() {
        return lexTypealiasStatement(tokens_size());
    }

    /**
     * lexes continue statement
     */
    bool lexContinueStatement();

    /**
     * lexes a single if expr and the body without else if or else
     * meaning '(' expr ')' '{' body '}'
     */
    bool lexIfExprAndBlock(unsigned start, bool is_value, bool lex_value_node, bool top_level);

    /**
     * lex if block
     */
    bool lexIfBlockTokens(bool is_value, bool lex_value_node, bool top_level);

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
     * lex loop block tokens
     */
    bool lexLoopBlockTokens(bool is_value);

    /**
     * lex parameter list
     * @return true when no errors occurred
     */
    bool lexParameterList(bool optionalTypes = false, bool defValues = true, bool lexImplicitParams = true, bool variadicParam = true);

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
    bool lexFunctionStructureTokens(unsigned start, bool allow_declaration = false, bool allow_extensions = false);

    /**
     * a helper function
     */
    bool lexFunctionStructureTokens(bool allow_declaration = false, bool allow_extensions = false) {
        return lexFunctionStructureTokens(tokens_size(), allow_declaration, allow_extensions);
    }

    /**
     * lexes interface block, this means { member(s) }
     * without the `interface` keyword and name identifier
     */
    void lexInterfaceBlockTokens();

    /**
     * lexes a interface structure
     */
    bool lexInterfaceStructureTokens(unsigned start);

    /**
     * a helper function
     */
    bool lexInterfaceStructureTokens() {
        return lexInterfaceStructureTokens(tokens_size());
    }

    /**
     * lex namespace tokens
     */
    bool lexNamespaceTokens(unsigned start);

    /**
     * a helper function
     */
    bool lexNamespaceTokens() {
        return lexNamespaceTokens(tokens_size());
    }

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
    bool lexStructStructureTokens(unsigned start, bool unnamed = false, bool direct_init = false);

    /**
     * a helper function
     */
    bool lexStructStructureTokens(bool unnamed = false, bool direct_init = false) {
        return lexStructStructureTokens(tokens_size(), unnamed, direct_init);
    }

    /**
     * lexes a single member of the struct
     */
    bool lexVariantMemberTokens();

    /**
     * lexes struct block, this means { member(s) }
     * without the `struct` keyword and name identifier
     */
    void lexVariantBlockTokens();

    /**
     * lexes a struct block
     */
    bool lexVariantStructureTokens(unsigned start);

    /**
     * a helper function
     */
    bool lexVariantStructureTokens() {
        return lexVariantStructureTokens(tokens_size());
    }

    /**
     * lexes struct block, this means { member(s) }
     * without the `struct` keyword and name identifier
     */
    void lexUnionBlockTokens();

    /**
     * lexes a struct block
     */
    bool lexUnionStructureTokens(unsigned start, bool unnamed = false, bool direct_init = false);

    /**
     * a helper function
     */
    bool lexUnionStructureTokens(bool unnamed = false, bool direct_init = false) {
        return lexUnionStructureTokens(tokens_size(), unnamed, direct_init);
    }

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
    bool lexEnumStructureTokens(unsigned start);

    /**
     * helper function
     */
    bool lexEnumStructureTokens() {
        return lexEnumStructureTokens(tokens_size());
    }

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
    void lexWhitespaceAndNewLines();

    /**
     * parses a single string value using the given allocator
     */
    Value* parseStringValue(ASTAllocator& allocator);

    /**
     * store direct value ast cst token
     */
    bool straight_data(LexTokenType type, ASTAny* value) {
        auto pos = token->position;
        if(value) {
            emplace(type, value, pos);
            return true;
        } else {
            return false;
        }
    }

    inline bool straight_value(Value* value) {
        return straight_data(LexTokenType::StraightValue, value);
    }

    inline bool straight_type(BaseType* type) {
        return straight_data(LexTokenType::StraightType, type);
    }

    inline bool straight_node(ASTNode* node) {
        return straight_data(LexTokenType::StraightNode, node);
    }

    /**
     * lexes a string token, string is enclosed inside double quotes
     * @return whether a string has been lexed
     * @deprecated
     */
    inline bool lexStringToken() {
        return straight_value(parseStringValue(global_allocator));
    }

    /**
     * a single character value is parsed
     */
    Value* parseCharValue(ASTAllocator& allocator);

    /**
     * lexes a char token, char is enclosed inside single quotes
     * @return whether a char has been lexed
     * @deprecated
     */
    inline bool lexCharToken() {
        return straight_value(parseCharValue(global_allocator));
    }

    /**
     * lex hash macro
     */
    bool lexAnnotationMacro();

    /**
     * parses the null value, otherwise returns nullptr
     */
    Value* parseNull(ASTAllocator& allocator);

    /**
     * lexes a null value
     * @deprecated
     */
    bool lexNull() {
        return straight_value(parseNull(global_allocator));
    }

    /**
     * parses a bool value otherwise returns nullptr
     */
    Value* parseBoolValue(ASTAllocator& allocator);

    /**
     * lexes a bool, true or false
     * @return whether a bool has been lexed
     * @deprecated
     */
    bool lexBoolToken() {
        return straight_value(parseBoolValue(global_allocator));
    }

    /**
      * lex a unsigned int as number token
      */
    bool lexUnsignedIntAsNumberToken();

    /**
     * parses a number value
     */
    Value* parseNumberValue(ASTAllocator& allocator);

    /**
     * lex an number token
     * @return whether a token was lexed or not
     * @deprecated
     */
    bool lexNumberToken() {
        return straight_value(parseNumberValue(global_allocator));
    }

    /**
     * lexes tokens for a complete struct object initialization
     */
    bool lexStructValueTokens(unsigned back_start);

    /**
     * lexes value tokens like integer, string
     */
    bool lexValueToken();

    /**
     * lexes a value token for switch
     */
    bool lexConstantValue();

    /**
     * lexes multiple switch case values separated with '|'
     */
    bool lexMultipleSwitchCaseValues();

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
     * a value is lexed but as a node
     */
    bool lexValueNode();

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
     * @return true if there are no errors
     */
    bool lexLambdaAfterParamsList(unsigned int start);

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
    bool lexParenExpressionAfterLParen();

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
    bool lexSwitchStatementBlock(bool is_value = false, bool lex_value_node = false);

    /**
     * lexes try catch block statements
     */
    bool lexTryCatchTokens();

    /**
     * lex using statement
     */
    bool lexUsingStatement();

    /**
     * lex provide statement
     */
    bool lexProvideStatement();

    /**
     * lexes a comptime block
     */
    bool lexComptimeBlock();

    // -------------------------------- Exposed till here

    /**
     * emplaces the token at the end
     */
    template<typename... Args>
    inline constexpr void emplace(Args&&... args) {
        unit.emplace(std::forward<Args>(args)...);
    }

    /**
     * emplaces the token at the end
     */
    inline constexpr void emplace(LexTokenType type, const Position& position, const std::string_view& view) {
        unit.emplace(type, position, view);
    }

    /**
     * takes elements (by removing) from tokens vector, starting from start position, till end
     */
    void take_from(std::vector<CSTToken*>& slice, unsigned int start, unsigned int end) {
        unsigned size = end - start;
        slice.reserve(size);
        unsigned i = start;
        while(i < end) {
            slice.push_back(unit.tokens[i]);
            i++;
        }
        auto begin = unit.tokens.begin() + start;
        unit.tokens.erase(begin, begin + size);
    }

    /**
     * put tokens in a compound token of specified type, starting from start
     * @deprecated
     */
    void compound_from(unsigned int start, LexTokenType type) {
        std::vector<CSTToken*> slice;
        take_from(slice, start, tokens_size());
        unit.emplace_compound(type);
        unit.tokens.back()->tokens = std::move(slice);
    }

    /**
     * All the chars that cause new line
     * for example \n \r
     */
    inline bool lexNewLineChars() {
        if(token->type == TokenType::NewLine) {
            token++;
            return true;
        } else {
            return false;
        }
    }

    /**
     * make a diagnostic with given parameters, for the current file
     */
    Diag make_diag(Position start, const std::string &message, DiagSeverity severity) {
        return CSTDiagnoser::make_diag(message, file_path(), start, token->position, severity);
    }

    /**
     * adds an error at the current stream position (which is the end), starting from the last token's end,
     * @param position the position (in the tokens vector) of the token at end of which error started
     * @param message the message for the error
     */
    void diagnostic(Position start, const std::string_view &message, DiagSeverity severity) {
        add_diag(CSTDiagnoser::make_diag(message, file_path(), start, token->position, severity));
    }

    /**
     * get the last token from the unit
     */
    inline CSTToken* unit_last_token() {
        return unit.tokens[unit.tokens.size() - 1];
    }

    /**
     * This just calls the diagnostic method above giving it the position
     */
    inline void diagnostic(CSTToken* token, const std::string &message, DiagSeverity severity) {
        auto &pos = token->position();
        diagnostic({pos.line, pos.character + token->length()}, message, severity);
    }

    /**
     * This just calls the diagnostic method above
     * It has a range, so it will start at the last token that was consumed
     * and up until the current stream position
     */
    inline void diagnostic(const std::string &message, DiagSeverity severity) {
        std::string rep;
        auto token = unit_last_token();
        token->end_token()->append_representation(rep);
        diagnostic(token->start_token(), message + " got \"" + rep + "\"", severity);
    }

    /**
     * a helper function
     */
    inline void diagnostic(std::string& message, DiagSeverity severity) {
        CSTDiagnoser::diagnostic(message, file_path(), unit_last_token()->end_token(), severity);
    }

    /**
     * a helper function
     */
    inline void diagnostic(std::string_view& message, DiagSeverity severity) {
        CSTDiagnoser::diagnostic(message, file_path(), unit_last_token()->end_token(), severity);
    }

    /**
     * This just calls the diagnostic method above with DiagSeverity::Warning
     */
    inline void warning(std::string &message) {
        diagnostic(message, DiagSeverity::Warning);
    }

    /**
     * This just calls the diagnostic method above with DiagSeverity::Information
     */
    inline void info(std::string &message) {
        diagnostic(message, DiagSeverity::Information);
    }

    /**
     * This just calls the diagnostic method above with DiagSeverity::Hint
     */
    inline void hint(std::string &message) {
        diagnostic(message, DiagSeverity::Hint);
    }

    /**
     * This just calls the diagnostic method above with DiagSeverity::Error
     */
    inline void error(std::string &message) {
        diagnostic(message, DiagSeverity::Error);
    }

    /**
     * This just calls the diagnostic method above with DiagSeverity::Warning
     */
    inline void warning(std::string_view message) {
        diagnostic(message, DiagSeverity::Warning);
    }

    /**
     * This just calls the diagnostic method above with DiagSeverity::Information
     */
    inline void info(std::string_view message) {
        diagnostic(message, DiagSeverity::Information);
    }

    /**
     * This just calls the diagnostic method above with DiagSeverity::Hint
     */
    inline void hint(std::string_view message) {
        diagnostic(message, DiagSeverity::Hint);
    }

    /**
     * This just calls the diagnostic method above with DiagSeverity::Error
     */
    inline void error(std::string_view message) {
        diagnostic(message, DiagSeverity::Error);
    }

    /**
     * save error at given error token position
     */
    inline void error(std::string_view message, Token* err_token) {
        diagnostic(err_token->position, message, DiagSeverity::Error);
    }

    /**
     * malformed value
     */
    void mal_value(unsigned start, std::string& message) {
        diagnostic(message, DiagSeverity::Error);
        compound_from(start, LexTokenType::CompMalformedValue);
    }

    /**
     * malformed node
     */
    void mal_node(unsigned start, std::string& message) {
        diagnostic(message, DiagSeverity::Error);
        compound_from(start, LexTokenType::CompMalformedNode);
    }

    /**
     * malformed type
     */
    void mal_type(unsigned start, std::string& message) {
        diagnostic(message, DiagSeverity::Error);
        compound_from(start, LexTokenType::CompMalformedType);
    }

    /**
     * malformed value
     */
    void mal_value(unsigned start, std::string_view message) {
        diagnostic(message, DiagSeverity::Error);
        compound_from(start, LexTokenType::CompMalformedValue);
    }

    /**
     * malformed node
     */
    void mal_node(unsigned start, std::string_view message) {
        diagnostic(message, DiagSeverity::Error);
        compound_from(start, LexTokenType::CompMalformedNode);
    }

    /**
     * malformed type
     */
    void mal_type(unsigned start, std::string_view message) {
        diagnostic(message, DiagSeverity::Error);
        compound_from(start, LexTokenType::CompMalformedType);
    }

    /**
     * malformed node
     */
    void mal_value_or_node(unsigned start, std::string_view message, bool is_value) {
        diagnostic(message, DiagSeverity::Error);
        compound_from(start, is_value ? LexTokenType::CompMalformedValue : LexTokenType::CompMalformedNode);
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
     * when true, init block in lexed
     */
    bool isLexInitBlock = false;

    /**
     * when true, import statements will be lexed
     */
    bool isLexImportStatement = true;

};