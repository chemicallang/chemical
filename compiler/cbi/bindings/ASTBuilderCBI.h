// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "CBIUtils.h"
#include "std/chem_string_view.h"
#include "std/chem_span.h"
#include "ast/utils/Operation.h"
#include "ast/base/AccessSpecifier.h"
#include "ast/base/ast_fwd.h"

class ASTBuilder;
class BatchAllocator;
class ASTAllocator;
class TypeBuilder;

struct ValueSpan {
    Value** ptr;
    size_t size;
};

extern "C" {

    void* ASTBuilderallocate_with_cleanup(ASTBuilder* builder, std::size_t obj_size, std::size_t alignment, void* cleanup_fn);

    void ASTBuilderstore_cleanup(ASTBuilder* builder, void* obj, void* cleanup_fn);

    EmbeddedNode* ASTBuildermake_embedded_node(ASTBuilder* builder, chem::string_view* name, void* data_ptr, void* known_type_fn, void* child_res_fn, ValueSpan* chemical_values, ASTNode* parent_node, uint64_t location);

    EmbeddedValue* ASTBuildermake_embedded_value(ASTBuilder* builder, chem::string_view* name, void* data_ptr, BaseType* type, ValueSpan* chemical_values, uint64_t location);

    AnyType* ASTBuildermake_any_type(ASTBuilder* builder, uint64_t location);

    ArrayType* ASTBuildermake_array_type(ASTBuilder* builder, BaseType* elem_type, int array_size, uint64_t location);

    BoolType* ASTBuildermake_bool_type(ASTBuilder* builder, uint64_t location);

    DoubleType* ASTBuildermake_double_type(ASTBuilder* builder, uint64_t location);

    DynamicType* ASTBuildermake_dynamic_type(ASTBuilder* builder, BaseType* child_type, uint64_t location);

    FloatType* ASTBuildermake_float_type(ASTBuilder* builder, uint64_t location);

    FunctionType* ASTBuildermake_func_type(ASTBuilder* builder, BaseType* returnType, bool isVariadic, bool isCapturing, ASTNode* parent_node, uint64_t location);

    GenericType* ASTBuildermake_generic_type(ASTBuilder* builder, LinkedType* linkedType);

    LinkedType* ASTBuildermake_linked_type(ASTBuilder* builder, chem::string_view* type, ASTNode* linked, uint64_t location);

    LinkedValueType* ASTBuildermake_linked_value_type(ASTBuilder* builder, Value* value, uint64_t location);

    LiteralType* ASTBuildermake_literal_type(ASTBuilder* builder, BaseType* child_type, uint64_t location);

    PointerType* ASTBuildermake_ptr_type(ASTBuilder* builder, BaseType* child_type, uint64_t location);

    ReferenceType* ASTBuildermake_reference_type(ASTBuilder* builder, BaseType* child_type, uint64_t location);

    CharType* ASTBuilderget_char_type(ASTBuilder* builder);
    ShortType* ASTBuilderget_short_type(ASTBuilder* builder);
    IntType* ASTBuilderget_int_type(ASTBuilder* builder);
    LongType* ASTBuilderget_long_type(ASTBuilder* builder);
    LongLongType* ASTBuilderget_longlong_type(ASTBuilder* builder);

    UCharType* ASTBuilderget_uchar_type(ASTBuilder* builder);
    UShortType* ASTBuilderget_ushort_type(ASTBuilder* builder);
    UIntType* ASTBuilderget_uint_type(ASTBuilder* builder);
    ULongType* ASTBuilderget_ulong_type(ASTBuilder* builder);
    ULongLongType* ASTBuilderget_ulonglong_type(ASTBuilder* builder);

    I8Type* ASTBuilderget_i8_type(ASTBuilder* builder);
    I16Type* ASTBuilderget_i16_type(ASTBuilder* builder);
    I32Type* ASTBuilderget_i32_type(ASTBuilder* builder);
    I64Type* ASTBuilderget_i64_type(ASTBuilder* builder);
    Int128Type* ASTBuilderget_i128_type(ASTBuilder* builder);

    U8Type* ASTBuilderget_u8_type(ASTBuilder* builder);
    U16Type* ASTBuilderget_u16_type(ASTBuilder* builder);
    U32Type* ASTBuilderget_u32_type(ASTBuilder* builder);
    U64Type* ASTBuilderget_u64_type(ASTBuilder* builder);
    UInt128Type* ASTBuilderget_u128_type(ASTBuilder* builder);

    StringType* ASTBuildermake_string_type(ASTBuilder* builder, uint64_t location);

    VoidType* ASTBuildermake_void_type(ASTBuilder* builder, uint64_t location);

    AccessChain* ASTBuildermake_access_chain(ASTBuilder* builder, chem::span<ChainValue*>* values, uint64_t location);

    ASTNode* ASTBuildermake_access_chain_node(ASTBuilder* builder, chem::span<ChainValue*>* values, ASTNode* parent_node, uint64_t location);

    ValueWrapperNode* ASTBuildermake_value_wrapper(ASTBuilder* builder, Value* value, ASTNode* parent_node);

    AddrOfValue* ASTBuildermake_addr_of_value(ASTBuilder* builder, Value* value, uint64_t location);

    ArrayValue* ASTBuildermake_array_value(ASTBuilder* builder, BaseType* type, uint64_t location);

    IntNumValue* ASTBuildermake_bigint_value(ASTBuilder* builder, long long value, uint64_t location);

    BoolValue* ASTBuildermake_bool_value(ASTBuilder* builder, bool value, uint64_t location);

    CastedValue* ASTBuildermake_casted_value(ASTBuilder* builder, Value* value, BaseType* type, uint64_t location);

    IntNumValue* ASTBuildermake_char_value(ASTBuilder* builder, char value, uint64_t location);

    DereferenceValue* ASTBuildermake_dereference_value(ASTBuilder* builder, Value* value, uint64_t location);

    DoubleValue* ASTBuildermake_double_value(ASTBuilder* builder, double value, uint64_t location);

    Expression* ASTBuildermake_expression_value(ASTBuilder* builder, Value* first, Value* second, Operation op, uint64_t location);

    FloatValue* ASTBuildermake_float_value(ASTBuilder* builder, float value, uint64_t location);

    FunctionCall* ASTBuildermake_function_call_value(ASTBuilder* builder, ChainValue* parent_val, uint64_t location);

    AccessChainNode* ASTBuildermake_function_call_node(ASTBuilder* builder, ChainValue* parent_val, ASTNode* parent_node, uint64_t location);

    IndexOperator* ASTBuildermake_index_op_value(ASTBuilder* builder, ChainValue* parent_val, uint64_t location);

    IntNumValue* ASTBuildermake_int128_value(ASTBuilder* builder, uint64_t mag, bool is_neg, uint64_t location);

    IntNumValue* ASTBuildermake_int_value(ASTBuilder* builder, int value, uint64_t location);

    IsValue* ASTBuildermake_is_value(ASTBuilder* builder, Value* value, BaseType* type, bool is_negating, uint64_t location);

    LambdaFunction* ASTBuildermake_lambda_function(ASTBuilder* builder, Value* value, BaseType* type, bool isVariadic, ASTNode* parent_node, uint64_t location);

    CapturedVariable* ASTBuildermake_captured_variable(ASTBuilder* builder, chem::string_view* name, unsigned int index, bool capture_by_ref, bool mutable_ref, long value, uint64_t location);

    IntNumValue* ASTBuildermake_long_value(ASTBuilder* builder, long value, uint64_t location);

    NegativeValue* ASTBuildermake_negative_value(ASTBuilder* builder, Value* value, uint64_t location);

    NotValue* ASTBuildermake_not_value(ASTBuilder* builder, Value* value, uint64_t location);

    NullValue* ASTBuildermake_null_value(ASTBuilder* builder, uint64_t location);

    IntNumValue* ASTBuildermake_number_value(ASTBuilder* builder, uint64_t value, uint64_t location);

    IntNumValue* ASTBuildermake_short_value(ASTBuilder* builder, short value, uint64_t location);

    SizeOfValue* ASTBuildermake_sizeof_value(ASTBuilder* builder, BaseType* type, uint64_t location);

    StringValue* ASTBuildermake_string_value(ASTBuilder* builder, chem::string_view* value, uint64_t location);

    StructValue* ASTBuildermake_struct_value(ASTBuilder* builder, BaseType* ref, ASTNode* parent_node, uint64_t location);

    IntNumValue* ASTBuildermake_ubigint_value(ASTBuilder* builder, unsigned long long value, uint64_t location);

    IntNumValue* ASTBuildermake_uchar_value(ASTBuilder* builder, unsigned char value, uint64_t location);

    IntNumValue* ASTBuildermake_uint128_value(ASTBuilder* builder, uint64_t low, uint64_t high, uint64_t location);

    IntNumValue* ASTBuildermake_uint_value(ASTBuilder* builder, unsigned int value, uint64_t location);

    IntNumValue* ASTBuildermake_ulong_value(ASTBuilder* builder, unsigned long value, uint64_t location);

    IntNumValue* ASTBuildermake_ushort_value(ASTBuilder* builder, unsigned short value, uint64_t location);

    BlockValue* ASTBuildermake_block_value(ASTBuilder* builder, ASTNode* parent_node, uint64_t location);

    ValueNode* ASTBuildermake_value_node(ASTBuilder* builder, Value* value, ASTNode* parent_node, uint64_t location);

    VariableIdentifier* ASTBuildermake_identifier(ASTBuilder* builder, chem::string_view* value, ASTNode* linked, bool is_ns, uint64_t location);

    VariantCase* ASTBuildermake_variant_case(ASTBuilder* builder, VariantMember* member, SwitchStatement* stmt, uint64_t location);

    VariantCaseVariable* ASTBuildermake_variant_case_variable(ASTBuilder* builder, chem::string_view* name, VariantMemberParam* param, SwitchStatement* switch_stmt, uint64_t location);

    AssignStatement* ASTBuildermake_assignment_stmt(ASTBuilder* builder, Value* lhs, Value* rhs, Operation op, ASTNode* parent_node, uint64_t location);

    BreakStatement* ASTBuildermake_break_stmt(ASTBuilder* builder, LoopASTNode* loop_node, ASTNode* parent_node, uint64_t location);

    ContinueStatement* ASTBuildermake_continue_stmt(ASTBuilder* builder, LoopASTNode* loop_node, ASTNode* parent_node, uint64_t location);

    DestructStmt* ASTBuildermake_destruct_stmt(ASTBuilder* builder, Value* array_value, Value* ptr_value, bool is_array, ASTNode* parent_node, uint64_t location);

    ReturnStatement* ASTBuildermake_return_stmt(ASTBuilder* builder, Value* value, FunctionTypeBody* decl, ASTNode* parent_node, uint64_t location);

    //SwitchStatement* ASTBuildermake_return_stmt(ASTBuilder* builder, Value* value, FunctionType* decl, ASTNode* parent_node, uint64_t location);

    //ThrowStatement* ASTBuildermake_throw_stmt(ASTBuilder* builder, Value* value, FunctionType* decl, ASTNode* parent_node, uint64_t location);

    TypealiasStatement* ASTBuildermake_typealias_stmt(ASTBuilder* builder, chem::string_view* identifier, uint64_t name_loc, BaseType* actual_type, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    UsingStmt* ASTBuildermake_using_stmt(ASTBuilder* builder, AccessChain* chain, ASTNode* parent_node, bool is_namespace, uint64_t location);

    VarInitStatement* ASTBuildermake_varinit_stmt(ASTBuilder* builder, bool is_const, bool is_reference, chem::string_view* identifier, uint64_t id_loc, BaseType* type, Value* value, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    Scope* ASTBuildermake_scope(ASTBuilder* builder, ASTNode* parent_node, uint64_t location);

    DoWhileLoop* ASTBuildermake_do_while_loop(ASTBuilder* builder, Value* condition, ASTNode* parent_node, uint64_t location);

    EnumDeclaration* ASTBuildermake_enum_decl(ASTBuilder* builder, chem::string_view* name, uint64_t name_loc, IntNType* underlying_type, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    EnumMember* ASTBuildermake_enum_member(ASTBuilder* builder, chem::string_view* name, unsigned int index, Value* init_value, EnumDeclaration* parent_node, uint64_t location);

    ForLoop* ASTBuildermake_for_loop(ASTBuilder* builder, VarInitStatement* initializer, Value* conditionExpr, ASTNode* incrementerExpr, ASTNode* parent_node, uint64_t location);

    FunctionDeclaration* ASTBuildermake_function(ASTBuilder* builder, chem::string_view* name, uint64_t name_location, BaseType* returnType, bool isVariadic, bool hasBody, ASTNode* parent_node, uint64_t location);

    FunctionParam* ASTBuildermake_function_param(ASTBuilder* builder, chem::string_view* name, BaseType* type, unsigned int index, Value* value, bool implicit, FunctionType* decl, uint64_t location);

    GenericTypeParameter* ASTBuildermake_generic_param(ASTBuilder* builder, chem::string_view* name, BaseType* def_type, ASTNode* parent_node, unsigned int index, uint64_t location);

    IfStatement* ASTBuildermake_if_stmt(ASTBuilder* builder, Value* condition, ASTNode* parent_node, uint64_t location);

    ImplDefinition* ASTBuildermake_impl_def(ASTBuilder* builder, BaseType* interface_type, BaseType* struct_type, ASTNode* parent_node, uint64_t location);

    InitBlock* ASTBuildermake_init_block(ASTBuilder* builder, ASTNode* parent_node, uint64_t location);

    InterfaceDefinition* ASTBuildermake_interface_def(ASTBuilder* builder, chem::string_view* name, uint64_t name_loc, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    Namespace* ASTBuildermake_namespace(ASTBuilder* builder, chem::string_view* name, uint64_t name_location, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    StructDefinition* ASTBuildermake_struct_def(ASTBuilder* builder, chem::string_view* name, uint64_t name_loc, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    StructMember* ASTBuildermake_struct_member(ASTBuilder* builder, chem::string_view* name, BaseType* type, Value* defValue, bool isConst, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    UnionDef* ASTBuildermake_union_def(ASTBuilder* builder, chem::string_view* name, uint64_t name_loc, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    UnsafeBlock* ASTBuildermake_unsafe_block(ASTBuilder* builder, ASTNode* node, uint64_t location);

    WhileLoop* ASTBuildermake_while_loop(ASTBuilder* builder, Value* condition, ASTNode* node, uint64_t location);

    VariantDefinition* ASTBuildermake_variant_def(ASTBuilder* builder, chem::string_view* name, uint64_t name_loc, AccessSpecifier specifier, ASTNode* node, uint64_t location);

    VariantMember* ASTBuildermake_variant_member(ASTBuilder* builder, chem::string_view* name, VariantDefinition* parent_node, uint64_t location);

    VariantMemberParam* ASTBuildermake_variant_member_param(ASTBuilder* builder, chem::string_view* name, unsigned int index, bool is_const, BaseType* type, Value* defValue, VariantMember* parent_node, uint64_t location);

}