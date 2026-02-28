// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include "ast/base/ast_fwd.h"

class SymbolResolver;

extern "C" {

    struct FuncDeclAttributesCBI {
        int32_t specifier;
        bool is_comptime;
        bool is_compiler_decl;
        uint8_t multi_func_index;
        int32_t inline_strategy;
        bool is_extern;
        bool is_cpp_mangle;
        bool deprecated;
        bool is_implicit;
        bool is_noReturn;
        bool is_constructor_fn;
        bool is_copy_fn;
        bool is_delete_fn;
        bool is_unsafe;
        bool is_override;
        bool has_usage;
        bool no_mangle;
        bool is_generated;
        bool std_call;
        bool dll_import;
        bool dll_export;
    };

    struct InterfaceDefinitionAttrsCBI {
        int32_t specifier;
        bool has_implementation;
        bool deprecated;
        bool is_static;
        bool is_no_mangle;
        bool is_extern;
    };

    struct TypealiasDeclAttributesCBI {
        int32_t specifier;
        bool is_comptime;
        bool deprecated;
        bool is_no_mangle;
        bool is_inlined;
    };

    int ASTAnygetAnyKind(ASTAny* any);

    uint64_t ValuegetEncodedLocation(Value* value);

    int ValuegetKind(Value* value);

    BaseType* ValuegetType(Value* value);

    uint64_t ASTNodegetEncodedLocation(ASTNode* node);

    int ASTNodegetKind(ASTNode* node);

    ASTNode* ASTNodechild(ASTNode* node, chem::string_view* name);

    int BaseTypegetKind(BaseType* type);

    int IntNTypeget_intn_type_kind(IntNType* type);

    ASTNode* LinkedTypegetLinkedNode(LinkedType* type);

    LinkedType* GenericTypegetLinkedType(GenericType* type);

    BaseType* PointerTypegetChildType(PointerType* type);

    BaseType* ReferenceTypegetChildType(ReferenceType* type);

    BaseType* TypealiasStatementgetActualType(TypealiasStatement* type);

    std::vector<FunctionParam*>* FunctionTypeget_params(FunctionType* func_type);

    Value* AccessChainas_value(AccessChain* chain);

    std::vector<Value*>* AccessChainget_values(AccessChain* chain);

    std::vector<Value*>* ArrayValueget_values(ArrayValue* value);

    std::vector<Value*>* ExpressiveStringgetValues(ExpressiveString* value);

    std::vector<Value*>* FunctionCallget_args(FunctionCall* value);

    std::vector<Value*>* FunctionCallNodeget_args(AccessChainNode* node);

    Value** IndexOperatorget_idx_ptr(IndexOperator* op);

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

    void* EmbeddedNodegetDataPtr(EmbeddedNode* node);

    void* EmbeddedValuegetDataPtr(EmbeddedValue* value);

    BaseType* FunctionDeclarationgetReturnType(FunctionDeclaration* decl);

    void FunctionParamgetName(chem::string_view* view, FunctionParam* param);

    BaseType* FunctionParamgetType(FunctionParam* param);

    void BaseDefMembergetName(chem::string_view* view, BaseDefMember* member);

    BaseType* BaseDefMembergetType(BaseDefMember* member);

    std::vector<BaseDefMember*>* StructDefinitiongetMembers(StructDefinition* def);

    std::vector<ASTNode*>* StructDefinitiongetFunctions(StructDefinition* def);

    std::vector<ASTNode*>* InterfaceDefinitiongetFunctions(InterfaceDefinition* def);

    std::vector<EnumMember*>* EnumDeclarationgetMembers(EnumDeclaration* decl);

    std::vector<BaseDefMember*>* VariantDefinitiongetMembers(VariantDefinition* def);

    void FunctionDeclarationgetName(chem::string_view* view, FunctionDeclaration* decl);

    void StructDefinitiongetName(chem::string_view* view, StructDefinition* def);

    void InterfaceDefinitiongetName(chem::string_view* view, InterfaceDefinition* def);

    void NamespacegetName(chem::string_view* view, Namespace* ns);

    void EnumDeclarationgetName(chem::string_view* view, EnumDeclaration* decl);

    void EnumMembergetName(chem::string_view* view, EnumMember* member);

    void VariantMembergetName(chem::string_view* view, VariantMember* member);

    void VariantDefinitiongetName(chem::string_view* view, VariantDefinition* def);

    void UnionDefinitiongetName(chem::string_view* view, UnionDef* def);

    void FunctionDeclarationgetAttributes(FuncDeclAttributesCBI* out, FunctionDeclaration* decl);

    void InterfaceDefinitiongetAttributes(InterfaceDefinitionAttrsCBI* out, InterfaceDefinition* def);

    void TypealiasStatementgetAttributes(TypealiasDeclAttributesCBI* out, TypealiasStatement* stmt);

    int ASTNodegetAccessSpecifier(ASTNode* node);

    void TypealiasStatementgetName(chem::string_view* view, TypealiasStatement* stmt);

    std::size_t GenericTypegetArgumentCount(GenericType* type);

    BaseType* GenericTypegetArgumentType(GenericType* type, std::size_t index);

    uint64_t GenericTypegetArgumentLocation(GenericType* type, std::size_t index);

}