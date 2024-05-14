// Copyright (c) Qinetik 2024.

#pragma once

#include "common/Diagnostic.h"
#include "CSTVisitor.h"
#include "CSTDiagnoser.h"
#include <unordered_map>
#include <string>
#include "utils/functionalfwd.h"

#include <memory>
#include <vector>

class LoopASTNode;

using cst_tokens_ref_type = std::vector<std::unique_ptr<CSTToken>> &;

struct FunctionParamsResult {
    bool isVariadic;
    func_params params;
    unsigned int index;
};

class CSTConverter : public CSTVisitor, public CSTDiagnoser {
private:

    /**
     * are function param types optional, used to check when converting function parameters
     * since lambda parameter types are optional
     */
    bool optional_param_types = false;

    /**
     * is code gen for 64bit
     */
    bool is64Bit;

public:

    /**
     * function parameter index
     */
    unsigned int param_index = 0;

    /**
     * when true, do not create nodes for imports
     */
    bool no_imports = false;

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
    std::vector<Diag> diagnostics;

    /**
     * primitive type provide is a function when given a type in string format
     * like 'int' it will create a AST BaseType
     */
    typedef BaseType*(*PrimitiveTypeProvider)(CSTConverter*);

    /**
     * primitive type map that is initialized when visitor is initialized
     */
    std::unordered_map<std::string, PrimitiveTypeProvider> primitive_type_map;

    /**
     * This is a pointer to current function declaration
     * All nodes being parsed belong to this function's body
     */
    FunctionDeclaration *current_func_decl = nullptr;

    /**
     * This is a pointer to current struct declaration
     * All nodes being parsed belong to this struct
     */
    StructDefinition *current_struct_decl = nullptr;

    /**
     * This is a pointer to current interface declaration
     * All nodes being parsed belong to this interface
     */
    InterfaceDefinition* current_interface_decl = nullptr;

    /**
     * This is a pointer to current impl declaration
     * All nodes being parsed belong to this implementation
     */
    ImplDefinition* current_impl_decl = nullptr;

    /**
     * The current loop node
     * All nodes being parsed belong this loop's body
     */
    LoopASTNode *current_loop_node = nullptr;

    /**
     * constructor
     */
    CSTConverter(bool is64Bit);

    /**
     * visit the tokens, from start to end
     * @param start start is the inclusive index at which to begin visiting
     * @param end end is exclusive index at which to end visiting
     */
    void visit(std::vector<std::unique_ptr<CSTToken>> &tokens, unsigned int start, unsigned int end);

    /**
     * A helper function to visit the tokens starting at start
     */
    void visit(std::vector<std::unique_ptr<CSTToken>> &tokens, unsigned int start = 0) {
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

    /**
     * get function params for the given tokens
     */
    FunctionParamsResult function_params(cst_tokens_ref_type tokens, unsigned start = 0);

    // nodes

    void visitContinue(CompoundCSTToken *continueCst) override;

    void visitBreak(CompoundCSTToken *breakCST) override;

    void visitFunctionParam(CompoundCSTToken *param) override;

    void visitFunction(CompoundCSTToken *function) override;

    void visitVarInit(CompoundCSTToken *varInit) override;

    void visitSwitch(CompoundCSTToken *switchCst) override;

    void visitLambda(CompoundCSTToken *cst) override;

    void visitAssignment(CompoundCSTToken *assignment) override;

    void visitImport(CompoundCSTToken *importCst) override;

    void visitReturn(CompoundCSTToken *returnCst) override;

    void visitTypealias(CompoundCSTToken *alias) override;

    void visitBody(CompoundCSTToken *bodyCst) override;

    void visitIncDec(CompoundCSTToken *incDec) override;

    void visitIf(CompoundCSTToken *ifCst) override;

    void visitForLoop(CompoundCSTToken *forLoop) override;

    void visitWhile(CompoundCSTToken *whileCst) override;

    void visitDoWhile(CompoundCSTToken *doWhileCst) override;

    void visitStructDef(CompoundCSTToken *structDef) override;

    void visitInterface(CompoundCSTToken *interface) override;

    void visitImpl(CompoundCSTToken *impl) override;

    void visitTryCatch(CompoundCSTToken *tryCatch) override;

    void visitEnumDecl(CompoundCSTToken *enumDecl) override;

    // Types

    void visit(TypeToken *token) override;

    void visitPointerType(CompoundCSTToken *pointerType) override;

    void visitGenericType(CompoundCSTToken *genericType) override;

    void visitArrayType(CompoundCSTToken *arrayType) override;

    void visitFunctionType(CompoundCSTToken *functionType) override;

    // Values

    void visit(NullToken *token) override;

    void visit(StringToken *token) override;

    void visit(CharToken *token) override;

    void visit(NumberToken *token) override;

    void visitStructValue(CompoundCSTToken *structValueCst) override;

    void visitArrayValue(CompoundCSTToken *arrayValue) override;

    void visitCast(CompoundCSTToken *castCst) override;

    void visitAddrOf(CompoundCSTToken *castCst) override;

    void visitDereference(CompoundCSTToken *notCst) override;

    void visitFunctionCall(CompoundCSTToken *call) override;

    void visitIndexOp(CompoundCSTToken *op) override;

    void visit(VariableToken *token) override;

    void visit(BoolToken *token) override;

    void visitAccessChain(AccessChainCST *accessChain) override;

    void visitExpression(CompoundCSTToken *expressionCst) override;

    void visitNegative(CompoundCSTToken *negativeCst) override;

    void visitNot(CompoundCSTToken *notCst) override;

};