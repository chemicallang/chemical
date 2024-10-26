// Copyright (c) Qinetik 2024.

#pragma once

#include "CBIUtils.h"
#include "ast/utils/Operation.h"
#include "ast/base/AccessSpecifier.h"

class CSTConverter;

class Value;

class BaseType;

class ASTNode;

class CSTToken;

class AnyType;
class ArrayType;
class BigIntType;
class BoolType;
class BoolType;
class DoubleType;
class DynamicType;
class FloatType;
class FunctionType;
class GenericType;
class Int128Type;
class IntType;
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

extern "C" {

    AnyType* ASTBuildermake_any_type(CSTConverter* converter, uint64_t location);

    ArrayType* ASTBuildermake_array_type(CSTConverter* converter, BaseType* elem_type, int array_size, uint64_t location);

    BigIntType* ASTBuildermake_bigint_type(CSTConverter* converter, uint64_t location);

    BoolType* ASTBuildermake_bool_type(CSTConverter* converter, uint64_t location);

    BoolType* ASTBuildermake_char_type(CSTConverter* converter, uint64_t location);

    DoubleType* ASTBuildermake_double_type(CSTConverter* converter, uint64_t location);

    DynamicType* ASTBuildermake_dynamic_type(CSTConverter* converter, BaseType* child_type, uint64_t location);

    FloatType* ASTBuildermake_float_type(CSTConverter* converter, uint64_t location);

    FunctionType* ASTBuildermake_func_type(CSTConverter* converter, BaseType* returnType, bool isVariadic, bool isCapturing, ASTNode* parent_node, uint64_t location);

    GenericType* ASTBuildermake_generic_type(CSTConverter* converter, LinkedType* linkedType);

    Int128Type* ASTBuildermake_int128_type(CSTConverter* converter, uint64_t location);

    IntType* ASTBuildermake_int_type(CSTConverter* converter, uint64_t location);

    LinkedType* ASTBuildermake_linked_type(CSTConverter* converter, chem::string* type, ASTNode* linked, uint64_t location);

    LinkedValueType* ASTBuildermake_linked_value_type(CSTConverter* converter, Value* value, uint64_t location);

    LiteralType* ASTBuildermake_literal_type(CSTConverter* converter, BaseType* child_type, uint64_t location);

    LongType* ASTBuildermake_long_type(CSTConverter* converter, uint64_t location);

    PointerType* ASTBuildermake_ptr_type(CSTConverter* converter, BaseType* child_type, uint64_t location);

    ReferenceType* ASTBuildermake_reference_type(CSTConverter* converter, BaseType* child_type, uint64_t location);

    ShortType* ASTBuildermake_short_type(CSTConverter* converter, uint64_t location);

    StringType* ASTBuildermake_string_type(CSTConverter* converter, uint64_t location);

    UBigIntType* ASTBuildermake_ubigint_type(CSTConverter* converter, uint64_t location);

    UCharType* ASTBuildermake_uchar_type(CSTConverter* converter, uint64_t location);

    UInt128Type* ASTBuildermake_uint128_type(CSTConverter* converter, uint64_t location);

    UIntType* ASTBuildermake_uint_type(CSTConverter* converter, uint64_t location);

    ULongType* ASTBuildermake_ulong_type(CSTConverter* converter, uint64_t location);

    UShortType* ASTBuildermake_ushort_type(CSTConverter* converter, uint64_t location);

    VoidType* ASTBuildermake_void_type(CSTConverter* converter, uint64_t location);

    AccessChain* ASTBuildermake_access_chain(CSTConverter* converter, ASTNode* parent_node, bool is_node, uint64_t location);

    AddrOfValue* ASTBuildermake_addr_of_value(CSTConverter* converter, Value* value, uint64_t location);

    ArrayValue* ASTBuildermake_array_value(CSTConverter* converter, BaseType* type, uint64_t location);

    BigIntValue* ASTBuildermake_bigint_value(CSTConverter* converter, long long value, uint64_t location);

    BoolValue* ASTBuildermake_bool_value(CSTConverter* converter, bool value, uint64_t location);

    CastedValue* ASTBuildermake_casted_value(CSTConverter* converter, Value* value, BaseType* type, uint64_t location);

    CharValue* ASTBuildermake_char_value(CSTConverter* converter, char value, uint64_t location);

    DereferenceValue* ASTBuildermake_dereference_value(CSTConverter* converter, Value* value, uint64_t location);

    DoubleValue* ASTBuildermake_double_value(CSTConverter* converter, double value, uint64_t location);

    Expression* ASTBuildermake_expression_value(CSTConverter* converter, Value* first, Value* second, Operation op, uint64_t location);

    FloatValue* ASTBuildermake_float_value(CSTConverter* converter, float value, uint64_t location);

    FunctionCall* ASTBuildermake_function_call_value(CSTConverter* converter, uint64_t location);

    IndexOperator* ASTBuildermake_index_op_value(CSTConverter* converter, uint64_t location);

    Int128Value* ASTBuildermake_int128_value(CSTConverter* converter, uint64_t mag, bool is_neg, uint64_t location);

    IntValue* ASTBuildermake_int_value(CSTConverter* converter, int value, uint64_t location);

    IsValue* ASTBuildermake_is_value(CSTConverter* converter, Value* value, BaseType* type, bool is_negating, uint64_t location);

    LambdaFunction* ASTBuildermake_lambda_function(CSTConverter* converter, Value* value, BaseType* type, bool isVariadic, ASTNode* parent_node, uint64_t location);

    CapturedVariable* ASTBuildermake_captured_variable(CSTConverter* converter, chem::string* name, unsigned int index, bool capture_by_ref, long value, uint64_t location);

    LongValue* ASTBuildermake_long_value(CSTConverter* converter, long value, uint64_t location);

    NegativeValue* ASTBuildermake_negative_value(CSTConverter* converter, Value* value, uint64_t location);

    NotValue* ASTBuildermake_not_value(CSTConverter* converter, Value* value, uint64_t location);

    NullValue* ASTBuildermake_null_value(CSTConverter* converter, uint64_t location);

    NumberValue* ASTBuildermake_number_value(CSTConverter* converter, int64_t value, uint64_t location);

    ShortValue* ASTBuildermake_short_value(CSTConverter* converter, short value, uint64_t location);

    SizeOfValue* ASTBuildermake_sizeof_value(CSTConverter* converter, BaseType* type, uint64_t location);

    StringValue* ASTBuildermake_string_value(CSTConverter* converter, chem::string* value, uint64_t location);

    StructMemberInitializer* ASTBuildermake_struct_member_initializer(CSTConverter* converter, chem::string* name, Value* value, StructValue* structValue);

    StructValue* ASTBuildermake_struct_value(CSTConverter* converter, BaseType* ref, ASTNode* parent_node, uint64_t location);

    UBigIntValue* ASTBuildermake_ubigint_value(CSTConverter* converter, unsigned long long value, uint64_t location);

    UCharValue* ASTBuildermake_uchar_value(CSTConverter* converter, unsigned char value, uint64_t location);

    UInt128Value* ASTBuildermake_uint128_value(CSTConverter* converter, uint64_t low, uint64_t high, uint64_t location);

    UIntValue* ASTBuildermake_uint_value(CSTConverter* converter, unsigned int value, uint64_t location);

    ULongValue* ASTBuildermake_ulong_value(CSTConverter* converter, unsigned long value, uint64_t location);

    UShortValue* ASTBuildermake_ushort_value(CSTConverter* converter, unsigned short value, uint64_t location);

    ValueNode* ASTBuildermake_value_node(CSTConverter* converter, Value* value, ASTNode* parent_node, uint64_t location);

    VariableIdentifier* ASTBuildermake_identifier(CSTConverter* converter, chem::string* value, bool is_ns, uint64_t location);

    VariantCall* ASTBuildermake_variant_call(CSTConverter* converter, AccessChain* chain, uint64_t location);

    VariantCase* ASTBuildermake_variant_case(CSTConverter* converter, AccessChain* chain, SwitchStatement* stmt, uint64_t location);

    VariantCaseVariable* ASTBuildermake_variant_case_variable(CSTConverter* converter, chem::string* name, VariantCase* variant_case, uint64_t location);

    AssignStatement* ASTBuildermake_assignment_stmt(CSTConverter* converter, Value* lhs, Value* rhs, Operation op, ASTNode* parent_node, uint64_t location);

    BreakStatement* ASTBuildermake_break_stmt(CSTConverter* converter, LoopASTNode* loop_node, ASTNode* parent_node, uint64_t location);

    Comment* ASTBuildermake_comment_stmt(CSTConverter* converter, chem::string* value, bool multiline, ASTNode* parent_node, uint64_t location);

    ContinueStatement* ASTBuildermake_continue_stmt(CSTConverter* converter, LoopASTNode* loop_node, ASTNode* parent_node, uint64_t location);

    DestructStmt* ASTBuildermake_destruct_stmt(CSTConverter* converter, Value* array_value, Value* ptr_value, bool is_array, ASTNode* parent_node, uint64_t location);

    ReturnStatement* ASTBuildermake_return_stmt(CSTConverter* converter, Value* value, FunctionType* decl, ASTNode* parent_node, uint64_t location);

    //SwitchStatement* ASTBuildermake_return_stmt(CSTConverter* converter, Value* value, FunctionType* decl, ASTNode* parent_node, uint64_t location);

    //ThrowStatement* ASTBuildermake_throw_stmt(CSTConverter* converter, Value* value, FunctionType* decl, ASTNode* parent_node, uint64_t location);

    TypealiasStatement* ASTBuildermake_typealias_stmt(CSTConverter* converter, chem::string* identifier, BaseType* actual_type, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    UsingStmt* ASTBuildermake_using_stmt(CSTConverter* converter, AccessChain* chain, bool is_namespace, uint64_t location);

    VarInitStatement* ASTBuildermake_varinit_stmt(CSTConverter* converter, bool is_const, chem::string* identifier, BaseType* type, Value* value, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    Scope* ASTBuildermake_scope(CSTConverter* converter, ASTNode* parent_node, uint64_t location);

    DoWhileLoop* ASTBuildermake_do_while_loop(CSTConverter* converter, Value* condition, ASTNode* parent_node, uint64_t location);

    EnumDeclaration* ASTBuildermake_enum_decl(CSTConverter* converter, chem::string* name, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    EnumMember* ASTBuildermake_enum_member(CSTConverter* converter, chem::string* name, unsigned int index, Value* init_value, EnumDeclaration* parent_node, uint64_t location);

    ForLoop* ASTBuildermake_for_loop(CSTConverter* converter, VarInitStatement* initializer, Value* conditionExpr, ASTNode* incrementerExpr, ASTNode* parent_node, uint64_t location);

    FunctionDeclaration* ASTBuildermake_function(CSTConverter* converter, chem::string* name, uint64_t name_location, BaseType* returnType, bool isVariadic, bool hasBody, ASTNode* parent_node, uint64_t location);

    FunctionParam* ASTBuildermake_function_param(CSTConverter* converter, chem::string* name, BaseType* type, unsigned int index, Value* value, bool implicit, FunctionType* decl, uint64_t location);

    GenericTypeParameter* ASTBuildermake_generic_param(CSTConverter* converter, chem::string* name, BaseType* at_least_type, BaseType* def_type, ASTNode* parent_node, unsigned int index, uint64_t location);

    IfStatement* ASTBuildermake_if_stmt(CSTConverter* converter, Value* condition, bool is_value, ASTNode* parent_node, uint64_t location);

    ImplDefinition* ASTBuildermake_impl_def(CSTConverter* converter, BaseType* interface_type, BaseType* struct_type, ASTNode* parent_node, uint64_t location);

    InitBlock* ASTBuildermake_init_block(CSTConverter* converter, ASTNode* parent_node, uint64_t location);

    InterfaceDefinition* ASTBuildermake_interface_def(CSTConverter* converter, chem::string* name, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    Namespace* ASTBuildermake_namespace(CSTConverter* converter, chem::string* name, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    StructDefinition* ASTBuildermake_struct_def(CSTConverter* converter, chem::string* name, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    StructMember* ASTBuildermake_struct_member(CSTConverter* converter, chem::string* name, BaseType* type, Value* defValue, bool isConst, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    UnionDef* ASTBuildermake_union_def(CSTConverter* converter, chem::string* name, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location);

    UnsafeBlock* ASTBuildermake_unsafe_block(CSTConverter* converter, ASTNode* node, uint64_t location);

    WhileLoop* ASTBuildermake_while_loop(CSTConverter* converter, Value* condition, ASTNode* node, uint64_t location);

    VariantDefinition* ASTBuildermake_variant_def(CSTConverter* converter, chem::string* name, AccessSpecifier specifier, ASTNode* node, uint64_t location);

    VariantMember* ASTBuildermake_variant_member(CSTConverter* converter, chem::string* name, VariantDefinition* parent_node, uint64_t location);

    VariantMemberParam* ASTBuildermake_variant_member_param(CSTConverter* converter, chem::string* name, unsigned int index, bool is_const, BaseType* type, Value* defValue, VariantMember* parent_node, uint64_t location);

}