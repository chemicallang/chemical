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
#include "ast/base/ASTUnit.h"

#include <memory>
#include <vector>

class ASTAllocator;

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

class CompilerBinder;

class CSTConverter : public CSTVisitor, public CSTDiagnoser {
public:

    /**
     * function parameter index
     */
    unsigned int param_index = 0;

    /**
     * enable or disable CBI
     */
    bool isCBIEnabled = true;

    /**
     * the global allocator is used for things like function signature or struct type
     * anything allocated using global allocator is not supposed to be destructed until the code
     * for complete job (executable / dll) has been generated
     */
    ASTAllocator& global_allocator;

    /**
     * the module allocator is used for module level things, non public functions
     */
    ASTAllocator& mod_allocator;

    /**
     * local allocator is the allocator for a statement, type or value present inside a non generic,
     * non comptime or internal functions, these are not retained after module has generated code
     * we dispose these allocations after generating code for module
     */
    ASTAllocator* local_allocator;

    /**
     * top level nodes
     */
    std::vector<ASTNode*> nodes;

    /**
     * types found when visiting tokens
     */
    std::vector<BaseType*> types;

    /**
     * values found when visiting tokens
     */
    std::vector<Value*> values;

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
    GlobalInterpretScope& global_scope;

    /**
     * the reference to compiler binder
     */
    CompilerBinder& binder;

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
    CSTConverter(
        std::string path,
        bool is64Bit,
        std::string target,
        GlobalInterpretScope& scope,
        CompilerBinder& binder,
        ASTAllocator& global_allocator,
        ASTAllocator& local_allocator
    );

    /**
     * this returns the default specifier to use for the node, if user given specifier is missing
     * it checks the parent node for the specifier and also the grand parent
     */
    AccessSpecifier def_specifier(std::optional<AccessSpecifier> opt);

    /**
     * puts the type on the type vector
     */
    inline void put_type(BaseType* type, CSTToken* token);

    /**
     * puts the value in the value vector
     */
    inline void put_value(Value* value, CSTToken* token);

    /**
     * puts the node in the nodes vector
     */
    inline void put_node(ASTNode* node, CSTToken* token);

    /**
     * pop the last node from nodes vector node
     */
    inline ASTNode* pop_last_node();

    /**
     * pop the last value from values vector
     */
    inline Value* pop_last_value();

    /**
     * pop the last type from the types vector
     */
    inline BaseType* pop_last_type();

    /**
     * Take the ast unit
     */
    ASTUnit take_unit();

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
    Value* value();

    /**
     * consumes the latest (last in the vector) type from the types vector
     */
    BaseType* type();

    /**
     * take a compound cst token of type generic arg list and convert its tokens to generic arg list
     */
    void to_generic_arg_list(std::vector<BaseType*>& generic_list, CSTToken* container);

    /**
     * allocate the type locally in the module
     */
    template<typename T>
    inline T* local() {
        return local_allocator->allocate<T>();
    }

    /**
     * allocate the type globally (for the whole executable / dll)
     */
    template<typename T>
    inline T* global() {
        return global_allocator.allocate<T>();
    }

    /**
     * get the allocator for allocating given type
     * the access specifier is used to determine where it should be allocated
     */
    ASTAllocator& allocator(AccessSpecifier spec) {
        switch(spec) {
            case AccessSpecifier::Private:
            case AccessSpecifier::Protected:
            case AccessSpecifier::Internal:
                return *local_allocator;
            case AccessSpecifier::Public:
                return global_allocator;
        }
    }

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

    void visitInitBlock(CSTToken *initBlock) override;

    void visitUnsafeBlock(CSTToken *unsafeBlock) override;

    void visitWhile(CSTToken* whileCst) override;

    void visitDoWhile(CSTToken* doWhileCst) override;

    void visitStructDef(CSTToken* structDef) override;

    void visitInterface(CSTToken* interface) override;

    void visitImpl(CSTToken* impl) override;

    void visitUnionDef(CSTToken* unionDef) override;

    void visitNamespace(CSTToken* ns) override;

    void visitTryCatch(CSTToken* tryCatch) override;

    void visitLoopBlock(CSTToken *block) override;

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

    void visitLinkedValueType(CSTToken* ref_value) override;

    void visitPointerType(CSTToken* pointerType) override;

    void visitReferenceType(CSTToken* pointerType) override;

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

ASTNode* CSTConverter::pop_last_node() {
    const auto last = nodes.back();
    nodes.pop_back();
    return last;
}

Value* CSTConverter::pop_last_value() {
    const auto last = values.back();
    values.pop_back();
    return last;
}

BaseType* CSTConverter::pop_last_type() {
    const auto last = types.back();
    types.pop_back();
    return last;
}