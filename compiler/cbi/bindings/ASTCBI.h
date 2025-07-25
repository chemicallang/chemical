// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include "ast/base/ast_fwd.h"

class SymbolResolver;

extern "C" {

    int ASTAnygetAnyKind(ASTAny* any);

    uint64_t ValuegetEncodedLocation(Value* value);

    int ValuegetKind(Value* value);

    ASTNode* ValuegetLinkedNode(Value* value);

    uint64_t ASTNodegetEncodedLocation(ASTNode* node);

    int ASTNodegetKind(ASTNode* node);

    ASTNode* ASTNodechild(ASTNode* node, chem::string_view* name);

    int BaseTypegetKind(BaseType* type);

    ASTNode* BaseTypegetLinkedNode(BaseType* type);

    std::vector<FunctionParam*>* FunctionTypeget_params(FunctionType* func_type);

    Value* AccessChainas_value(AccessChain* chain);

    std::vector<ChainValue*>* AccessChainget_values(AccessChain* chain);

    std::vector<Value*>* ArrayValueget_values(ArrayValue* value);

    std::vector<Value*>* FunctionCallget_args(FunctionCall* value);

    std::vector<Value*>* IndexOperatorget_values(IndexOperator* op);

    std::vector<FunctionParam*>* LambdaFunctionget_params(LambdaFunction* lambdaFunc);

    std::vector<CapturedVariable*>* LambdaFunctionget_capture_list(LambdaFunction* lambdaFunc);

    std::vector<ASTNode*>* LambdaFunctionget_body(LambdaFunction* lambdaFunc);

    void StructValueadd_value(StructValue* structValue, chem::string_view* name, Value* initializer);

    void VariantCaseadd_variable(VariantCase* variantCase, VariantCaseVariable* variable);

    std::vector<ASTNode*>* ScopegetNodes(Scope* scope);

    std::vector<ASTNode*>* DoWhileLoopget_body(DoWhileLoop* loop);

    std::vector<ASTNode*>* WhileLoopget_body(WhileLoop* loop);

    std::vector<ASTNode*>* ForLoopget_body(ForLoop* loop);

    void EnumDeclarationadd_member(EnumDeclaration* decl, EnumMember* member);

    std::vector<FunctionParam*>* FunctionDeclarationget_params(FunctionDeclaration* decl);

    std::vector<ASTNode*>* FunctionDeclarationadd_body(FunctionDeclaration* decl);

    std::vector<ASTNode*>* IfStatementget_body(IfStatement* stmt);

    std::vector<ASTNode*>* IfStatementadd_else_body(IfStatement* stmt);

    std::vector<ASTNode*>* IfStatementadd_else_if(IfStatement* stmt, Value* condition);

    void ImplDefinitionadd_function(ImplDefinition* definition, ASTAllocator* astAllocator, FunctionDeclaration* decl);

    void StructDefinitionadd_member(StructDefinition* definition, BaseDefMember* member);

    void StructDefinitionadd_function(StructDefinition* definition, ASTAllocator* astAllocator, FunctionDeclaration* decl);

    void InterfaceDefinitionadd_function(InterfaceDefinition* definition, ASTAllocator* astAllocator, FunctionDeclaration* decl);

    std::vector<ASTNode*>* Namespaceget_body(Namespace* ns);

    std::vector<ASTNode*>* UnsafeBlockget_body(UnsafeBlock* ub);

    std::vector<ASTNode*>* BlockValueget_body(BlockValue* bv);

    void BlockValuesetCalculatedValue(BlockValue* bv, Value* value);

    void UnionDefinitionadd_member(UnionDef* definition, BaseDefMember* member);

    void UnionDefinitionadd_function(UnionDef* definition, ASTAllocator* astAllocator, FunctionDeclaration* decl);

    void VariantDefinitionadd_member(VariantDefinition* definition, BaseDefMember* member);

    void VariantMemberadd_param(VariantMember* member, VariantMemberParam* param);

    void InitBlockadd_initializer(InitBlock* block, chem::string_view* name, Value* value);

    void* EmbeddedNodegetDataPtr(EmbeddedNode* node);

    void* EmbeddedValuegetDataPtr(EmbeddedValue* value);

}