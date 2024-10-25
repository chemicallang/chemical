// Copyright (c) Qinetik 2024.

#pragma once

#include "integration/common/Diagnostic.h"
#include "CSTVisitor.h"
#include "CSTDiagnoser.h"
#include <unordered_map>
#include <string>
#include "utils/fwd/functional.h"
#include "ast/base/Annotation.h"
#include "ast/base/ASTUnit.h"

#include <memory>
#include <vector>

class ASTAllocator;

class LoopASTNode;

using cst_tokens_ref_type = std::vector<CSTToken*>&;

class FunctionParam;

class BaseType;

class GlobalInterpretScope;

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

class CSTConverter : public CSTDiagnoser, public CSTVisitor {
public:

    /**
     * file id is the id of the file that is being converted
     * we use this file id providing the location manager
     */
    unsigned int file_id;

    /**
     * function parameter index
     */
    unsigned int param_index = 0;

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
     * the file level allocator is used for file level private things, that are disposed
     * instantly after file has completed
     */
    ASTAllocator& file_allocator;

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
     * is code gen for 64bit
     */
    bool is64Bit;

    /**
     * constructor
     */
    CSTConverter(
        unsigned int file_id,
        bool is64Bit,
        std::string target,
        GlobalInterpretScope& scope,
        CompilerBinder& binder,
        LocationManager& loc_man,
        ASTAllocator& global_allocator,
        ASTAllocator& local_allocator,
        ASTAllocator& file_allocator
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
     * re-puts the type you popped, without the token
     */
    inline void re_put_type(BaseType* type);

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
     * this gets the source location for the token using the location manager
     */
    uint64_t loc(CSTToken* token);

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

    void visitContinue(CSTToken* continueCst) final;

    void visitBreak(CSTToken* breakCST) final;

    void visitUnreachable(CSTToken *cst) final;

    void visitFunctionParam(CSTToken* param) final;

    void visitFunction(CSTToken* function) final;

    void visitVarInit(CSTToken* varInit) final;

    void visitSwitch(CSTToken* switchCst) final;

    void visitLambda(CSTToken* cst) final;

    void visitAssignment(CSTToken* assignment) final;

    void visitImport(CSTToken* importCst) final;

    void visitThrow(CSTToken* throwStmt) final;

    void visitUsing(CSTToken* usingStmt) final;

    void visitProvide(CSTToken *provideStmt) final;

    void visitComptimeBlock(CSTToken *block) final;

    void visitReturn(CSTToken* returnCst) final;

    void visitTypealias(CSTToken* alias) final;

    void visitBody(CSTToken* bodyCst) final;

    void visitIncDec(CSTToken* incDec) final;

    void visitIf(CSTToken* ifCst) final;

    void visitForLoop(CSTToken* forLoop) final;

    void visitInitBlock(CSTToken *initBlock) final;

    void visitUnsafeBlock(CSTToken *unsafeBlock) final;

    void visitWhile(CSTToken* whileCst) final;

    void visitDoWhile(CSTToken* doWhileCst) final;

    void visitStructDef(CSTToken* structDef) final;

    void visitInterface(CSTToken* interface) final;

    void visitImpl(CSTToken* impl) final;

    void visitUnionDef(CSTToken* unionDef) final;

    void visitNamespace(CSTToken* ns) final;

    void visitTryCatch(CSTToken* tryCatch) final;

    void visitLoopBlock(CSTToken *block) final;

    void visitEnumDecl(CSTToken* enumDecl) final;

    void visitMalformedInput(CSTToken *token) final;

    void visitMacro(CSTToken* macroCst) final;

    void visitAnnotation(CSTToken* annotation) final;

    void visitAnnotationToken(CSTToken* token) final;

    void visitVariant(CSTToken* variant) final;

    void visitVariantMember(CSTToken* variant_member) final;

    void visitDestruct(CSTToken* delStmt) final;

    // Types

    void visitValueNode(CSTToken *ifCst) final;

    void visitTypeToken(CSTToken* token) final;

    void visitLinkedValueType(CSTToken* ref_value) final;

    void visitPointerType(CSTToken* pointerType) final;

    void visitReferenceType(CSTToken* pointerType) final;

    void visitGenericType(CSTToken* genericType) final;

    void visitQualifiedType(CSTToken *qualType) final;

    void visitArrayType(CSTToken* arrayType) final;

    void visitFunctionType(CSTToken* functionType) final;

    // Values

    void visitNullToken(CSTToken* token) final;

    void visitStringToken(CSTToken* token) final;

    void visitCharToken(CSTToken* token) final;

    void visitNumberToken(NumberToken *token) final;

    void visitStructValue(CSTToken* structValueCst) final;

    void visitArrayValue(CSTToken* arrayValue) final;

    void visitCast(CSTToken* castCst) final;

    void visitIsValue(CSTToken* castCst) final;

    void visitAddrOf(CSTToken* castCst) final;

    void visitDereference(CSTToken* notCst) final;

    void visitFunctionCall(CSTToken* call) final;

    void visitIndexOp(CSTToken* op) final;

    void visitVariableToken(CSTToken* token) final;

    void visitBoolToken(CSTToken* token) final;

    void visitAccessChain(CSTToken* accessChain) final;

    void visitExpression(CSTToken* expressionCst) final;

    void visitNegative(CSTToken* negativeCst) final;

    void visitNot(CSTToken* notCst) final;

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

inline void CSTConverter::re_put_type(BaseType* type) {
    types.emplace_back(type);
}

void CSTConverter::put_type(BaseType* type, CSTToken* token) {
    types.emplace_back(type);
#ifdef LSP_BUILD
    token->any = (ASTAny*) type;
#endif
}

void CSTConverter::put_value(Value* value, CSTToken* token) {
    values.emplace_back(value);
#ifdef LSP_BUILD
    token->any = (ASTAny*) value;
#endif
}

void CSTConverter::put_node(ASTNode* node, CSTToken* token) {
    nodes.emplace_back(node);
#ifdef LSP_BUILD
    token->any = (ASTAny*) node;
#endif
}