// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "CBIUtils.h"
#include "std/chem_string_view.h"
#include "ast/utils/Operation.h"
#include "ast/base/AccessSpecifier.h"

class Value;

class BaseType;

class ASTNode;

class ChainValue;

class BatchAllocator;
class ASTAllocator;

class AnyType;
class ArrayType;
class BigIntType;
class BoolType;
class BoolType;
class DoubleType;
class DynamicType;
class FloatType;
class FunctionType;
class FunctionTypeBody;
class GenericType;
class Int128Type;
class IntType;
class IntNType;
class LinkedType;
class LinkedValueType;
class LiteralType;
class LongType;
class PointerType;
class ReferenceType;
class ShortType;
class StringType;
class UBigIntType;
class UCharType;
class UInt128Type;
class UIntType;
class ULongType;
class UShortType;
class VoidType;
class AccessChain;
class AddrOfValue;
class ArrayValue;
class BigIntValue;
class BoolValue;
class CastedValue;
class CharValue;
class DereferenceValue;
class DoubleValue;
class Expression;
class FloatValue;
class FunctionCall;
class IndexOperator;
class Int128Value;
class IntValue;
class IsValue;
class LambdaFunction;
class LongValue;
class NegativeValue;
class NotValue;
class NullValue;
class NumberValue;
class ShortValue;
class SizeOfValue;
class StringValue;
class StructMemberInitializer;
class StructValue;
class UBigIntValue;
class UCharValue;
class UInt128Value;
class UIntValue;
class ULongValue;
class UShortValue;
class ValueNode;
class VariableIdentifier;
class VariantCall;
class VariantCase;
class VariantCaseVariable;
class AssignStatement;
class BreakStatement;
class Comment;
class ContinueStatement;
class DestructStmt;
class SwitchStatement;
class LoopASTNode;
class ReturnStatement;
class TypealiasStatement;
class UsingStmt;
class VarInitStatement;
class Scope;
class ValueWrapperNode;
class DoWhileLoop;
class EnumDeclaration;
class EnumMember;
class ForLoop;
class FunctionDeclaration;
class FunctionParam;
class CapturedVariable;
class GenericTypeParameter;
class IfStatement;
class ImplDefinition;
class BlockValue;
class InitBlock;
class InterfaceDefinition;
class Namespace;
class StructDefinition;
class StructMember;
class UnionDef;
class UnsafeBlock;
class WhileLoop;
class VariantDefinition;
class VariantMember;
class VariantMemberParam;
class SymResNode;
class SymResValue;

