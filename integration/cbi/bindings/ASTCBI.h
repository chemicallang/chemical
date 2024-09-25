// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>

class FunctionParam;

class GenericType;

class BaseType;

class ChainValue;

class CapturedVariable;

class BaseDefMember;

extern "C" {

    std::vector<FunctionParam*>* FunctionTypeget_params(FunctionType* func_type);

    std::vector<BaseType*>* GenericTypeget_types(GenericType* gen_type);

    std::vector<ChainValue*>* AccessChainget_values(AccessChain* chain);

    std::vector<Value*>* ArrayValueget_values(ArrayValue* value);

    void ArrayValueadd_size(ArrayValue* value, unsigned int size);

    std::vector<Value*>* FunctionCallget_args(FunctionCall* value);

    std::vector<Value*>* IndexOperatorget_values(IndexOperator* op);

    std::vector<FunctionParam*>* LambdaFunctionget_params(LambdaFunction* lambdaFunc);

    std::vector<CapturedVariable*>* LambdaFunctionget_capture_list(LambdaFunction* lambdaFunc);

    std::vector<ASTNode*>* LambdaFunctionget_body(LambdaFunction* lambdaFunc);

    std::vector<BaseType*>* StructValueget_generic_list(StructValue* gen_type);

    void StructValueadd_value(StructValue* structValue, chem::string* name, StructMemberInitializer* initializer);

    void VariantCaseadd_variable(VariantCase* variantCase, VariantCaseVariable* variable);

    std::vector<ASTNode*>* DoWhileLoopget_body(DoWhileLoop* loop);

    std::vector<ASTNode*>* WhileLoopget_body(WhileLoop* loop);

    std::vector<ASTNode*>* ForLoopget_body(ForLoop* loop);

    void EnumDeclarationadd_member(EnumDeclaration* decl, EnumMember* member);

    std::vector<FunctionParam*>* FunctionDeclarationget_params(FunctionDeclaration* decl);

    std::vector<GenericTypeParameter*>* FunctionDeclarationget_generic_params(FunctionDeclaration* decl);

    std::vector<ASTNode*>* FunctionDeclarationadd_body(FunctionDeclaration* decl);

    std::vector<ASTNode*>* IfStatementget_body(IfStatement* stmt);

    std::vector<ASTNode*>* IfStatementadd_else_body(IfStatement* stmt);

    std::vector<ASTNode*>* IfStatementadd_else_if(IfStatement* stmt, Value* condition);

    void ImplDefinitionadd_function(ImplDefinition* definition, FunctionDeclaration* decl);

    void StructDefinitionadd_member(StructDefinition* definition, chem::string* name, BaseDefMember* member);

    void StructDefinitionadd_function(StructDefinition* definition, FunctionDeclaration* decl);

    std::vector<GenericTypeParameter*>* StructDefinitionget_generic_params(StructDefinition* definition);

    void InterfaceDefinitionadd_function(InterfaceDefinition* definition, FunctionDeclaration* decl);

    std::vector<ASTNode*>* Namespaceget_children(Namespace* ns);

    std::vector<ASTNode*>* UnsafeBlockget_children(UnsafeBlock* ub);

    void UnionDefinitionadd_member(UnionDef* definition, chem::string* name, BaseDefMember* member);

    void UnionDefinitionadd_function(UnionDef* definition, FunctionDeclaration* decl);

    std::vector<GenericTypeParameter*>* UnionDefinitionget_generic_params(UnionDef* definition);

    void VariantDefinitionadd_member(VariantDefinition* definition, chem::string* name, BaseDefMember* member);

    void VariantMemberadd_param(VariantMember* member, VariantMemberParam* param);

    void InitBlockadd_initializer(InitBlock* block, chem::string* name, bool is_inherited_type, Value* value);

}