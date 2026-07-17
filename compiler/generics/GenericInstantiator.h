// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "preprocess/visitors/RecursiveVisitor.h"
#include "compiler/symres/SymbolTable.h"
#include "compiler/generics/InstantiationsContainer.h"
#include <mutex>
#include <unordered_map>
#include <optional>

class CompilerBinder;

class GenericInstantiator : public RecursiveVisitor<GenericInstantiator> {
public:

    /**
     * annotation controller can be used to access the annotated data
     */
    AnnotationController& controller;

    /**
     * instantiations container contains types for each instantiation
     */
    InstantiationsContainer& container;

    /**
     * the allocator which allows to allocate memory for all instantiations
     */
    ASTAllocator* allocator_ptr;

    /**
     * the diagnoser to report errors
     */
    ASTDiagnoser& diagnoser;

    /**
     * the symbol table to use for declaring and resolving nodes
     */
    SymbolTable table;

    /**
     * the compiler binder is used to to replace nested values in embedded nodes and values
     */
    CompilerBinder& binder;

    /**
     * child resolver is used to fix children
     */
    ChildResolver& child_resolver;

    /**
     * a reference to type builder
     */
    TypeBuilder& typeBuilder;

    /**
     * the nodes in core module are used for getting interfaces
     */
    CoreNodes& coreNodes;

    /**
     * we lookup impl blocks from this
     * required for determining type of overloaded operators by getting
     * exact methods from implementations of interfaces for different types
     */
    ImplementationsIndex& implsIndex;

    /**
     * the registration mutex (recursive) is used to register instantiations
     * and finalize signatures atomically, so they can be used among threads safely.
     * recursive because signature finalization may recursively register new generic types.
     */
    std::recursive_mutex& registration_mutex;

    /**
     * this points to the node being instantiated
     * this allows us to check self-referential pointers to generic decls
     */
    BaseGenericDecl* current_gen = nullptr;

    /**
     * allows relinking generic type pointers to it's new implementation
     */
    ASTNode* current_impl_ptr = nullptr;

    /**
     * this is stored to check which function we are currently in
     * to check if private members are accessible
     */
    FunctionTypeBody* current_func_type = nullptr;

    /**
     * type location is tracked for diagnostics
     */
    SourceLocation type_location = 0;

    /**
     * thread-local map of active generic type parameter -> concrete type.
     * replaces direct mutation of GenericTypeParam::active_type to avoid
     * data races when multiple threads finalize different instantiations
     * of the same generic declaration concurrently.
     */
    std::unordered_map<GenericTypeParameter*, BaseType*> active_type_map;

    /**
     * requirement is set by the public apis to require, what is required of nested generic types
     * in debug mode we use optional which causes failures if not set explicitly
     */
#ifdef DEBUG
    std::optional<InstantiationRequirement> _requirement = std::nullopt;
#else
    InstantiationRequirement _requirement = InstantiationRequirement::Registration;
#endif

    /**
     * target data
     */
    TargetData& targetData;

    /**
     * constructor
     * the allocator must be an ast allocator
     */
    GenericInstantiator(
        AnnotationController& controller,
        CompilerBinder& binder,
        ChildResolver& child_resolver,
        InstantiationsContainer& container,
        CoreNodes& coreNodes,
        ImplementationsIndex& implsIndex,
        std::recursive_mutex& registration_mutex,
        ASTAllocator& allocator,
        ASTDiagnoser& diagnoser,
        TypeBuilder& typeBuilder,
        TargetData& targetData
    ) : controller(controller), child_resolver(child_resolver), binder(binder), container(container),
        allocator_ptr(&allocator), diagnoser(diagnoser), typeBuilder(typeBuilder), targetData(targetData),
        coreNodes(coreNodes), implsIndex(implsIndex), registration_mutex(registration_mutex) {

    }

    static inline GenericInstantiator newGenericInstantiatorFrom(const GenericInstantiator& g) {
        return GenericInstantiator(
        g.controller, g.binder, g.child_resolver,
        g.container, g.coreNodes, g.implsIndex, g.registration_mutex,
        *g.allocator_ptr, g.diagnoser, g.typeBuilder, g.targetData
        );
    }

