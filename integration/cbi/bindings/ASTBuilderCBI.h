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

    AnyType* ASTBuildermake_any_type(CSTConverter* converter, CSTToken* token);

    ArrayType* ASTBuildermake_array_type(CSTConverter* converter, BaseType* elem_type, int array_size, CSTToken* token);

    BigIntType* ASTBuildermake_bigint_type(CSTConverter* converter, CSTToken* token);

    BoolType* ASTBuildermake_bool_type(CSTConverter* converter, CSTToken* token);

    BoolType* ASTBuildermake_char_type(CSTConverter* converter, CSTToken* token);

    DoubleType* ASTBuildermake_double_type(CSTConverter* converter, CSTToken* token);

    DynamicType* ASTBuildermake_dynamic_type(CSTConverter* converter, BaseType* child_type, CSTToken* token);

    FloatType* ASTBuildermake_float_type(CSTConverter* converter, CSTToken* token);

    FunctionType* ASTBuildermake_func_type(CSTConverter* converter, BaseType* returnType, bool isVariadic, bool isCapturing, CSTToken* token);

    GenericType* ASTBuildermake_generic_type(CSTConverter* converter, LinkedType* linkedType);

    Int128Type* ASTBuildermake_int128_type(CSTConverter* converter, CSTToken* token);

    IntType* ASTBuildermake_int_type(CSTConverter* converter, CSTToken* token);

    LinkedType* ASTBuildermake_linked_type(CSTConverter* converter, chem::string* type, ASTNode* linked, CSTToken* token);

    LinkedValueType* ASTBuildermake_linked_value_type(CSTConverter* converter, Value* value, CSTToken* token);

    LiteralType* ASTBuildermake_literal_type(CSTConverter* converter, BaseType* child_type, CSTToken* token);

    LongType* ASTBuildermake_long_type(CSTConverter* converter, CSTToken* token);

    PointerType* ASTBuildermake_ptr_type(CSTConverter* converter, BaseType* child_type, CSTToken* token);

    ReferenceType* ASTBuildermake_reference_type(CSTConverter* converter, BaseType* child_type, CSTToken* token);

    ShortType* ASTBuildermake_short_type(CSTConverter* converter, CSTToken* token);

    StringType* ASTBuildermake_string_type(CSTConverter* converter, CSTToken* token);

    UBigIntType* ASTBuildermake_ubigint_type(CSTConverter* converter, CSTToken* token);

    UCharType* ASTBuildermake_uchar_type(CSTConverter* converter, CSTToken* token);

    UInt128Type* ASTBuildermake_uint128_type(CSTConverter* converter, CSTToken* token);

    UIntType* ASTBuildermake_uint_type(CSTConverter* converter, CSTToken* token);

    ULongType* ASTBuildermake_ulong_type(CSTConverter* converter, CSTToken* token);

    UShortType* ASTBuildermake_ushort_type(CSTConverter* converter, CSTToken* token);

    VoidType* ASTBuildermake_void_type(CSTConverter* converter, CSTToken* token);

    AccessChain* ASTBuildermake_access_chain(CSTConverter* converter, ASTNode* parent_node, bool is_node, CSTToken* token);

    AddrOfValue* ASTBuildermake_addr_of_value(CSTConverter* converter, Value* value, CSTToken* token);

    ArrayValue* ASTBuildermake_array_value(CSTConverter* converter, BaseType* type, CSTToken* token);

    BigIntValue* ASTBuildermake_bigint_value(CSTConverter* converter, long long value, CSTToken* token);

    BoolValue* ASTBuildermake_bool_value(CSTConverter* converter, bool value, CSTToken* token);

    CastedValue* ASTBuildermake_casted_value(CSTConverter* converter, Value* value, BaseType* type, CSTToken* token);

    CharValue* ASTBuildermake_char_value(CSTConverter* converter, char value, CSTToken* token);

    DereferenceValue* ASTBuildermake_dereference_value(CSTConverter* converter, Value* value, CSTToken* token);

    DoubleValue* ASTBuildermake_double_value(CSTConverter* converter, double value, CSTToken* token);

    Expression* ASTBuildermake_expression_value(CSTConverter* converter, Value* first, Value* second, Operation op, CSTToken* token);

    FloatValue* ASTBuildermake_float_value(CSTConverter* converter, float value, CSTToken* token);

    FunctionCall* ASTBuildermake_function_call_value(CSTConverter* converter, CSTToken* token);

    IndexOperator* ASTBuildermake_index_op_value(CSTConverter* converter, CSTToken* token);

    Int128Value* ASTBuildermake_int128_value(CSTConverter* converter, uint64_t mag, bool is_neg, CSTToken* token);

    IntValue* ASTBuildermake_int_value(CSTConverter* converter, int value, CSTToken* token);

    IsValue* ASTBuildermake_is_value(CSTConverter* converter, Value* value, BaseType* type, bool is_negating, CSTToken* token);

    LambdaFunction* ASTBuildermake_lambda_function(CSTConverter* converter, Value* value, BaseType* type, bool isVariadic, ASTNode* parent_node, CSTToken* token);

    CapturedVariable* ASTBuildermake_captured_variable(CSTConverter* converter, chem::string* name, unsigned int index, bool capture_by_ref, long value, CSTToken* token);

    LongValue* ASTBuildermake_long_value(CSTConverter* converter, long value, CSTToken* token);

    NegativeValue* ASTBuildermake_negative_value(CSTConverter* converter, Value* value, CSTToken* token);

    NotValue* ASTBuildermake_not_value(CSTConverter* converter, Value* value, CSTToken* token);

    NullValue* ASTBuildermake_null_value(CSTConverter* converter, CSTToken* token);

    NumberValue* ASTBuildermake_number_value(CSTConverter* converter, int64_t value, CSTToken* token);

    ShortValue* ASTBuildermake_short_value(CSTConverter* converter, short value, CSTToken* token);

    SizeOfValue* ASTBuildermake_sizeof_value(CSTConverter* converter, BaseType* type, CSTToken* token);

    StringValue* ASTBuildermake_string_value(CSTConverter* converter, chem::string* value, CSTToken* token);

    StructMemberInitializer* ASTBuildermake_struct_member_initializer(CSTConverter* converter, chem::string* name, Value* value, StructValue* structValue);

    StructValue* ASTBuildermake_struct_struct_value(CSTConverter* converter, Value* ref, ASTNode* parent_node, CSTToken* token);

    UBigIntValue* ASTBuildermake_ubigint_value(CSTConverter* converter, unsigned long long value, CSTToken* token);

    UCharValue* ASTBuildermake_uchar_value(CSTConverter* converter, unsigned char value, CSTToken* token);

    UInt128Value* ASTBuildermake_uint128_value(CSTConverter* converter, uint64_t low, uint64_t high, CSTToken* token);

    UIntValue* ASTBuildermake_uint_value(CSTConverter* converter, unsigned int value, CSTToken* token);

    ULongValue* ASTBuildermake_ulong_value(CSTConverter* converter, unsigned long value, CSTToken* token);

    UShortValue* ASTBuildermake_ushort_value(CSTConverter* converter, unsigned short value, CSTToken* token);

    ValueNode* ASTBuildermake_value_node(CSTConverter* converter, Value* value, ASTNode* parent_node, CSTToken* token);

    VariableIdentifier* ASTBuildermake_identifier(CSTConverter* converter, chem::string* value, bool is_ns, CSTToken* token);

    VariantCall* ASTBuildermake_variant_call(CSTConverter* converter, AccessChain* chain, CSTToken* token);

    VariantCase* ASTBuildermake_variant_case(CSTConverter* converter, AccessChain* chain, SwitchStatement* stmt, CSTToken* token);

    VariantCaseVariable* ASTBuildermake_variant_case_variable(CSTConverter* converter, chem::string* name, VariantCase* variant_case, CSTToken* token);

    AssignStatement* ASTBuildermake_assignment_stmt(CSTConverter* converter, Value* lhs, Value* rhs, Operation op, ASTNode* parent_node, CSTToken* token);

    BreakStatement* ASTBuildermake_break_stmt(CSTConverter* converter, LoopASTNode* loop_node, ASTNode* parent_node, CSTToken* token);

    Comment* ASTBuildermake_comment_stmt(CSTConverter* converter, chem::string* value, bool multiline, ASTNode* parent_node, CSTToken* token);

    ContinueStatement* ASTBuildermake_continue_stmt(CSTConverter* converter, LoopASTNode* loop_node, ASTNode* parent_node, CSTToken* token);

    DestructStmt* ASTBuildermake_destruct_stmt(CSTConverter* converter, Value* array_value, Value* ptr_value, bool is_array, ASTNode* parent_node, CSTToken* token);

    ReturnStatement* ASTBuildermake_return_stmt(CSTConverter* converter, Value* value, FunctionType* decl, ASTNode* parent_node, CSTToken* token);

    //SwitchStatement* ASTBuildermake_return_stmt(CSTConverter* converter, Value* value, FunctionType* decl, ASTNode* parent_node, CSTToken* token);

    //ThrowStatement* ASTBuildermake_throw_stmt(CSTConverter* converter, Value* value, FunctionType* decl, ASTNode* parent_node, CSTToken* token);

    TypealiasStatement* ASTBuildermake_typealias_stmt(CSTConverter* converter, chem::string* identifier, BaseType* actual_type, AccessSpecifier specifier, ASTNode* parent_node, CSTToken* token);

    UsingStmt* ASTBuildermake_using_stmt(CSTConverter* converter, AccessChain* chain, bool is_namespace, CSTToken* token);

    VarInitStatement* ASTBuildermake_varinit_stmt(CSTConverter* converter, bool is_const, chem::string* identifier, BaseType* type, Value* value, AccessSpecifier specifier, ASTNode* parent_node, CSTToken* token);

    Scope* ASTBuildermake_scope(CSTConverter* converter, ASTNode* parent_node, CSTToken* token);

    DoWhileLoop* ASTBuildermake_do_while_loop(CSTConverter* converter, Value* condition, ASTNode* parent_node, CSTToken* token);

    EnumDeclaration* ASTBuildermake_enum_decl(CSTConverter* converter, chem::string* name, AccessSpecifier specifier, ASTNode* parent_node, CSTToken* token);

    EnumMember* ASTBuildermake_enum_member(CSTConverter* converter, chem::string* name, unsigned int index, EnumDeclaration* parent_node, CSTToken* token);

    ForLoop* ASTBuildermake_for_loop(CSTConverter* converter, VarInitStatement* initializer, Value* conditionExpr, ASTNode* incrementerExpr, ASTNode* parent_node, CSTToken* token);

    FunctionDeclaration* ASTBuildermake_function(CSTConverter* converter, chem::string* name, BaseType* returnType, bool isVariadic, bool hasBody, ASTNode* parent_node, CSTToken* token);

    FunctionParam* ASTBuildermake_function_param(CSTConverter* converter, chem::string* name, BaseType* type, unsigned int index, Value* value, FunctionType* decl, CSTToken* token);

    GenericTypeParameter* ASTBuildermake_generic_param(CSTConverter* converter, chem::string* name, BaseType* def_type, ASTNode* parent_node, unsigned int index, CSTToken* token);

    IfStatement* ASTBuildermake_if_stmt(CSTConverter* converter, Value* condition, bool is_value, ASTNode* parent_node, CSTToken* token);

    ImplDefinition* ASTBuildermake_impl_def(CSTConverter* converter, BaseType* interface_type, BaseType* struct_type, ASTNode* parent_node, CSTToken* token);

    InitBlock* ASTBuildermake_init_block(CSTConverter* converter, ASTNode* parent_node, CSTToken* token);

    InterfaceDefinition* ASTBuildermake_interface_def(CSTConverter* converter, chem::string* name, AccessSpecifier specifier, ASTNode* parent_node, CSTToken* token);

    Namespace* ASTBuildermake_namespace(CSTConverter* converter, chem::string* name, AccessSpecifier specifier, ASTNode* parent_node, CSTToken* token);

    StructDefinition* ASTBuildermake_struct_def(CSTConverter* converter, chem::string* name, AccessSpecifier specifier, ASTNode* parent_node, CSTToken* token);

    StructMember* ASTBuildermake_struct_member(CSTConverter* converter, chem::string* name, BaseType* type, Value* defValue, bool isConst, AccessSpecifier specifier, ASTNode* parent_node, CSTToken* token);

    UnionDef* ASTBuildermake_union_def(CSTConverter* converter, chem::string* name, AccessSpecifier specifier, ASTNode* parent_node, CSTToken* token);

    UnsafeBlock* ASTBuildermake_unsafe_block(CSTConverter* converter, ASTNode* node, CSTToken* token);

    WhileLoop* ASTBuildermake_while_loop(CSTConverter* converter, Value* condition, ASTNode* node, CSTToken* token);

    VariantDefinition* ASTBuildermake_variant_def(CSTConverter* converter, chem::string* name, AccessSpecifier specifier, ASTNode* node, CSTToken* token);

    VariantMember* ASTBuildermake_variant_member(CSTConverter* converter, chem::string* name, VariantDefinition* parent_node, CSTToken* token);

    VariantMemberParam* ASTBuildermake_variant_member_param(CSTConverter* converter, chem::string* name, unsigned int index, BaseType* type, Value* defValue, VariantMember* parent_node, CSTToken* token);

}