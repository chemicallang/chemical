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

using cst_tokens_ref_type = std::vector<CSTToken*>&;

class FunctionParam;

class BaseType;



class CSTConverter;
class MembersContainer;

/**
 * a function that can handle macro
 */
typedef void(*MacroHandlerFn)(CSTConverter*, CSTToken* container);

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
     * puts the type on the type vector
     */
    inline void put_type(BaseType* type);

    /**
     * pop the last node from nodes vector node
     */
    inline ASTNode* pop_last_node();

    /**
     * pop the last value from values vector
     */
    inline Value* pop_last_value();

    /**
     * visit the tokens, from start to end
     * @param start start is the inclusive index at which to begin visiting
     * @param end end is exclusive index at which to end visiting
     */
    void visit(std::vector<CSTToken*> &tokens, unsigned int start, unsigned int end);

    /**
     * A helper function to visit the tokens starting at start
     */
    void visit(std::vector<CSTToken*> &tokens, unsigned int start = 0) {
        visit(tokens, start, tokens.size());
    }

    /**
     * converts the tokens, or visits all the tokens
     */
    void convert(std::vector<CSTToken*> &tokens) {
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
    void to_generic_arg_list(std::vector<std::unique_ptr<BaseType>>& generic_list, CSTToken* container);

    // nodes

    void visitContinue(CSTToken* continueCst) override;

    void visitBreak(CSTToken* breakCST) override;

    void visitFunctionParam(CSTToken* param) override;

    void visitFunction(CSTToken* function) override;

    void visitVarInit(CSTToken* varInit) override;

    void visitSwitch(CSTToken* switchCst) override;

    void visitLambda(CSTToken* cst) override;

    void visitAssignment(CSTToken* assignment) override;

    void visitImport(CSTToken* importCst) override;

    void visitThrow(CSTToken* throwStmt) override;

    void visitUsing(CSTToken* usingStmt) override;

    void visitReturn(CSTToken* returnCst) override;

    void visitTypealias(CSTToken* alias) override;

    void visitBody(CSTToken* bodyCst) override;

    void visitIncDec(CSTToken* incDec) override;

    void visitIf(CSTToken* ifCst) override;

    void visitForLoop(CSTToken* forLoop) override;

    void visitWhile(CSTToken* whileCst) override;

    void visitDoWhile(CSTToken* doWhileCst) override;

    void visitStructDef(CSTToken* structDef) override;

    void visitInterface(CSTToken* interface) override;

    void visitImpl(CSTToken* impl) override;

    void visitUnionDef(CSTToken* unionDef) override;

    void visitNamespace(CSTToken* ns) override;

    void visitTryCatch(CSTToken* tryCatch) override;

    void visitEnumDecl(CSTToken* enumDecl) override;

    void visitMacro(CSTToken* macroCst) override;

    void visitAnnotation(CSTToken* annotation) override;

    void visitAnnotationToken(CSTToken* token) override;

    void visitVariant(CSTToken* variant) override;

    void visitVariantMember(CSTToken* variant_member) override;

    void visitDestruct(CSTToken* delStmt) override;

    // Types

    void visitValueNode(CSTToken *ifCst) override;

    void visitTypeToken(CSTToken* token) override;

    void visitReferencedValueType(CSTToken* ref_value) override;

    void visitPointerType(CSTToken* pointerType) override;

    void visitGenericType(CSTToken* genericType) override;

    void visitSpecializedType(CSTToken* specType) override;

    void visitArrayType(CSTToken* arrayType) override;

    void visitFunctionType(CSTToken* functionType) override;

    // Values

    void visitNullToken(CSTToken* token) override;

    void visitStringToken(CSTToken* token) override;

    void visitCharToken(CSTToken* token) override;

    void visitNumberToken(NumberToken *token) override;

    void visitStructValue(CSTToken* structValueCst) override;

    void visitArrayValue(CSTToken* arrayValue) override;

    void visitCast(CSTToken* castCst) override;

    void visitIsValue(CSTToken* castCst) override;

    void visitAddrOf(CSTToken* castCst) override;

    void visitDereference(CSTToken* notCst) override;

    void visitFunctionCall(CSTToken* call) override;

    void visitIndexOp(CSTToken* op) override;

    void visitVariableToken(CSTToken* token) override;

    void visitBoolToken(CSTToken* token) override;

    void visitAccessChain(CSTToken* accessChain) override;

    void visitExpression(CSTToken* expressionCst) override;

    void visitNegative(CSTToken* negativeCst) override;

    void visitNot(CSTToken* notCst) override;

    ~CSTConverter();

};