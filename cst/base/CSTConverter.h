// Copyright (c) Qinetik 2024.

#pragma once

#include "common/Diagnostic.h"
#include "ast/base/ASTNode.h"
#include "CSTVisitor.h"
#include <unordered_map>
#include <string>
#include <functional>

#include <memory>
#include <vector>

class LoopASTNode;

class CSTConverter : public CSTVisitor {
public:

    /**
     * function parameter index
     */
    unsigned int param_index = 0;

    /**
     * nodes found when visiting tokens
     */
    std::vector<std::unique_ptr<ASTNode>> nodes;

    /**
     * types found when visiting tokens
     */
    std::vector<std::unique_ptr<BaseType>> types;

    /**
     * values found when visiting tokens
     */
    std::vector<std::unique_ptr<Value>> values;

    /**
     * diagnostics, containing errors and warning
     */
    std::vector<Diagnostic> diagnostics;

    /**
     * primitive type map that is initialized when visitor is initialized
     */
    std::unordered_map<std::string, std::function<BaseType *()>> primitive_type_map;

    /**
     * This is a pointer to current function declaration
     * All nodes being parsed belong to this function's body
     */
    FunctionDeclaration *current_func_decl = nullptr;

    /**
     * The current loop node
     * All nodes being parsed belong this loop's body
     */
    LoopASTNode *current_loop_node = nullptr;

    /**
     * constructor
     */
    CSTConverter();

    /**
     * visit the tokens, from start to end
     * @param start start is the inclusive index at which to begin visiting
     * @param end end is exclusive index at which to end visiting
     */
    void visit(std::vector<std::unique_ptr<CSTToken>> &tokens, unsigned int start, unsigned int end);

    /**
     * A helper function to visit the tokens starting at start
     */
    void visit(std::vector<std::unique_ptr<CSTToken>> &tokens, unsigned int start) {
        visit(tokens, start, tokens.size());
    }

    /**
     * converts the tokens, or visits all the tokens
     */
    void convert(std::vector<std::unique_ptr<CSTToken>> &tokens) {
        visit(tokens, 0, tokens.size());
    }

    /**
     * consumes the latest (last in the vector) value from the values vector
     */
    std::unique_ptr<Value> value();

    /**
     * consumes the latest (last in the vector) type from the types vector
     */
    std::unique_ptr<BaseType> type();

    /**
     * consume the latest value optionally
     */
    std::optional<std::unique_ptr<Value>> opt_value();

    /**
     * consume the latest type optionally
     */
    std::optional<std::unique_ptr<BaseType>> opt_type();

    void error(const std::string &message, CSTToken *start, CSTToken *end, DiagSeverity severity = DiagSeverity::Error);

    void error(const std::string &message, CSTToken *inside, DiagSeverity severity = DiagSeverity::Error);

    // nodes

    void visit(FunctionParamCST *param) override;

    void visit(FunctionCST *function) override;

    void visit(VarInitCST *varInit) override;

    void visit(AssignmentCST *assignment) override;

    void visit(ImportCST *importCst) override;

    void visit(ReturnCST *returnCst) override;

    void visit(BodyCST *bodyCst) override;

    // Types

    void visit(TypeToken *token) override;

    void visit(GenericTypeCST *genericType) override;

    void visit(ArrayTypeCST *arrayType) override;

    void visit(FunctionTypeCST *functionType) override;

    // Values

    void visit(StringToken *token) override;

    void visit(CharToken *token) override;

    void visit(NumberToken *token) override;

    void visit(StructValueCST *structValueCst) override;

    void visit(ArrayValueCST *arrayValue) override;

    void visit(VariableToken *token) override;

    void visit(BoolToken *token) override;

    void visit(AccessChainCST *accessChain) override;

    void visit(ExpressionCST *expressionCst) override;

    void visit(NegativeCST *negativeCst) override;

    void visit(NotCST *notCst) override;

};