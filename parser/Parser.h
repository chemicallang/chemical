// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include <utility>
#include <vector>
#include "ast/base/AccessSpecifier.h"
#include "stream/SourceProvider.h"
#include "ast/utils/Operation.h"
#include "core/diag/Diagnostic.h"
#include <memory>
#include <optional>
#include "compiler/ASTDiagnoser.h"
#include "lexer/Token.h"
#include "ast/base/ASTAllocator.h"
#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"
#include "ast/base/BaseType.h"
#include "ast/base/Annotation.h"
#include "ast/base/LocatedIdentifier.h"
#include "compiler/processor/ModuleFileData.h"
#include "ast/base/TypeLoc.h"
#include "compiler/cbi/model/CBIFunctionType.h"

class CompilerBinder;

class Parser;

class GlobalInterpretScope;

class ValueAndOperatorStack;

class TypeBuilder;

struct AnnotationDefinition;

class AnnotationController;

struct SavedAnnotation {

    AnnotationDefinition& definition;

    std::vector<Value*> arguments;

};

class BasicParser : public ASTDiagnoser {
public:

    /**
     * The file id to use with location manager
     */
    unsigned int file_id;

    /**
     * the current token
     */
    Token* token;

    /**
     * current parent node
     */
    ASTNode* parent_node = nullptr;

    /**
     * constructor
     */
    inline BasicParser(
            LocationManager& loc_man,
            unsigned int file_id,
            Token* token
    ) : file_id(file_id), token(token), ASTDiagnoser(loc_man) {

    }

    /**
     * allocate given view on allocator
     */
    inline chem::string_view allocate_view(BatchAllocator& allocator, const chem::string_view& view) {
        return { allocator.allocate_str(view.data(), view.size()), view.size() };
    }

    /**
     * get a encoded location
     */
    uint64_t loc(const Position& start, const Position& end);

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
     * get location for a single token that is on the same line
     */
    inline uint64_t loc_single(Token& t) {
        return loc_single(t.position, t.value.size());
    }

    /**
     * consume the current token if it's given type
     */
    bool consumeToken(enum TokenType type) {
        if(token->type == type) {
            token++;
            return true;
        } else {
            return false;
        }
    }

    /**
     * parses the token of type
     * will skip new line, comment token, multi line comment tokens to check for this
     */
    Token* consumeOfType(enum TokenType type) {
        auto& t = *token;
        if(t.type == type) {
            token++;
            return &t;
        } else {
            return nullptr;
        }
    }

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
     * a utility function to skip new lines
     */
    void consumeNewLines();

    /**
     * a package definition is parsed
     */
    bool parseModuleDefinition(ASTAllocator& allocator, ModuleFileData& data);

    /**
     * a source statement defines a source directory in .mod file
     */
    bool parseSourceStmt(ASTAllocator& allocator, ModuleFileData& data);

    /**
     * a module file is a .mod file that defines which modules are imported
     * by a single module, and it's scope and module declaration
     */
    void parseModuleFile(ASTAllocator& allocator, ModuleFileData& data);

    /**
     * parse a string
     */
    std::optional<chem::string_view> parseString(ASTAllocator& allocator);

    /**
     * parses import statement after the import keyword
     */
    ImportStatement* parseImportStmtAfterKw(ASTAllocator& allocator, bool error_out = true);

    /**
     * lexes import statement
     */
    ImportStatement* parseImportStatement(ASTAllocator& allocator);

    /**
     * parses a single or multiple import statements
     */
    bool parseSingleOrMultipleImportStatements(ASTAllocator& allocator, std::vector<ASTNode*>& nodes);

    /**
     * get file path for the current file id
     */
    chem::string_view get_file_path();

    /**
     * make a diagnostic with given parameters, for the current file
     */
    Diag make_diag(Position start, const chem::string_view &message, DiagSeverity severity) {
        return Diagnoser::make_diag(message, get_file_path(), start, token->position, severity);
    }

    /**
     * get an empty diagnostic to append to
     */
    Diag& empty_diagnostic(const Position& start, const Position& end, DiagSeverity severity) {
        return Diagnoser::empty_diagnostic(get_file_path(), start, end, severity);
    }

