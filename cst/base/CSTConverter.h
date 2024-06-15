// Copyright (c) Qinetik 2024.

#pragma once

#include "common/Diagnostic.h"
#include "CSTVisitor.h"
#include "CSTDiagnoser.h"
#include <unordered_map>
#include <string>
#include "utils/fwd/functional.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/Annotation.h"

#include <memory>
#include <vector>

class LoopASTNode;

using cst_tokens_ref_type = std::vector<std::unique_ptr<CSTToken>> &;

struct FunctionParamsResult {
    bool isVariadic;
    std::vector<std::unique_ptr<FunctionParam>> params;
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

    /**
     * a function that can handle macro
     */
    typedef void(*MacroHandlerFn)(CSTConverter*, CompoundCSTToken* container);

    /**
     * all the functions that can handle native macros like #eval
     */
    std::unordered_map<std::string, MacroHandlerFn> macro_handlers;

    /**
     * a function that can handle annotation
     */
    typedef void(*AnnotationHandlerFn)(CSTConverter*, CSTToken* container);

    /**
     * all the functions that can handle native annotations
     */
    std::unordered_map<std::string, AnnotationHandlerFn> annotation_handlers;

    /**
     * global interpret scope
     */
    GlobalInterpretScope global_scope;

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
     * when true, a single node is not put into nodes vector and then mode is turned off
     * it allows us to skip converting a struct or function based on an annotation
     */
    bool dispose_node = false;

    /**
     * when turned on, all nodes will not be put into nodes vector
     * it allows us to keep skipping nodes, until user asks to explicitly not dispose nodes with an annotation
     */
    bool keep_disposing = false;

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
     * collected annotations that will be applied to next struct / function
     */
    std::vector<Annotation> annotations;

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
     * initializes macro handlers
     */
    void init_macro_handlers();

    /**
     * initializes annotation handlers
     */
    void init_annotation_handlers();

    /**
     * the function that should be used to ask if node should be disposed
     */
    bool is_dispose();

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
     * consume the latest type optionally
     */
    std::optional<std::unique_ptr<BaseType>> opt_type();

    /**
     * get function params for the given tokens
     * @param start is the first parameter token index in the cst tokens vector
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

    void visitMacro(CompoundCSTToken* macroCst) override;

    void visitAnnotation(CompoundCSTToken *annotation) override;

    void visitAnnotationToken(LexToken *token) override;

    // Types

    void visitTypeToken(LexToken *token) override;

    void visitPointerType(CompoundCSTToken *pointerType) override;

    void visitGenericType(CompoundCSTToken *genericType) override;

    void visitArrayType(CompoundCSTToken *arrayType) override;

    void visitFunctionType(CompoundCSTToken *functionType) override;

    // Values

    void visitNullToken(LexToken *token) override;

    void visitStringToken(LexToken *token) override;

    void visitCharToken(LexToken *token) override;

    void visitNumberToken(NumberToken *token) override;

    void visitStructValue(CompoundCSTToken *structValueCst) override;

    void visitArrayValue(CompoundCSTToken *arrayValue) override;

    void visitCast(CompoundCSTToken *castCst) override;

    void visitAddrOf(CompoundCSTToken *castCst) override;

    void visitDereference(CompoundCSTToken *notCst) override;

    void visitFunctionCall(CompoundCSTToken *call) override;

    void visitIndexOp(CompoundCSTToken *op) override;

    void visitVariableToken(LexToken *token) override;

    void visitBoolToken(LexToken *token) override;

    void visitAccessChain(AccessChainCST *accessChain) override;

    void visitExpression(CompoundCSTToken *expressionCst) override;

    void visitNegative(CompoundCSTToken *negativeCst) override;

    void visitNot(CompoundCSTToken *notCst) override;

};