    inline ASTAllocator& getAllocator() {
        return *allocator_ptr;
    }

    inline void setAllocator(ASTAllocator& allocator) {
        allocator_ptr = &allocator;
    }

    inline InstantiationRequirement getRequirement() {
#ifdef DEBUG
        // this would fail, if we explicitly don't set requirement
        return _requirement.value();
#else
        return _requirement;
#endif
    }

    inline void setRequirement(InstantiationRequirement req) {
        _requirement = req;
    }

    inline void debug_unsetRequirement() {
#ifdef DEBUG
        _requirement = std::nullopt;
#endif
    }

    void make_gen_type_concrete(BaseType*& type);

    bool relink_identifier(VariableIdentifier* identifier);

    // We want to override visit, what we want is a BaseType*& so we can replace
    // every BaseType*& with the appropriate concrete implementation if it's referencing a generic type

    template<typename T>
    inline void visit(T* ptr) {
        VisitByPtrTypeNoNullCheck(ptr);
    }
    inline void visit(ASTNode* node) {
        VisitNodeNoNullCheck(node);
    }
    inline void visit(BaseDefMember* node) {
        VisitNodeNoNullCheck(node);
    }
    inline void visit(Value* value) {
        VisitValueNoNullCheck(value);
    }
    inline void visit(BaseType*& type_ref) {
        type_location = 0;
        GenericInstantiator::make_gen_type_concrete(type_ref);
        VisitTypeNoNullCheck(type_ref);
    }
    inline void visit(TypeLoc& type) {
        type_location = type.getLocation();
        auto changed = const_cast<BaseType*>(type.getType());
        GenericInstantiator::make_gen_type_concrete(changed);
        VisitTypeNoNullCheck(changed);
        if(changed != type.getType()) {
            type = { changed, type.getLocation() };
        }
    }
    inline void visit(LinkedType*& type_ref) {
        visit((BaseType*&) type_ref);
    }
    inline void visit(Scope& scope) {
        VisitScope(&scope);
    }

    void VisitLinkedType(LinkedType* type);

    void VisitGenericType(GenericType* type);

    void VisitStructValue(StructValue *val);

    void VisitScope(Scope* node) {
        table.scope_start();
        RecursiveVisitor<GenericInstantiator>::VisitScope(node);
        table.scope_end();
    }

    void VisitVarInitStmt(VarInitStatement* node) {
        RecursiveVisitor<GenericInstantiator>::VisitVarInitStmt(node);
        table.declare(node->name_view(), node);
    }

    void VisitTypealiasStmt(TypealiasStatement* node) {
        RecursiveVisitor<GenericInstantiator>::VisitTypealiasStmt(node);
        table.declare(node->name_view(), node);
    }

    void VisitAssignmentStmt(AssignStatement *assign);

    void VisitLambdaFunction(LambdaFunction *func);

    void VisitSwitchStmt(SwitchStatement* node);

    inline void VisitVariableIdentifier(VariableIdentifier* value) {
        relink_identifier(value);
    }

    void VisitPatternMatchExpr(PatternMatchExpr* value);

    void VisitAccessChain(AccessChain* value);

    void VisitFunctionCall(FunctionCall *call);

    void VisitIsValue(IsValue* value);

    void VisitIfValue(IfValue* value);

    void VisitSwitchValue(SwitchValue* value);

    void VisitLoopValue(LoopValue* value);

    void VisitComptimeValue(ComptimeValue* value);

    void VisitIncDecValue(IncDecValue* value);

    void VisitAddrOfValue(AddrOfValue* value);

    void VisitReferenceOfValue(ReferenceOfValue* value);

    void VisitDereferenceValue(DereferenceValue* value);

    void VisitRuntimeValue(RuntimeValue* value);

    void VisitExpression(Expression *expr);

    void VisitIndexOperator(IndexOperator* value);

    void VisitNegativeValue(NegativeValue* value);

    void VisitUnsafeValue(UnsafeValue* value);

    void VisitNewValue(NewValue *value);