extern "C" {

    void* ASTBuilderallocate_with_cleanup(ASTAllocator* allocator, std::size_t obj_size, std::size_t alignment, void* cleanup_fn);

    BaseType* ASTBuildercreateValueType(ASTAllocator* allocator, ASTNode* node);

    BaseType* ASTBuildercreateType(ASTAllocator* allocator, Value* value);

    SymResNode* ASTBuildermake_sym_res_node(ASTAllocator* allocator, void* decl_fn, void* repl_fn, void* data_ptr, ASTNode* parent_node, uint64_t location);

    SymResValue* ASTBuildermake_sym_res_value(ASTAllocator* allocator, void* repl_fn, void* data_ptr, uint64_t location);

    AnyType* ASTBuildermake_any_type(ASTAllocator* allocator, uint64_t location);

    ArrayType* ASTBuildermake_array_type(ASTAllocator* allocator, BaseType* elem_type, int array_size, uint64_t location);

    BigIntType* ASTBuildermake_bigint_type(ASTAllocator* allocator, uint64_t location);

    BoolType* ASTBuildermake_bool_type(ASTAllocator* allocator, uint64_t location);

    BoolType* ASTBuildermake_char_type(ASTAllocator* allocator, uint64_t location);

    DoubleType* ASTBuildermake_double_type(ASTAllocator* allocator, uint64_t location);

    DynamicType* ASTBuildermake_dynamic_type(ASTAllocator* allocator, BaseType* child_type, uint64_t location);

    FloatType* ASTBuildermake_float_type(ASTAllocator* allocator, uint64_t location);

    FunctionType* ASTBuildermake_func_type(ASTAllocator* allocator, BaseType* returnType, bool isVariadic, bool isCapturing, ASTNode* parent_node, uint64_t location);

    GenericType* ASTBuildermake_generic_type(ASTAllocator* allocator, LinkedType* linkedType);

    Int128Type* ASTBuildermake_int128_type(ASTAllocator* allocator, uint64_t location);

    IntType* ASTBuildermake_int_type(ASTAllocator* allocator, uint64_t location);

    LinkedType* ASTBuildermake_linked_type(ASTAllocator* allocator, chem::string_view* type, ASTNode* linked, uint64_t location);

    LinkedValueType* ASTBuildermake_linked_value_type(ASTAllocator* allocator, Value* value, uint64_t location);

    LiteralType* ASTBuildermake_literal_type(ASTAllocator* allocator, BaseType* child_type, uint64_t location);

    LongType* ASTBuildermake_long_type(ASTAllocator* allocator, bool is64Bit, uint64_t location);

    PointerType* ASTBuildermake_ptr_type(ASTAllocator* allocator, BaseType* child_type, uint64_t location);

    ReferenceType* ASTBuildermake_reference_type(ASTAllocator* allocator, BaseType* child_type, uint64_t location);

    ShortType* ASTBuildermake_short_type(ASTAllocator* allocator, uint64_t location);

    StringType* ASTBuildermake_string_type(ASTAllocator* allocator, uint64_t location);

    UBigIntType* ASTBuildermake_ubigint_type(ASTAllocator* allocator, uint64_t location);

    UCharType* ASTBuildermake_uchar_type(ASTAllocator* allocator, uint64_t location);

    UInt128Type* ASTBuildermake_uint128_type(ASTAllocator* allocator, uint64_t location);

    UIntType* ASTBuildermake_uint_type(ASTAllocator* allocator, uint64_t location);

    ULongType* ASTBuildermake_ulong_type(ASTAllocator* allocator, bool is64Bit, uint64_t location);

    UShortType* ASTBuildermake_ushort_type(ASTAllocator* allocator, uint64_t location);

    VoidType* ASTBuildermake_void_type(ASTAllocator* allocator, uint64_t location);

    AccessChain* ASTBuildermake_access_chain(ASTAllocator* allocator, bool is_node, uint64_t location);

    ValueWrapperNode* ASTBuildermake_value_wrapper(ASTAllocator* allocator, Value* value, ASTNode* parent_node);

    AddrOfValue* ASTBuildermake_addr_of_value(ASTAllocator* allocator, Value* value, uint64_t location);

    ArrayValue* ASTBuildermake_array_value(ASTAllocator* allocator, BaseType* type, uint64_t location);

    BigIntValue* ASTBuildermake_bigint_value(ASTAllocator* allocator, long long value, uint64_t location);

    BoolValue* ASTBuildermake_bool_value(ASTAllocator* allocator, bool value, uint64_t location);

    CastedValue* ASTBuildermake_casted_value(ASTAllocator* allocator, Value* value, BaseType* type, uint64_t location);

    CharValue* ASTBuildermake_char_value(ASTAllocator* allocator, char value, uint64_t location);

    DereferenceValue* ASTBuildermake_dereference_value(ASTAllocator* allocator, Value* value, uint64_t location);

    DoubleValue* ASTBuildermake_double_value(ASTAllocator* allocator, double value, uint64_t location);

    Expression* ASTBuildermake_expression_value(ASTAllocator* allocator, Value* first, Value* second, Operation op, bool is64Bit, uint64_t location);

    FloatValue* ASTBuildermake_float_value(ASTAllocator* allocator, float value, uint64_t location);

    FunctionCall* ASTBuildermake_function_call_value(ASTAllocator* allocator, ChainValue* parent_val, uint64_t location);

    IndexOperator* ASTBuildermake_index_op_value(ASTAllocator* allocator, ChainValue* parent_val, uint64_t location);

    Int128Value* ASTBuildermake_int128_value(ASTAllocator* allocator, uint64_t mag, bool is_neg, uint64_t location);

    IntValue* ASTBuildermake_int_value(ASTAllocator* allocator, int value, uint64_t location);

    IsValue* ASTBuildermake_is_value(ASTAllocator* allocator, Value* value, BaseType* type, bool is_negating, uint64_t location);

    LambdaFunction* ASTBuildermake_lambda_function(ASTAllocator* allocator, Value* value, BaseType* type, bool isVariadic, ASTNode* parent_node, uint64_t location);

    CapturedVariable* ASTBuildermake_captured_variable(ASTAllocator* allocator, chem::string_view* name, unsigned int index, bool capture_by_ref, long value, uint64_t location);

    LongValue* ASTBuildermake_long_value(ASTAllocator* allocator, long value, bool is64Bit, uint64_t location);

    NegativeValue* ASTBuildermake_negative_value(ASTAllocator* allocator, Value* value, uint64_t location);

    NotValue* ASTBuildermake_not_value(ASTAllocator* allocator, Value* value, uint64_t location);

    NullValue* ASTBuildermake_null_value(ASTAllocator* allocator, uint64_t location);

    NumberValue* ASTBuildermake_number_value(ASTAllocator* allocator, int64_t value, uint64_t location);

    ShortValue* ASTBuildermake_short_value(ASTAllocator* allocator, short value, uint64_t location);

    SizeOfValue* ASTBuildermake_sizeof_value(ASTAllocator* allocator, BaseType* type, uint64_t location);

    StringValue* ASTBuildermake_string_value(ASTAllocator* allocator, chem::string_view* value, uint64_t location);

    StructValue* ASTBuildermake_struct_value(ASTAllocator* allocator, BaseType* ref, ASTNode* parent_node, uint64_t location);

    UBigIntValue* ASTBuildermake_ubigint_value(ASTAllocator* allocator, unsigned long long value, uint64_t location);

    UCharValue* ASTBuildermake_uchar_value(ASTAllocator* allocator, unsigned char value, uint64_t location);

    UInt128Value* ASTBuildermake_uint128_value(ASTAllocator* allocator, uint64_t low, uint64_t high, uint64_t location);

    UIntValue* ASTBuildermake_uint_value(ASTAllocator* allocator, unsigned int value, uint64_t location);

    ULongValue* ASTBuildermake_ulong_value(ASTAllocator* allocator, unsigned long value, bool is64Bit, uint64_t location);

    UShortValue* ASTBuildermake_ushort_value(ASTAllocator* allocator, unsigned short value, uint64_t location);

    BlockValue* ASTBuildermake_block_value(ASTAllocator* allocator, ASTNode* parent_node, uint64_t location);

    ValueNode* ASTBuildermake_value_node(ASTAllocator* allocator, Value* value, ASTNode* parent_node, uint64_t location);

    VariableIdentifier* ASTBuildermake_identifier(ASTAllocator* allocator, chem::string_view* value, bool is_ns, uint64_t location);

    VariantCall* ASTBuildermake_variant_call(ASTAllocator* allocator, AccessChain* chain, uint64_t location);

    VariantCase* ASTBuildermake_variant_case(ASTAllocator* allocator, Value* parent_value, SwitchStatement* stmt, uint64_t location);

    VariantCaseVariable* ASTBuildermake_variant_case_variable(ASTAllocator* allocator, chem::string_view* name, VariableIdentifier* parent_val, SwitchStatement* switch_stmt, uint64_t location);

    AssignStatement* ASTBuildermake_assignment_stmt(ASTAllocator* allocator, Value* lhs, Value* rhs, Operation op, ASTNode* parent_node, uint64_t location);

    BreakStatement* ASTBuildermake_break_stmt(ASTAllocator* allocator, LoopASTNode* loop_node, ASTNode* parent_node, uint64_t location);

    Comment* ASTBuildermake_comment_stmt(ASTAllocator* allocator, chem::string_view* value, bool multiline, ASTNode* parent_node, uint64_t location);

    ContinueStatement* ASTBuildermake_continue_stmt(ASTAllocator* allocator, LoopASTNode* loop_node, ASTNode* parent_node, uint64_t location);

    DestructStmt* ASTBuildermake_destruct_stmt(ASTAllocator* allocator, Value* array_value, Value* ptr_value, bool is_array, ASTNode* parent_node, uint64_t location);

    ReturnStatement* ASTBuildermake_return_stmt(ASTAllocator* allocator, Value* value, FunctionTypeBody* decl, ASTNode* parent_node, uint64_t location);

    //SwitchStatement* ASTBuildermake_return_stmt(ASTAllocator* allocator, Value* value, FunctionType* decl, ASTNode* parent_node, uint64_t location);

    //ThrowStatement* ASTBuildermake_throw_stmt(ASTAllocator* allocator, Value* value, FunctionType* decl, ASTNode* parent_node, uint64_t location);

    TypealiasStatement* ASTBuildermake_typealias_stmt(ASTAllocator* allocator, chem::string_view* identifier, uint64_t name_loc, BaseType* actual_type, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    UsingStmt* ASTBuildermake_using_stmt(ASTAllocator* allocator, AccessChain* chain, ASTNode* parent_node, bool is_namespace, uint64_t location);

    VarInitStatement* ASTBuildermake_varinit_stmt(ASTAllocator* allocator, bool is_const, bool is_reference, chem::string_view* identifier, uint64_t id_loc, BaseType* type, Value* value, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    Scope* ASTBuildermake_scope(ASTAllocator* allocator, ASTNode* parent_node, uint64_t location);

    DoWhileLoop* ASTBuildermake_do_while_loop(ASTAllocator* allocator, Value* condition, ASTNode* parent_node, uint64_t location);

    EnumDeclaration* ASTBuildermake_enum_decl(ASTAllocator* allocator, chem::string_view* name, uint64_t name_loc, IntNType* underlying_type, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    EnumMember* ASTBuildermake_enum_member(ASTAllocator* allocator, chem::string_view* name, unsigned int index, Value* init_value, EnumDeclaration* parent_node, uint64_t location);

    ForLoop* ASTBuildermake_for_loop(ASTAllocator* allocator, VarInitStatement* initializer, Value* conditionExpr, ASTNode* incrementerExpr, ASTNode* parent_node, uint64_t location);

    FunctionDeclaration* ASTBuildermake_function(ASTAllocator* allocator, chem::string_view* name, uint64_t name_location, BaseType* returnType, bool isVariadic, bool hasBody, ASTNode* parent_node, uint64_t location);

    FunctionParam* ASTBuildermake_function_param(ASTAllocator* allocator, chem::string_view* name, BaseType* type, unsigned int index, Value* value, bool implicit, FunctionType* decl, uint64_t location);

    GenericTypeParameter* ASTBuildermake_generic_param(ASTAllocator* allocator, chem::string_view* name, BaseType* at_least_type, BaseType* def_type, ASTNode* parent_node, unsigned int index, uint64_t location);

    IfStatement* ASTBuildermake_if_stmt(ASTAllocator* allocator, Value* condition, bool is_value, ASTNode* parent_node, uint64_t location);

    ImplDefinition* ASTBuildermake_impl_def(ASTAllocator* allocator, BaseType* interface_type, BaseType* struct_type, ASTNode* parent_node, uint64_t location);

    InitBlock* ASTBuildermake_init_block(ASTAllocator* allocator, ASTNode* parent_node, uint64_t location);

    InterfaceDefinition* ASTBuildermake_interface_def(ASTAllocator* allocator, chem::string_view* name, uint64_t name_loc, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    Namespace* ASTBuildermake_namespace(ASTAllocator* allocator, chem::string_view* name, uint64_t name_location, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    StructDefinition* ASTBuildermake_struct_def(ASTAllocator* allocator, chem::string_view* name, uint64_t name_loc, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    StructMember* ASTBuildermake_struct_member(ASTAllocator* allocator, chem::string_view* name, BaseType* type, Value* defValue, bool isConst, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    UnionDef* ASTBuildermake_union_def(ASTAllocator* allocator, chem::string_view* name, uint64_t name_loc, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    UnsafeBlock* ASTBuildermake_unsafe_block(ASTAllocator* allocator, ASTNode* node, uint64_t location);

    WhileLoop* ASTBuildermake_while_loop(ASTAllocator* allocator, Value* condition, ASTNode* node, uint64_t location);

    VariantDefinition* ASTBuildermake_variant_def(ASTAllocator* allocator, chem::string_view* name, uint64_t name_loc, AccessSpecifier specifier, ASTNode* node, uint64_t location);

    VariantMember* ASTBuildermake_variant_member(ASTAllocator* allocator, chem::string_view* name, VariantDefinition* parent_node, uint64_t location);

    VariantMemberParam* ASTBuildermake_variant_member_param(ASTAllocator* allocator, chem::string_view* name, unsigned int index, bool is_const, BaseType* type, Value* defValue, VariantMember* parent_node, uint64_t location);

}