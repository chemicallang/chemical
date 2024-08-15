// Copyright (c) Qinetik 2024.

#pragma once

#include "integration/common/Diagnostic.h"
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

class FunctionParam;

class BaseType;



class CSTConverter;
class MembersContainer;

/**
 * a function that can handle macro
 */
typedef void(*MacroHandlerFn)(CSTConverter*, CompoundCSTToken* container);

/**
 * a function that can handle annotation
 */
typedef void(*AnnotationHandlerFn)(CSTConverter*, CSTToken* container, AnnotationKind kind);

class CSTConverter : public CSTVisitor, public CSTDiagnoser {
private:

    /**
     * are function param types optional, used to check when converting function parameters
     * since lambda parameter types are optional
     */
    bool optional_param_types = false;

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
     * enable or disable CBI
     */
    bool isCBIEnabled = true;

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
     * all nodes being parsed belong to this function type
     */
    ASTNode* parent_node = nullptr;

    /**
     * This is a pointer to current function type,
     * All nodes being parsed belong to this function type
     */
    FunctionType *current_func_type = nullptr;

    /**
     * This is a pointer to current members container
     * All nodes being pased belong to this membes container
     */
    MembersContainer* current_members_container = nullptr;

    /**
     * The current loop node
     * All nodes being parsed belong this loop's body
     */
    LoopASTNode *current_loop_node = nullptr;

    /**
     * global interpret scope
     */
    GlobalInterpretScope global_scope;

    /**
     * the target is provided to the source code
     */
    std::string target;

    /**
     * the path to file being converted, could be empty, if we are just
     * converting anonymous tokens
     */
    std::string path;

    /**
     * is code gen for 64bit
     */
    bool is64Bit;

    /**
     * constructor
     */
    CSTConverter(std::string path, bool is64Bit, std::string target);

    /**
     * the function that should be used to ask if node should be disposed
     */
    bool is_dispose();

    /**
     * pop the last node from nodes vector node
     */
    ASTNode* pop_last_node() {
        auto last = nodes.back().release();
        nodes.pop_back();
        return last;
    }

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
     * take a compound cst token of type generic arg list and convert its tokens to generic arg list
     */
    void to_generic_arg_list(std::vector<std::unique_ptr<BaseType>>& generic_list, CompoundCSTToken* container);

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

    void visitThrow(CompoundCSTToken *throwStmt) override;

    void visitUsing(CompoundCSTToken *usingStmt) override;

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

    void visitUnionDef(CompoundCSTToken *unionDef) override;

    void visitNamespace(CompoundCSTToken *ns) override;

    void visitTryCatch(CompoundCSTToken *tryCatch) override;

    void visitEnumDecl(CompoundCSTToken *enumDecl) override;

    void visitMacro(CompoundCSTToken* macroCst) override;

    void visitAnnotation(CompoundCSTToken *annotation) override;

    void visitAnnotationToken(LexToken *token) override;

    // Types

    void visitTypeToken(LexToken *token) override;

    void visitReferencedValueType(CompoundCSTToken *ref_value) override;

    void visitPointerType(CompoundCSTToken *pointerType) override;

    void visitGenericType(CompoundCSTToken *genericType) override;

    void visitSpecializedType(CompoundCSTToken *specType) override;

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

    void visitAccessChain(CompoundCSTToken *accessChain) override;

    void visitExpression(CompoundCSTToken *expressionCst) override;

    void visitNegative(CompoundCSTToken *negativeCst) override;

    void visitNot(CompoundCSTToken *notCst) override;

    ~CSTConverter();

};