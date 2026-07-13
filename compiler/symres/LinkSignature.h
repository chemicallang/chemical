// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "SymbolResolver.h"
#include "preprocess/visitors/RecursiveVisitor.h"

class TopLevelLinkSignature : public RecursiveVisitor<TopLevelLinkSignature> {
public:

    /**
     * the instnace of symbol resolver
     */
    SymbolResolver& linker;

    /**
     * the diagnoser is used to store diagnostics
     */
    ASTDiagnoser diagnoser;

    /**
     * this is set before visiting any type
     */
    SourceLocation type_location = 0;

    /**
     * per-file local symbol table — holds file-private symbols (generic type params,
     * using-imports, aliases) that die when this context is destroyed
     */
    SymbolTable table;

    /**
     * we own a generic instantiator for each file
     */
    GenericInstantiatorAPI generic_instantiator;

    /**
     * internal flag to detect comptime context
     */
    bool comptime_context = false;
    /**
     * internal flag to detect safe context
     */
    bool safe_context = true;
    /**
     * internal flag to detect generic context
     */
    bool generic_context = false;

    /**
     * this requires that all types that are linked
     * with structs, variants, unions, type aliases be public
     */
    bool require_exported = false;

    /**
     * constructor
     */
    TopLevelLinkSignature(
        SymbolResolver& resolver
    ) : linker(resolver), diagnoser(resolver.loc_man), generic_instantiator(
        resolver.controller, resolver.binder, resolver.child_resolver,
        resolver.instContainer, resolver.coreNodes, resolver.implsIndex, resolver.generic_inst_reg_mutex,
        *resolver.ast_allocator, diagnoser, resolver.comptime_scope.typeBuilder, resolver.comptime_scope.target_data
    ) {

    }

    // -------------- non const references -------------



    // -------------- const references ----------------

    inline const ChildResolver* getChildResolver() {
        return &linker.child_resolver;
    }

    inline const TypeBuilder& getTypeBuilder() {
        return linker.comptime_scope.typeBuilder;
    }

    inline const TargetData& getTargetData() {
        return linker.comptime_scope.target_data;
    }

    inline const CoreNodes& getCoreNodes() {
        return linker.coreNodes;
    }

    inline const ImplementationsIndex& getImplsIndex() {
        return linker.implsIndex;
    }

    inline const CompilerBinder& getCompilerBinder() {
        return linker.binder;
    }

    inline ASTAllocator& getAstAllocator() {
        return *linker.ast_allocator;
    }

    inline ASTAllocator& getModAllocator() {
        return *linker.mod_allocator;
    }

    inline GenericInstantiatorAPI& getGenericInstantiatorAPI() {
        return generic_instantiator;
    }

    inline ASTNode* tld_find(const chem::string_view& name) {
        auto* result = table.resolve(name);
        if(result) return result;
        return linker.find(name);
    }

    inline UnresolvedDecl* get_unresolved_decl() {
        return linker.get_unresolved_decl();
    }

    void link_param(GenericTypeParameter* param);

    // TODO: we don't want to override this
    // function types require that signature resolved is set to true
    // which by default is false
    void VisitFunctionType(FunctionType* type) {
        RecursiveVisitor<TopLevelLinkSignature>::VisitFunctionType(type);
        // TODO: remove this method
        type->data.signature_resolved = true;
    }

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
        VisitTypeNoNullCheck(type_ref);
    }
    inline void visit(TypeLoc& type) {
        type_location = type.encoded_location();
        VisitTypeNoNullCheck(const_cast<BaseType*>(type.getType()));
    }
    inline void visit(LinkedType*& type_ref) {
        visit((BaseType*&) type_ref);
    }
    inline void visit(Scope& scope) {
        VisitScope(&scope);
    }

    void VisitVariableIdentifier(VariableIdentifier* value);

    void VisitFunctionCall(FunctionCall* value);

    void VisitLinkedType(LinkedType* type);

    void VisitGenericType(GenericType* type);

    void VisitArrayType(ArrayType* type);

    void VisitAccessChain(AccessChain* value);

    void VisitExpression(Expression* value);

    void VisitAddrOfValue(AddrOfValue* value);

    void VisitReferenceOfValue(ReferenceOfValue* value);

    void VisitArrayValue(ArrayValue* value);

    void VisitComptimeValue(ComptimeValue* value);

    void VisitDereferenceValue(DereferenceValue* value);

    void VisitIncDecValue(IncDecValue* value);

    void VisitIndexOperator(IndexOperator* value);

    void VisitIsValue(IsValue* value);

    void VisitLambdaFunction(LambdaFunction* value);

    void VisitNegativeValue(NegativeValue* value);

    void VisitUnsafeValue(UnsafeValue* value);

    void VisitNewValue(NewValue* value);

    void VisitNewTypedValue(NewTypedValue* value);

    void VisitPlacementNewValue(PlacementNewValue* value);

    void VisitNotValue(NotValue* value);

    void VisitBitwiseNot(BitwiseNot* value);

    void VisitPatternMatchExpr(PatternMatchExpr* value);

    void VisitBlockValue(BlockValue* value);

    void VisitStructValue(StructValue* value);

    void VisitEmbeddedNode(EmbeddedNode* node);

    void VisitEmbeddedValue(EmbeddedValue* value);

    void VisitComptimeBlock(ComptimeBlock* node);

    void LinkVariablesNoScope(VariablesContainerBase* container);

    void LinkMembersContainerNoScope(MembersContainer* container);

    void LinkMembersContainerNoScopeExposed(MembersContainer* container);

    void LinkVariables(VariablesContainerBase* container);

    void LinkMembersContainer(MembersContainer* container);

    void LinkMembersContainerExposed(MembersContainer* container);

    void LinkMembersContainer(MembersContainer* container, AccessSpecifier specifier);

    void VisitUsingStmt(UsingStmt* node);

    void VisitAliasStmt(AliasStmt* node);

    void VisitVarInitStmt(VarInitStatement* node);

    void VisitTypealiasStmt(TypealiasStatement* node);

    void VisitFunctionDecl(FunctionDeclaration* node);

    void VisitEnumDecl(EnumDeclaration* node);

    void VisitGenericTypeDecl(GenericTypeDecl* node);

    void VisitGenericFuncDecl(GenericFuncDecl* node);

    void VisitGenericStructDecl(GenericStructDecl* node);

    void VisitGenericUnionDecl(GenericUnionDecl* node);

    void VisitGenericInterfaceDecl(GenericInterfaceDecl* node);

    void VisitGenericVariantDecl(GenericVariantDecl* node);

    void VisitGenericImplDecl(GenericImplDecl* node);

    void VisitIfStmt(IfStatement* node);

    void VisitImplDecl(ImplDefinition* node);

    void VisitInterfaceDecl(InterfaceDefinition* node);

    void VisitNamespaceDecl(Namespace* node);

    void VisitScope(Scope* node);

    void VisitUnnamedStruct(UnnamedStruct* node);

    void VisitStructDecl(StructDefinition* node);

    void VisitUnionDecl(UnionDef* node);

    void VisitVariantDecl(VariantDefinition* node);

    void VisitVariantMember(VariantMember* node);

    void VisitUnnamedUnion(UnnamedUnion* node);

};