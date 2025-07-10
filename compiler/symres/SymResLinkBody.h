// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "preprocess/visitors/RecursiveVisitor.h"

class SymResLinkBody : public RecursiveVisitor<SymResLinkBody> {
public:

    SymbolResolver& linker;

    /**
     * this is set before visiting type
     */
    SourceLocation type_location = 0;

    /**
     * constructor
     */
    SymResLinkBody(SymbolResolver& linker) : linker(linker) {

    }

    void LinkMembersContainerNoScope(MembersContainer* container);

    void LinkMembersContainer(MembersContainer* container) {
        linker.scope_start();
        LinkMembersContainerNoScope(container);
        linker.scope_end();
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
    inline void visit(ChainValue* value) {
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

//    void VisitAccessChain(AccessChain *chain);

    void VisitAssignmentStmt(AssignStatement *assign);

    void VisitUsingStmt(UsingStmt* node);

    void VisitBreakStmt(BreakStatement* node);

    void VisitDeleteStmt(DestructStmt* node);

    void VisitProvideStmt(ProvideStmt* node);

    void VisitReturnStmt(ReturnStatement* node);

    void VisitSwitchStmt(SwitchStatement *stmt);

    void VisitTypealiasStmt(TypealiasStatement* node);

    void VisitVarInitStmt(VarInitStatement* node);

    void VisitComptimeBlock(ComptimeBlock* node);

    void VisitDoWhileLoopStmt(DoWhileLoop* node);

    void VisitEnumMember(EnumMember* node);

    void VisitEnumDecl(EnumDeclaration* node);

    void VisitForLoopStmt(ForLoop* node);

    void VisitFunctionParam(FunctionParam* node);

    void VisitGenericTypeParam(GenericTypeParameter* node);

    void VisitFunctionDecl(FunctionDeclaration* node);

    void VisitInterfaceDecl(InterfaceDefinition* node);

    void VisitStructDecl(StructDefinition* node);

    void VisitVariantDecl(VariantDefinition* node);

    void VisitCapturedVariable(CapturedVariable* node);

    void VisitGenericFuncDecl(GenericFuncDecl* node);

    void VisitGenericImplDecl(GenericImplDecl* node);

    void VisitGenericInterfaceDecl(GenericInterfaceDecl* node);

    void VisitGenericStructDecl(GenericStructDecl* node);

    void VisitGenericUnionDecl(GenericUnionDecl* node);

    void VisitGenericVariantDecl(GenericVariantDecl* node);

    void VisitIfStmt(IfStatement* node);

    void VisitImplDecl(ImplDefinition* node);

    void VisitNamespaceDecl(Namespace* node);

    void VisitScope(Scope* node);

    void VisitLoopBlock(LoopBlock* node);

    void VisitInitBlock(InitBlock* node);

    void VisitUnionDecl(UnionDef* node);

    void VisitUnsafeBlock(UnsafeBlock* node);

    void VisitVariantCaseVariable(VariantCaseVariable* node);

    void VisitWhileLoopStmt(WhileLoop* node);

    void VisitValueNode(ValueNode* node);

    void VisitMultiFunctionNode(MultiFunctionNode* node);

    void VisitValueWrapper(ValueWrapperNode* node);

    void VisitEmbeddedNode(EmbeddedNode* node);

};