    /**
     * record a diagnostic at the given position
     */
    inline Diag& empty_diagnostic(const Position& start, DiagSeverity severity) {
        return empty_diagnostic(start, start, severity);
    }

    /**
     * record a diagnostic with the given severity at token position
     */
    inline Diag& diagnostic(DiagSeverity severity) {
        return empty_diagnostic(token->position, severity);
    }

    /**
     * create hint diagnostic at current token position
     */
    inline Diag& hint() {
        return diagnostic(DiagSeverity::Hint);
    }

    /**
     * create info diagnostic at current token position
     */
    inline Diag& info() {
        return diagnostic(DiagSeverity::Information);
    }

    /**
     * create warning diagnostic at current token position
     */
    inline Diag& warning() {
        return diagnostic(DiagSeverity::Warning);
    }

    /**
     * create info diagnostic at current token position
     */
    inline Diag& error() {
        return diagnostic(DiagSeverity::Error);
    }

    /**
     * create hint diagnostic with the following message at current token position
     */
    inline void hint(const chem::string_view& message) {
        diagnostic(DiagSeverity::Hint) << message;
    }

    /**
     * create info diagnostic with the following message at current token position
     */
    inline void info(const chem::string_view& message) {
        diagnostic(DiagSeverity::Information) << message;
    }

    /**
     * create warning diagnostic with the following message at current token position
     */
    inline void warning(const chem::string_view& message) {
        diagnostic(DiagSeverity::Warning) << message;
    }

    /**
     * create info diagnostic with the following message at current token position
     */
    inline void error(const chem::string_view& message) {
        diagnostic(DiagSeverity::Error) << message;
    }

    /**
     * create info diagnostic with the following message at current token position
     */
    inline void unexpected_error(const chem::string_view& message) {
        diagnostic(DiagSeverity::Error) << message << ", got '" << token->value << '\'';
    }

    /**
     * save error at given error token position
     */
    inline void error(const chem::string_view& message, const Position& position) {
        empty_diagnostic(position, DiagSeverity::Error) << message;
    }

};

/**
 * converts the given token type to access specifier
 * if not valid, public is returned
 */
enum AccessSpecifier get_specifier_from(enum TokenType type);

/**
 * the parser that is used to parse
 */
class Parser : public BasicParser {
public:

    /**
     * stored for debugging
     * TODO remove this
     */
    std::string_view stored_file_path;

    /**
     * the binder that will be used to compile binding code
     * if not present, cbi is considered disabled
     */
    CompilerBinder* const binder;

    /**
     * annotation controller
     */
    AnnotationController& controller;

    /**
     * the global allocator for the job
     */
    ASTAllocator& global_allocator;

    /**
     * the module allocator for the job
     */
    ASTAllocator& mod_allocator;

    /**
     * the type builder allows to cache types
     */
    TypeBuilder& typeBuilder;

    /**
     * the ast we're generating, is it for a 64 bit target
     */
    bool is64Bit;

    /**
     * these are annotation modifier functions that will be called on the next node
     */
    std::vector<SavedAnnotation> annotations;

    /**
     * initialize the lexer with this provider and path
     */
    Parser(
        unsigned int file_id,
        std::string_view file_path,
        Token* start_token,
        LocationManager& loc_man,
        AnnotationController& controller,
        ASTAllocator& global_allocator,
        ASTAllocator& mod_allocator,
        TypeBuilder& typeBuilder,
        bool is64Bit,
        CompilerBinder* binder = nullptr
    );

    /**
     * get the file path
     */
    std::string_view file_path();

    /**
     * get a located identifier
     */
    LocatedIdentifier loc_id(BatchAllocator& allocator, const chem::string_view& value, const Position& pos);

    /**
     * get a located identifier
     */
    inline LocatedIdentifier loc_id(BatchAllocator& allocator, Token* token) {
        return loc_id(allocator, token->value, token->position);
    }

    /**
     * get the ending position of the token
     */
    Position end_pos(Token* token);

    /**
     * suppose to be called on a node which can take annotations
     */
    void annotate(ASTNode* node);

