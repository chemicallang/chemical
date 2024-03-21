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
#include "ast/values/ArrayValue.h"
#include "ast/statements/Return.h"
#include "ast/statements/Break.h"
#include "ast/statements/Continue.h"
#include "ast/values/StructValue.h"
#include "ast/statements/Assignment.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/statements/Comment.h"
#include "parser/utils/ValueAndOperatorStack.h"
#include "lexer/model/tokens/VariableToken.h"
#include <optional>
#include <iostream>

class ImportStatement;

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
    std::vector<Diagnostic> errors;

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
     * This parses the nodes of one scope
     * @return
     */
    std::vector<std::unique_ptr<ASTNode>> parseScopeNodes();

    /**
     * This parses multiple statements, creates a Scope and moves nodes into the scope
     * and returns it
     */
    inline Scope parseScope() {
        return {parseScopeNodes()};
    }

    /**
     * This parses multiple statements, creates a LoopScope and moves nodes into the scope
     * and returns it
     * @return
     */
    inline LoopScope parseLoopScope() {
        return {parseScopeNodes()};
    }

    /**
     * parses the type token as a type
     * @return
     */
    lex_ptr<BaseType> parseType();

    /**
    * This parses multiple statements as ASTNode(s) into nodes
     */
    void parseMultipleStatements();

    /**
     * parses a number, int, float, double
     * @return
     */
    lex_ptr<Value> parseNumberValue();

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
     * parses a single array value
     * @return
     */
    lex_ptr<ArrayValue> parseArrayValue();

    /**
     * parses a single struct value
     * @return
     */
    lex_ptr<StructValue> parseStructValue(const std::string& structName);

    /**
     * parses a single value, which can be an expression, int, float...
     * @return
     */
    lex_ptr<Value> parseValue();

    /**
     * this is a helper function that's supposed to be called by parseRemainingInterpretableExpression
     * @param stack
     * @param final
     */
    void parseExpressionWith(ValueAndOperatorStack &stack, ValueAndOperatorStack &final);

    /**
     * parse the remaining expression as interpretable, what this does is
     * parse the expression as a postfix expression which can be interpreted
     * by changing the order of values / expressions
     * @param firstValue
     * @return
     */
    std::unique_ptr<Value> parseRemainingInterpretableExpression(std::unique_ptr<Value> firstValue);

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
     * parses a single assignment statement
     * @return
     */
    lex_ptr<ASTNode> parseVarAssignStatement();

    /**
     * parses a variable assignment state
     * @return whether a node was added
     */
    inline bool parseVarAssignStatementBool() {
        return parse_return_bool([&]() -> lex_ptr<ASTNode> {
            return parseVarAssignStatement();
        });
    }

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
     * parses a single if block, meaning condition and block, no else ifs and else
     * @return
     */
    std::optional<std::pair<std::unique_ptr<Value>, Scope>> parseSingleIfBlock();

    /**
     * Parse a single if statement
     * @return
     */
    lex_ptr<IfStatement> parseIfStatement();

    /**
     * parses an if statement into nodes, if found return strue
     * @return
     */
    inline bool parseIfStatementBool() {
        return parse_return_bool([&]() -> lex_ptr<IfStatement> {
            return parseIfStatement();
        });
    }

    /**
     * Parse a single for loop
     * @return
     */
    lex_ptr<ForLoop> parseForLoop();

    /**
     * parses a single for loop
     * @return true if parsed
     */
    inline bool parseForLoopBool() {
        return parse_return_bool([&]() -> lex_ptr<ForLoop> {
            return parseForLoop();
        });
    }

    /**
     * Parse a single while loop
     * @return
     */
    lex_ptr<WhileLoop> parseWhileLoop();

    /**
     * parses a while loop
     * @return true, if parsed
     */
    inline bool parseWhileLoopBool() {
        return parse_return_bool([&]() -> lex_ptr<WhileLoop> {
            return parseWhileLoop();
        });
    }

    /**
     * Parse a single do while loop
     * @return
     */
    lex_ptr<DoWhileLoop> parseDoWhileLoop();

    /**
     * parses a single do while loop
     * @return true if parsed
     */
    inline bool parseDoWhileLoopBool() {
        return parse_return_bool([&]() -> lex_ptr<DoWhileLoop> {
            return parseDoWhileLoop();
        });
    }

    /**
     * parses a single import statement
     * @return
     */
    lex_ptr<ImportStatement> parseImportStatement();

    /**
     * parse a import statement
     * @return
     */
    bool parseImportStatementBool();

    /**
     * parse a single continue statement in a loop
     * @return
     */
    inline bool parseContinueStatement() {
        if (consume("continue")) {
            nodes.emplace_back(std::make_unique<ContinueStatement>(current_loop_node));
            return true;
        } else {
            return false;
        }
    }

    /**
     * parse a single break statement in a loop
     * @return
     */
    inline bool parseBreakStatement() {
        if (consume("break")) {
            nodes.emplace_back(std::make_unique<BreakStatement>(current_loop_node));
            return true;
        } else {
            return false;
        }
    }

    /**
     * parse a single return statement present inside a function
     * @return
     */
    lex_ptr<ReturnStatement> parseReturnStatement();

    /**
     * parses a return s
     * @return
     */
    inline bool parseReturnStatementBool() {
        return parse_return_bool([&]() -> lex_ptr<ReturnStatement> {
            return parseReturnStatement();
        });
    }

    /**
     * Parse a single function definition
     * @param declaration specify whether to allow declarations or generate errors instead
     * @return
     */
    lex_ptr<FunctionDeclaration> parseFunctionDefinition(bool declaration = false);

    /**
     * Parse a single function definition
     * @return true if parsed
     */
    inline bool parseFunctionDefinitionBool(bool declaration = false) {
        return parse_return_bool([&]() -> std::optional<std::unique_ptr<FunctionDeclaration>> {
            return parseFunctionDefinition(declaration);
        });
    }

    /**
     * Parse a single enum declaration
     * @return
     */
    lex_ptr<EnumDeclaration> parseEnumDeclaration();

    /**
     * Parse a single enum declaration
     * @return true if parsed
     */
    inline bool parseEnumDeclarationBool() {
        return parse_return_bool([&]() -> lex_ptr<EnumDeclaration> {
            return parseEnumDeclaration();
        });
    }

    /**
     * Parse a single interface definition
     * @return
     */
    lex_ptr<InterfaceDefinition> parseInterfaceDefinition();

    /**
     * Parse a single interface definition
     * @return true if parsed
     */
    inline bool parseInterfaceDefinitionBool() {
        return parse_return_bool([&]() -> lex_ptr<InterfaceDefinition> {
            return parseInterfaceDefinition();
        });
    }

    /**
     * parses a single comment
     * @return
     */
    lex_ptr<Comment> parseComment();

    /**
     * parses a singel comment
     * @return true if comment is pased
     */
    bool parseCommentBool() {
        return parse_return_bool([&]() -> lex_ptr<Comment> {
            return parseComment();
        });
    }

    /**
     * Parse a single implementation definition
     * @return
     */
    lex_ptr<ImplDefinition> parseImplementationDefinition();

    /**
     * Parse a single implementation definition
     * @return true if parsed
     */
    inline bool parseImplementationDefinitionBool() {
        return parse_return_bool([&]() -> lex_ptr<ImplDefinition> {
            return parseImplementationDefinition();
        });
    }

    /**
     * Parse a single struct definition
     * @return
     */
    lex_ptr<StructDefinition> parseStructDefinition();

    /**
     * Parse a single struct definition
     * @return true if parsed
     */
    inline bool parseStructDefinitionBool() {
        return parse_return_bool([&]() -> lex_ptr<StructDefinition> {
            return parseStructDefinition();
        });
    }

    /**
     *
     * @tparam TFunc
     * @param yield
     * @return
     */
    template<typename TFunc>
    inline bool parse_return_bool(TFunc yield) {
        auto value = yield();
        if (value.has_value()) {
            nodes.emplace_back(std::move(value.value()));
            return true;
        } else {
            return !errors.empty();
        }
    }

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
     * consume a string token
     * @return
     */
    std::optional<std::string> consume_str_token();

    /**
     * consume a char token
     * @return
     */
    std::optional<char> consume_char_token();

    /**
     * a operation token ia a unique token for mathematical or logical operations like >= and +
     * @return
     */
    std::optional<Operation> consume_op_token();

    /**
     * consume any char operator token
     * @return
     */
    std::optional<char> consume_char_op();

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
    void diagnostic(const std::string &err, int tokenPosition, DiagSeverity severity) {
        std::string errStr = err;
        if (tokenPosition < tokens.size()) {
            auto t = tokens[tokenPosition].get();
            errStr += " stopped at " + t->type_string();
        }
        errStr.append(1, '\n');
        errors.emplace_back(
                Range(tokens[tokenPosition]->position, tokens[tokenPosition]->position),
                severity,
                std::nullopt,
                errStr
        );
    }

    /**
     * This just calls the diagnostic method above with current token position
     * @param err
     * @param severity
     */
    void diagnostic(const std::string &err, DiagSeverity severity) {
        diagnostic(err, position, severity);
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
     * consumes a single identifier token, this is definitely a variable
     * @return consumed identifier token
     */
    inline std::optional<std::string> consume_identifier(bool error_out = true) {
        auto variable = consumeOfType<VariableToken>(LexTokenType::Variable, error_out);
        if(variable.has_value()) {
            return variable.value()->value;
        } else {
            return std::nullopt;
        }
    }

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

    /**
     * a stateful variable, in the middle of parsing
     * n turned on expressions are parsed as interpretable
     */
    bool isParseInterpretableExpressions = false;

private:

    /**
     * this is the position in tokens variable
     * the current token being looked at !
     */
    unsigned int position = 0;

    /**
     * a stateful variable, in the middle of parsing
     * when turned on return statements are parsed
     */
    bool isParseReturnStatement = false;

    /**
     * a stateful variable, in the middle of parsing
     * when turned on import statements are parsed
     */
    bool isParseImportStatement = true;

    /**
     * a stateful variable, in the middle of parsing
     * when turned on continue statement are parsed
     */
    bool isParseContinueStatement = false;

    /**
     * a stateful variable, in the middle of parsing
     * when turned on break statements are parsed
     */
    bool isParseBreakStatement = false;

    /**
     * This is a pointer to current function declaration
     * All nodes being parsed belong to this function's body
     */
    FunctionDeclaration *current_func_decl;

    /**
     * The current loop node
     * All nodes being parsed belong this loop's body
     */
    LoopASTNode *current_loop_node;

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