    void VisitPlacementNewValue(PlacementNewValue *value);

    void VisitNotValue(NotValue* value);

    void VisitBitwiseNot(BitwiseNot* value);

    void activateIteration(BaseGenericDecl* gen_decl, size_t itr);

    /**
     * wait until the signature at decl->instantiation_statuses[index] is finalized
     */
    void waitSignatureFinalized(BaseGenericDecl* decl, size_t index);

    /**
     * mark the signature as finalized and notify waiters
     */
    void notifySignatureFinalized(BaseGenericDecl* decl, size_t index);

    void FinalizeSignature(TypealiasStatement* decl);

    void FinalizeSignature(GenericTypeDecl* decl, TypealiasStatement* impl, std::vector<TypeLoc>& generic_args);

    void FinalizeSignature(GenericTypeDecl* gen_decl, TypealiasStatement* decl, size_t itr);

    /**
     * this function allows to finalize the signature of a non generic function that is inside
     * a generic struct / variable / union
     */
    void FinalizeSignature(FunctionDeclaration* decl);

    void FinalizeSignature(GenericFuncDecl* gen_decl, FunctionDeclaration* decl, size_t itr);

    void FinalizeBody(GenericFuncDecl* gen_decl, FunctionDeclaration* decl, size_t itr);

    void FinalizeSignature(GenericStructDecl* gen_decl, StructDefinition* decl, size_t itr);

    void FinalizeBody(GenericStructDecl* gen_decl, StructDefinition* decl, size_t itr);

    void FinalizeSignature(GenericUnionDecl* gen_decl, UnionDef* decl, size_t itr);

    void FinalizeBody(GenericUnionDecl* gen_decl, UnionDef* decl, size_t itr);

    void FinalizeSignature(GenericInterfaceDecl* gen_decl, InterfaceDefinition* decl, size_t itr);

    void FinalizeBody(GenericInterfaceDecl* gen_decl, InterfaceDefinition* decl, size_t itr);

    void FinalizeSignature(GenericVariantDecl* gen_decl, VariantDefinition* decl, size_t itr);

    void FinalizeBody(GenericVariantDecl* gen_decl, VariantDefinition* decl, size_t itr);

    void FinalizeNestedImplSignature(ImplDefinition* def);

    void FinalizeNestedImplBody(ImplDefinition* def);

    void FinalizeSignature(GenericImplDecl* gen_decl, ImplDefinition* decl, size_t itr);

    void FinalizeBody(GenericImplDecl* gen_decl, ImplDefinition* decl, size_t itr);

    /**
     * clears the symbols from tables, from previous finalization
     */
    void Clear();

    void FinalizeSignature(GenericTypeDecl* decl, const std::span<TypealiasStatement*>& instantiations);

    void FinalizeSignature(GenericFuncDecl* decl, const std::span<FunctionDeclaration*>& instantiations);

    void FinalizeBody(GenericFuncDecl* decl, const std::span<FunctionDeclaration*>& instantiations);

    void FinalizeSignature(GenericStructDecl* decl, const std::span<StructDefinition*>& instantiations);

    void FinalizeBody(GenericStructDecl* decl, const std::span<StructDefinition*>& instantiations);

    void FinalizeSignature(GenericUnionDecl* decl, const std::span<UnionDef*>& instantiations);

    void FinalizeBody(GenericUnionDecl* decl, const std::span<UnionDef*>& instantiations);

    void FinalizeSignature(GenericInterfaceDecl* decl, const std::span<InterfaceDefinition*>& instantiations);

    void FinalizeBody(GenericInterfaceDecl* decl, const std::span<InterfaceDefinition*>& instantiations);

    void FinalizeSignature(GenericVariantDecl* decl, const std::span<VariantDefinition*>& instantiations);

    void FinalizeBody(GenericVariantDecl* decl, const std::span<VariantDefinition*>& instantiations);

    void FinalizeSignature(GenericImplDecl* decl, const std::span<ImplDefinition*>& instantiations);

    void FinalizeBody(GenericImplDecl* decl, const std::span<ImplDefinition*>& instantiations);

};