    /**
     * parses nodes into the given vector
     */
    void parse(std::vector<ASTNode*>& nodes);

    // ------------- Functions exposed to chemical begin here

public:

    /**
     * parses the token of type
     * will skip new line, comment token, multi line comment tokens to check for this
     */
    [[deprecated]]
    inline Token* consumeWSOfType(enum TokenType type) {
        return consumeOfType(type);
    }

    /**
     * parses a variable otherwise returns nullptr
     */
    VariableIdentifier* parseVariableIdentifier(ASTAllocator& allocator);

    /**
     * parses a single string value using the given allocator
     */
    Value* parseStringValue(ASTAllocator& allocator);

    /**
     * parse generic argument list
     */
    void parseGenericArgsList(std::vector<TypeLoc>& outArgs, ASTAllocator& allocator);

    /**
     * parse a function call
     */
    FunctionCall* parseFunctionCall(ASTAllocator& allocator, std::vector<ChainValue*>& values);

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

    LinkedType* ref_type_from(ASTAllocator& allocator, std::vector<ChainValue*>& values);

    /**
     * after an identifier has been consumed
     * we call this method to lex an access chain after it
     * identifier .element1.element2.element3
     * this is the method called by lexAccessChain after finding a identifier
     * @param assChain is the access chain in an assignment
     */
    Value* parseAccessChainAfterId(
            ASTAllocator& allocator,
            std::vector<ChainValue*>& values,
            Position& start,
            bool parseStruct = false,
            bool parseGenList = true
    );

    /**
     * this method does not compound the access chain, so can be called recursively
     * this method is called by lexAccessChain to not compound access chains nested in it
     * @param assChain is the access chain in an assignment
     */
    Value* parseAccessChainRecursive(ASTAllocator& allocator, std::vector<ChainValue*>& values, Position& start, bool parseStruct = false);

    /**
     * parses access chain node
     */
    void parseAccessChain(ASTAllocator& allocator, std::vector<ChainValue*>& values);

    /**
     * parse access chain
     */
    Value* parseAccessChain(ASTAllocator& allocator, bool parseStruct = false);

    /**
     * parse access chain or a value like null, false, true
     */
    Value* parseAccessChainOrKwValue(ASTAllocator& allocator, bool parseStruct = false);

    /**
     * parses a single addr of value
     */
    AddrOfValue* parseAddrOfValue(ASTAllocator& allocator);

    /**
     * parses a single dereference value
     */
    DereferenceValue* parseDereferenceValue(ASTAllocator& allocator);

    /**
     * it lexes a access chain, but allows a '&' operator before it to get the address of value
     * so this allows a.b.c or &a.b.c
     */
    Value* parseLhsValue(ASTAllocator& allocator);

    /**
     * it lexes a access chain, but allows a '&' operator before it to get the address of value
     * so this allows a.b.c or &a.b.c
     */
    Value* parseAccessChainOrAddrOf(ASTAllocator& allocator, bool parseStruct = false);

    /**
     * parse a pattern match expression
     */
    void parsePatternMatchExprAfterId(
            ASTAllocator& allocator,
            PatternMatchExpr* into,
            bool is_lbrace,
            bool parseElse
    );

    /**
     * lex allowDeclarations or initialization tokens
     * like var x : int; or var x : int = 5;
     * @param allowDeclarations when true, it will allow allowDeclarations only without the value initialization
     * like #var x : int; when false however, it'll be strict initialization
     * @return whether it was able to lex the tokens for the statement
     */
    ASTNode* parseVarInitializationTokens(
            ASTAllocator& allocator,
            AccessSpecifier specifier,
            bool matchExpr,
            bool allowDeclarations,
            bool requiredType,
            bool comptime
    );

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
     * parse a lambda type with location
     */
    TypeLoc parseLambdaTypeLoc(ASTAllocator& allocator, bool isCapturing);

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
    BaseType* parseArrayAndPointerTypesAfterTypeId(ASTAllocator& allocator, BaseType* typeId, SourceLocation location);

    /**
     * returns linked value type if found otherwise provided linked type
     */
    LinkedValueType* parseLinkedValueType(ASTAllocator& allocator, Token* type, SourceLocation location);

