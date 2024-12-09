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
#include "ast/base/Annotation.h"
#include "ast/base/LocatedIdentifier.h"
#include "cst/utils/ValueAndOperatorStack.h"

class CompilerBinder;

class Parser;

class GlobalInterpretScope;

/**
 * A function that is called upon encountering an annotation
 */
typedef void(*AnnotationModifierFunc)(Parser *parser, AnnotableNode* node);

/**
 * A function that is called upon encountering an annotation
 * @deprecated
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
     * interpret scope can be used to evaluate expressions during parsing
     */
    GlobalInterpretScope& comptime_scope;

    /**
     * the ast we're generating, is it for a 64 bit target
     */
    bool is64Bit;

    /**
     * these are annotation modifier functions that will be called on the next node
     */
    std::vector<AnnotationModifierFunc> annotations;

    /**
     * current parent node
     */
    ASTNode* parent_node = nullptr;

    /**
     * current function type
     */
    FunctionType* current_func_type = nullptr;

    /**
     * current loop node is stored here
     */
    LoopASTNode* current_loop_node = nullptr;

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
        GlobalInterpretScope& scope,
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
    uint64_t loc(const Position& start, const Position& end);

    /**
     * get a located identifier
     */
    LocatedIdentifier loc_id(const std::string_view& value, const Position& pos);

    /**
     * get a located identifier
     */
    inline LocatedIdentifier loc_id(Token* token) {
        return loc_id(token->value, token->position);
    }

    /**
     * get the ending position of the token
     */
    Position end_pos(Token* token);

    /**
     * get a encoded location
     */
    inline uint64_t loc(Token* start, Token* end) {
        return loc(start->position, end->position);
    }

    /**
     * get a location single at the position
     */
    uint64_t loc_single(Position& position, unsigned int length);

    /**
     * get location for a single token that is on the same line
     */
    inline uint64_t loc_single(Token* t) {
        return loc_single(t->position, t->value.size());
    }

    /**
     * suppose to be called on a node which can take annotations
     */
    void annotate(AnnotableNode* node) {
        for(auto annot : annotations) {
            annot(this, node);
        }
        annotations.clear();
    }

    /**
     * parses nodes into the given vector
     */
    void parse(std::vector<ASTNode*>& nodes);

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