    /**
     * parses a single struct type
     */
    StructType* parseStructType(ASTAllocator& allocator);

    /**
     * parses a single struct type with location
     */
    TypeLoc parseStructTypeLoc(ASTAllocator& allocator);

    /**
     * parses a single union type
     */
    UnionType* parseUnionType(ASTAllocator& allocator);

    /**
     * parses a single union type with location
     */
    TypeLoc parseUnionTypeLoc(ASTAllocator& allocator);

    /**
     * expression type only works with logical && and || operators
     */
    BaseType* parseExpressionType(ASTAllocator& allocator, BaseType* firstType);

    /**
     * parse a single type with location
     */
    TypeLoc parseTypeLoc(ASTAllocator& allocator);

    /**
     * parse a single type
     */
    BaseType* parseType(ASTAllocator& allocator) {
        return const_cast<BaseType*>(parseTypeLoc(allocator).getType());
    }

    /**
     * top level access specified declarations
     */
    ASTNode* parseTopLevelAccessSpecifiedDecl(ASTAllocator& allocator, AccessSpecifier specifier, bool comptime);

    /**
     * lexes a single top level statement, top level means in file scope, These include
     * functions, structs, interfaces, implementations, enum, annotations
     * comments, variable initialization with value, constants
     */
    ASTNode* parseTopLevelStatement(ASTAllocator& allocator, bool comptime);

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
     * All top levels statements lexed, These include
     * functions, structs, interfaces, implementations
     * comments, variable initialization with value, constants
     */
    void parseTopLevelMultipleStatements(ASTAllocator& allocator, std::vector<ASTNode*>& nodes, bool break_at_no_stmt = false);

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
     * parses a brace bock with nested statements
     */
    std::optional<Scope> parseBraceBlock(const std::string_view &forThing, ASTNode* parent_node, ASTAllocator& allocator);

    /**
     * parses top level brace block
     */
    std::optional<Scope> parseTopLevelBraceBlock(ASTAllocator& allocator, const std::string_view& forThing);

    /**
     * parses a nested level brace block
     */
    std::optional<Scope> parseNestedBraceBlock(const std::string_view &forThing, ASTAllocator& allocator);

    /**
     * parses a brace block or a value node
     */
    std::optional<Scope> parseBraceBlockOrValueNode(ASTAllocator& allocator, const std::string_view& forThing, bool is_value, bool parse_value_node);

    /**
     * parse destruct statement
     */
    DestructStmt* parseDestructStatement(ASTAllocator& allocator);

    /**
     * parse dealloc statement
     */
    DeallocStmt* parseDeallocStatement(ASTAllocator& allocator);

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
    ASTNode* parseTypealiasStatement(ASTAllocator& allocator, AccessSpecifier specifier);

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
    ASTNode* parseFunctionStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier, bool member, bool allow_extensions, bool comptime);

    /**
     * parses a interface structure
     */
    ASTNode* parseInterfaceStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier);

    /**
     * parse namespace tokens
     */
    Namespace* parseNamespace(ASTAllocator& allocator, AccessSpecifier specifier);

    /**
     * parses a single struct member
     */
    StructMember* parseStructMember(ASTAllocator& allocator, AccessSpecifier specifier);

    /**
     * parses a single unnamed struct
     */
    UnnamedStruct* parseUnnamedStruct(ASTAllocator& allocator, AccessSpecifier specifier);

    /**
     * parses members of a container, for example (struct / union members) or even compile time if statements
     */
    void parseContainerMembersInto(VariablesContainer* container, ASTAllocator& allocator, AccessSpecifier specifier, bool comptime);

    /**
     * lexes a struct block
     */
    ASTNode* parseStructStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier);

    /**
     * parses a single variant member
     */
    VariantMember* parseVariantMember(ASTAllocator& allocator, VariantDefinition* definition);

    /**
     * parses variant members and functions into the variant definition
     */
    bool parseAnyVariantMember(ASTAllocator& allocator, VariantDefinition* def, AccessSpecifier specifier, bool comptime);

    /**
     * lexes a struct block
     */
    ASTNode* parseVariantStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier);

    /**
     * parses a single unnamed union
     */
    UnnamedUnion* parseUnnamedUnion(ASTAllocator& allocator, AccessSpecifier specifier);

    /**
     * lexes a struct block
     */
    ASTNode* parseUnionStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier);

    /**
     * parses impl token
     */
    ASTNode* parseImplTokens(ASTAllocator& allocator, AccessSpecifier specifier);

    /**
     * lexes a enum block
     */
    EnumDeclaration* parseEnumStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier);

    /**
     * a single character value is parsed
     */
    Value* parseCharValue(ASTAllocator& allocator);

    /**
     * parse a single annotation, the annotation is stored in annotations vector
     */
    bool parseAnnotation(ASTAllocator& allocator);

    /**
     * parses a macro value
     */
    Value* parseMacroValue(ASTAllocator& allocator);

    /**
     * parses a macro value node
     */
    ASTNode* parseMacroNode(ASTAllocator& allocator, CBIFunctionType type);

    /**
     * parses a number value
     */
    Value* parseNumberValue(ASTAllocator& allocator);

    /**
     * parses struct value after the identifier
     */
    StructValue* parseStructValue(ASTAllocator& allocator, BaseType* refType, Position& start);

    /**
     * parses array initialization
     */
    Value* parseArrayInit(ASTAllocator& allocator);

    /**
     * parses a single new value
     */
    Value* parseNewValue(ASTAllocator& allocator);

    /**
     * parses a single unsafe value
     */
    Value* parseUnsafeValue(ASTAllocator& allocator);

    /**
     * parses a single comptime value
     */
    Value* parseComptimeValue(ASTAllocator& allocator);

    /**
     * parses the sizeof value
     */
    Value* parseSizeOfValue(ASTAllocator& allocator);

    /**
     * parses the align of value
     */
    Value* parseAlignOfValue(ASTAllocator& allocator);

    /**
     * parses a new value as a node
     */
    ASTNode* parsePlacementNewNode(ASTAllocator& allocator);

    /**
     * values in provide are supported very less
     */
    Value* parseProvideValue(ASTAllocator& allocator);

    /**
     * parses post increment and decrement after given value
     */
    Value* parsePostIncDec(ASTAllocator& allocator, Value* value, Token* start_token);

    /**
     * user can write things after the value, like a cast
     * 123 as int, or 123 is int, this function takes care of parsing
     * all values afterward a parsed value inside the expression
     */
    Value* parseAfterValue(ASTAllocator& allocator, Value* value, Token* start_token);

    /**
     * increment decrement node is parsed
     */
    IncDecNode* parsePreIncDecNode(ASTAllocator& allocator, bool increment);

    /**
     * increment decrement value is parsed
     */
    Value* parsePreIncDecValue(ASTAllocator& allocator, bool increment);

    /**
     * lexes access chain like x.y.z or a value like 10, could be int, string, char
     */
    Value* parseAccessChainOrValueNoAfter(ASTAllocator& allocator, bool parseStruct = false);

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
    Value* parseRemainingExpression(ASTAllocator& allocator, Value* first_value, Token* start_tok);

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
     * parses an expression array, or struct
     */
    Value* parseExpressionOrArrayOrStruct(ASTAllocator& allocator, bool parseLambda = true);

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
     * parse the alias statement
     */
    AliasStmt* parseAliasStatement(ASTAllocator& allocator, AccessSpecifier specifier);

    /**
     * lex provide statement
     */
    ProvideStmt* parseProvideStatement(ASTAllocator& allocator);

    /**
     * parses a comptime block without a comptime keyword
     */
    ComptimeBlock* parseComptimeBlockNoKw(ASTAllocator& allocator);

    /**
     * lexes a comptime block
     */
    ComptimeBlock* parseComptimeBlock(ASTAllocator& allocator) {
        if(token->type == TokenType::ComptimeKw) {
            token++;
            return parseComptimeBlockNoKw(allocator);
        } else {
            return nullptr;
        }
    }

    // -------------------------------- Exposed till here

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

};