public:

    /**
     * consume the current token if it's given type
     */
    bool consumeToken(enum TokenType type);

    /**
     * parses the token of type
     * will skip new line, comment token, multi line comment tokens to check for this
     */
    Token* consumeOfType(enum TokenType type);

    /**
     * parses the token of type
     * will skip new line, comment token, multi line comment tokens to check for this
     */
    Token* consumeWSOfType(enum TokenType type);

    /**
     * consume a identifier or keyword at the current location
     */
    Token* consumeIdentifierOrKeyword() {
        auto& t = *token;
        const auto type = t.type;
        if(type == TokenType::Identifier || Token::isKeyword(type)) {
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
    VariableIdentifier* parseVariableIdentifier(ASTAllocator& allocator);

    /**
     * lex a variable token into tokens until the until character occurs
     * only lexes the token if the identifier is not empty
     */
    bool lexVariableToken();

    /**
     * parse generic argument list
     */
    void parseGenericArgsList(std::vector<BaseType*>& outArgs, ASTAllocator& allocator);

    /**
     * parse a function call
     */
    FunctionCall* parseFunctionCall(ASTAllocator& allocator);

    /**
     * lexes a keyword access specifier public, private, internal & (if protect is true, then protected)
     */
    std::optional<AccessSpecifier> parseAccessSpecifier();

    /**
     * parses a accesss specifier, if not then default is returned
     */
    AccessSpecifier parseAccessSpecifier(AccessSpecifier def) {
        auto s = parseAccessSpecifier();
        return s.has_value() ? s.value() : def;
    }

    BaseType* ref_type_from(ASTAllocator& allocator, AccessChain* chain);

    /**
     * after an identifier has been consumed
     * we call this method to lex an access chain after it
     * identifier .element1.element2.element3
     * this is the method called by lexAccessChain after finding a identifier
     * @param assChain is the access chain in an assignment
     */
    Value* parseAccessChainAfterId(ASTAllocator& allocator, AccessChain* chain, Position& start, bool parseStruct = false);

    /**
     * this method does not compound the access chain, so can be called recursively
     * this method is called by lexAccessChain to not compound access chains nested in it
     * @param assChain is the access chain in an assignment
     */
    Value* parseAccessChainRecursive(ASTAllocator& allocator, AccessChain* chain, Position& start, bool parseStruct = false);

    /**
     * this method does not compound the access chain, so can be called recursively
     * this method is called by lexAccessChain to not compound access chains nested in it
     * @param assChain is the access chain in an assignment
     */
    bool lexAccessChainRecursive(bool lexStruct = false, unsigned int chain_length = 0);

    /**
     * parse access chain
     */
    Value* parseAccessChain(ASTAllocator& allocator, bool parseStruct = false);

    /**
     * it lexes a access chain, but allows a '&' operator before it to get the address of value
     * so this allows a.b.c or &a.b.c
     */
    Value* parseAccessChainOrAddrOf(ASTAllocator& allocator, bool parseStruct = false);

    /**
     * lex allowDeclarations or initialization tokens
     * like var x : int; or var x : int = 5;
     * @param allowDeclarations when true, it will allow allowDeclarations only without the value initialization
     * like #var x : int; when false however, it'll be strict initialization
     * @return whether it was able to lex the tokens for the statement
     */
    VarInitStatement* parseVarInitializationTokens(ASTAllocator& allocator, AccessSpecifier specifier, bool allowDeclarations = true, bool requiredType = false);

    /**
     * lex assignment tokens
     * like x = 5;
     * @return whether it was able to lex teh tokens for the statement
     */
    ASTNode* parseAssignmentStmt(ASTAllocator& allocator);

    /**
     * parses a single expression
     */
    std::optional<Operation> parseOperation();

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
    std::optional<Operation> parseAssignmentOperator();

    /**
     * parse lambda type
     */
    BaseType* parseLambdaType(ASTAllocator& allocator, bool isCapturing);

    /**
     * parses a generic type after id
     */
    BaseType* parseGenericTypeAfterId(ASTAllocator& allocator, BaseType* type);

    /**
     * parses a generic type or single linked type
     */
    BaseType* parseLinkedOrGenericType(ASTAllocator& allocator);

    /**
     * parse array and pointer type after id
     */
    BaseType* parseArrayAndPointerTypesAfterTypeId(ASTAllocator& allocator, BaseType* typeId);

    /**
     * parse type id
     */
    BaseType* parseTypeId(ASTAllocator& allocator, Token* type);

    /**
     * parse a single type
     */
    BaseType* parseType(ASTAllocator& allocator);

    /**
     * top level access specified declarations
     */
    bool lexTopLevelAccessSpecifiedDecls();

    /**
     * top level access specified declarations
     */
    ASTNode* parseTopLevelAccessSpecifiedDecls(ASTAllocator& allocator);

    /**
     * lexes a single top level statement, top level means in file scope, These include
     * functions, structs, interfaces, implementations, enum, annotations
     * comments, variable initialization with value, constants
     */
    ASTNode* parseTopLevelStatement(ASTAllocator& allocator);

    /**
     * lexes a single nested level statement, nested level means not top level (must not be in file scope)
     * These exclude functions, enum, structs, interfaces, implementations in nested scopes
     */
    ASTNode* parseNestedLevelStatementTokens(ASTAllocator& allocator, bool is_value = false, bool parse_value_node = false);

    /**
     * parse throw statement
     */
    ThrowStatement* parseThrowStatement(ASTAllocator& allocator);

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
    void parseTopLevelMultipleStatements(ASTAllocator& allocator, std::vector<ASTNode*>& nodes, bool break_at_no_stmt = false);

    /**
     * All import statements defined at top level will be lexed
     * @param should cause error on invalid syntax, or stop
     */
    void lexTopLevelMultipleImportStatements();

    /**
     * All import statements defined at top level will be lexed
     * @param should cause error on invalid syntax, or stop
     */
    void parseTopLevelMultipleImportStatements(ASTAllocator& allocator, std::vector<ASTNode*>& nodes);

    /**
     * lexes a multiple nested level statement, nested level means not top level (must not be in file scope)
     * These exclude functions, enum, structs, interfaces, implementations in nested scopes
     */
    void parseNestedLevelMultipleStatementsTokens(ASTAllocator& allocator, std::vector<ASTNode*>& nodes, bool is_value = false, bool parse_value_node = false);

    /**
     * lex single comment comment
     */
    bool lexSingleLineCommentTokens();

    /**
     * parse a single line comment node
     */
    Comment* parseSingleLineComment(ASTAllocator& allocator);

    /**
     * parses a multi line comment node
     */
    Comment* parseMultiLineComment(ASTAllocator& allocator);

    /**
     * lex multi line comment tokens
     */
    bool lexMultiLineCommentTokens();
    /**
     * parses a brace block, { statement(s) }
     */
    std::optional<Scope> parseBraceBlock(const std::string_view &forThing, ASTAllocator& allocator, void(*nested_lexer)(Parser*, ASTAllocator& allocator, std::vector<ASTNode*>& nodes));

    /**
     * parses a brace bock with nested statements
     */
    std::optional<Scope> parseBraceBlock(const std::string_view &forThing, ASTNode* parent_node, ASTAllocator& allocator);

    /**
     * lexes top level brace block
     */
    std::optional<Scope> parseTopLevelBraceBlock(ASTAllocator& allocator, const std::string_view& forThing);

    /**
     * parses a brace block or a value node
     */
    std::optional<Scope> parseBraceBlockOrValueNode(ASTAllocator& allocator, const std::string_view& forThing, bool is_value, bool parse_value_node);

    /**
     * lexes import identifier list example : { something, something }
     */
    bool lexImportIdentifierList();

    /**
     * lexes import statement
     */
    bool lexImportStatement();

    bool lexIdentifierToken();

    /**
     * lexes import statement
     */
    ImportStatement* parseImportStatement(ASTAllocator& allocator);

    /**
     * parse destruct statement
     */
    DestructStmt* parseDestructStatement(ASTAllocator& allocator);

    /**
     * parses a single return statement
     */
    ReturnStatement* parseReturnStatement(ASTAllocator& allocator);

    /**
     * parses a constructor init block
     */
    InitBlock* parseConstructorInitBlock(ASTAllocator& allocator);

    /**
     * parse a unsafe block
     */
    UnsafeBlock* parseUnsafeBlock(ASTAllocator& allocator);

    /**
     * parse break statement
     */
    BreakStatement* parseBreakStatement(ASTAllocator& allocator);

    /**
     * parses a single unreachable statement
     */
    UnreachableStmt* parseUnreachableStatement(ASTAllocator& allocator);

    /**
     * parses a single typealias statement
     */
    TypealiasStatement* parseTypealiasStatement(ASTAllocator& allocator, AccessSpecifier specifier);

    /**
     * parses a single continue statement
     */
    ContinueStatement* parseContinueStatement(ASTAllocator& allocator);

    /**
     * parse If expression and block, this useful as if can contain multiple ifs using else if
     */
    std::optional<std::pair<Value*, Scope>> parseIfExprAndBlock(ASTAllocator& allocator, bool is_value, bool lex_value_node, bool top_level);

    /**
     * parse an if statement
     */
    IfStatement* parseIfStatement(ASTAllocator& allocator, bool is_value, bool parse_value_node, bool top_level);

    /**
     * parses a single do while loop
     */
    DoWhileLoop* parseDoWhileLoop(ASTAllocator& allocator);

    /**
     * parses a single while loop
     */
    WhileLoop* parseWhileLoop(ASTAllocator& allocator);

    /**
     * parses a single for loop
     */
    ForLoop* parseForLoop(ASTAllocator& allocator);

    /**
     * lex loop block tokens
     */
    LoopBlock* parseLoopBlockTokens(ASTAllocator& allocator, bool is_value);

    /**
     * parse parameter list
     * @return whether the function is variadic or not
     */
    bool parseParameterList(
            ASTAllocator& allocator,
            std::vector<FunctionParam*>& parameters,
            bool optionalTypes = false,
            bool defValues = true,
            bool lexImplicitParams = true,
            bool variadicParam = true
    );

    /**
     * this occurs right after the function name
     */
    bool parseGenericParametersList(ASTAllocator& allocator, std::vector<GenericTypeParameter*>& params);

    /**
     * lexes a function block with parameters
     * @param allow_declaration allows a declaration, without body of the function that is
     */
    FunctionDeclaration* parseFunctionStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier, bool allow_declaration = false, bool allow_extensions = false);

    /**
     * parses a interface structure
     */
    InterfaceDefinition* parseInterfaceStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier);

    /**
     * parse namespace tokens
     */
    Namespace* parseNamespace(ASTAllocator& allocator, AccessSpecifier specifier);

    /**
     * parses a single struct member
     */
    StructMember* parseStructMember(ASTAllocator& allocator);

    /**
     * parses a single unnamed struct
     */
    UnnamedStruct* parseUnnamedStruct(ASTAllocator& allocator, AccessSpecifier specifier);

    bool parseVariableMemberInto(VariablesContainer* container, ASTAllocator& allocator, AccessSpecifier specifier);

    bool parseVariableAndFunctionInto(MembersContainer* container, ASTAllocator& allocator, AccessSpecifier specifier);

    /**
     * lexes a struct block
     */
    StructDefinition* parseStructStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier, bool unnamed = false, bool direct_init = false);

    /**
     * parses a single variant member
     */
    VariantMember* parseVariantMember(ASTAllocator& allocator, VariantDefinition* definition);

    /**
     * parses variant members and functions into the variant definition
     */
    bool parseAnyVariantMember(ASTAllocator& allocator, VariantDefinition* def, AccessSpecifier specifier);

    /**
     * lexes a struct block
     */
    VariantDefinition* parseVariantStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier);

    /**
     * parses a single unnamed union
     */
    UnnamedUnion* parseUnnamedUnion(ASTAllocator& allocator, AccessSpecifier specifier);

    /**
     * lexes a struct block
     */
    UnionDef* parseUnionStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier, bool unnamed = false, bool direct_init = false);

    /**
     * parses impl token
     */
    ImplDefinition* parseImplTokens(ASTAllocator& allocator, AccessSpecifier specifier);

    /**
     * lexes a enum block
     */
    EnumDeclaration* parseEnumStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier);

    /**
     * reads whitespace at current position
     */
    bool readWhitespace();

    /**
     * lex whitespace tokens
     * @deprecated
     */
    bool lexWhitespaceToken() {
        return readWhitespace();
    }

    /**
     * a utility function to lex whitespace tokens and also skip new lines
     */
    void consumeWhitespaceAndNewLines();

    /**
     * use consumeWhitespaceAndNewLines
     * @deprecated
     */
    void lexWhitespaceAndNewLines() {
        consumeWhitespaceAndNewLines();
    }

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
     * parse a single annotation, the annotation is stored in annotations vector
     */
    bool parseAnnotation(ASTAllocator& allocator);

    /**
     * parses a macro value
     */
    Value* parseMacroValue(ASTAllocator& allocator);

    /**
     * parses the null value, otherwise returns nullptr
     */
    Value* parseNull(ASTAllocator& allocator);

    /**
     * parses a bool value otherwise returns nullptr
     */
    Value* parseBoolValue(ASTAllocator& allocator);

    /**
     * parses a number value
     */
    Value* parseNumberValue(ASTAllocator& allocator);

    /**
     * parses struct value after the identifier
     */
    StructValue* parseStructValue(ASTAllocator& allocator, BaseType* refType, Position& start);

    /**
     * values like integer and string, but appearing in access chain
     */
    Value* parseAccessChainValueToken(ASTAllocator& allocator);

    /**
     * parses array initialization
     */
    Value* parseArrayInit(ASTAllocator& allocator);

    /**
     * values in provide are supported very less
     */
    Value* parseProvideValue(ASTAllocator& allocator);

    /**
     * user can write things after the value, like a cast
     * 123 as int, or 123 is int, this function takes care of parsing
     * all values afterward a parsed value inside the expression
     */
    Value* parseAfterValue(ASTAllocator& allocator, Value* value);

    /**
     * lexes access chain like x.y.z or a value like 10, could be int, string, char
     */
    Value* parseAccessChainOrValue(ASTAllocator& allocator, bool parseStruct = false);

    /**
     * parse value node
     */
    ValueNode* parseValueNode(ASTAllocator& allocator);

    /**
     * lex lambda after params list
     * @return true if there are no errors
     */
    bool parseLambdaAfterParamsList(ASTAllocator& allocator, LambdaFunction* lambda);

    /**
     * parses a single lambda function (PARAM1, PARAM2)[CAP1, CAP2] => {}
     */
    LambdaFunction* parseLambdaValue(ASTAllocator& allocator);

    /**
     * lexes remaining expression, this is used by lexExpressionTokens
     * this lexes the expression tokens after the first identifier / value
     * for example in expression a + b, after lexing a + b will lexed by this function
     * @param start is the start of the expression, index in tokens vector !
     */
    Value* parseRemainingExpression(ASTAllocator& allocator, Value* first_value);

    /**
     * it will lex a lambda meaning '() => {}' in a paren expression
     * it assumes you've already consumed '('
     */
    Value* parseLambdaOrExprAfterLParen(ASTAllocator& allocator);

    /**
     * parses a parenthesized expression
     */
    Value* parseParenExpression(ASTAllocator& allocator);

    /**
     * just a helper function for parsing expressions
     */
    void parseExpressionWith(ASTAllocator& allocator, ValueAndOperatorStack& stack, ValueAndOperatorStack& final);

    /**
     * parses a single not value
     */
    NotValue* parseNotValue(ASTAllocator& allocator);

    /**
     * parses a single negative value
     */
    NegativeValue* parseNegativeValue(ASTAllocator& allocator);

    /**
     * lexes an expression token which can contain access chain and values
     * @return whether an expression has been lexed, the expression can also be a single identifier or value
     */
    Value* parseExpression(ASTAllocator& allocator, bool parseStruct = false, bool parseLambda = true);

    /**
     * lexes switch block
     */
    SwitchStatement* parseSwitchStatementBlock(ASTAllocator& allocator, bool is_value = false, bool parse_value_node = false);

    /**
     * parse try catch
     */
    TryCatch* parseTryCatch(ASTAllocator& allocator);

    /**
     * lex using statement
     */
    UsingStmt* parseUsingStatement(ASTAllocator& allocator);

    /**
     * lex provide statement
     */
    ProvideStmt* parseProvideStatement(ASTAllocator& allocator);

    /**
     * lexes a comptime block
     */
    ComptimeBlock* parseComptimeBlock(ASTAllocator& allocator);

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
        CSTDiagnoser::diagnostic(message, file_path(), token->position, token->position, severity);
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

};