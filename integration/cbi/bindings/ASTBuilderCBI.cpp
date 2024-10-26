// Copyright (c) Qinetik 2024.

#include "ASTBuilderCBI.h"
#include "ASTCBI.h"
#include "cst/base/CSTConverter.h"
#include "ast/types/AnyType.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/statements/VarInit.h"
#include "ast/types/IntNType.h"
#include "ast/types/VoidType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/LinkedType.h"
#include "ast/types/UCharType.h"
#include "ast/types/Int128Type.h"
#include "ast/types/UInt128Type.h"
#include "ast/statements/Assignment.h"
#include "ast/types/ArrayType.h"
#include "ast/values/StringValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/Expression.h"
#include "ast/values/VariantCase.h"
#include "ast/values/BoolValue.h"
#include "ast/values/UCharValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/VariantCall.h"
#include "ast/values/AccessChain.h"
#include "ast/values/ValueNode.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/IntValue.h"
#include "ast/values/UInt128Value.h"
#include "ast/values/LongValue.h"
#include "ast/values/ULongValue.h"
#include "ast/values/Negative.h"
#include "ast/values/NullValue.h"
#include "ast/values/IsValue.h"
#include "ast/values/VariantCaseVariable.h"
#include "ast/values/Int128Value.h"
#include "ast/values/NotValue.h"
#include "ast/values/AddrOfValue.h"
#include "ast/values/DereferenceValue.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/statements/Import.h"
#include "ast/structures/TryCatch.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/statements/Return.h"
#include "ast/types/GenericType.h"
#include "ast/types/LiteralType.h"
#include "ast/structures/ForLoop.h"
#include "ast/structures/CapturedVariable.h"
#include "ast/structures/MultiFunctionNode.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/WhileLoop.h"
#include "ast/values/StructValue.h"
#include "ast/values/UIntValue.h"
#include "ast/values/ShortValue.h"
#include "ast/values/UShortValue.h"
#include "ast/values/BigIntValue.h"
#include "ast/values/UBigIntValue.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/structures/DoWhileLoop.h"
#include "ast/statements/Continue.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/statements/Comment.h"
#include "ast/statements/DestructStmt.h"
#include "ast/structures/Namespace.h"
#include "ast/statements/Break.h"
#include "ast/statements/Typealias.h"
#include "ast/values/ArrayValue.h"
#include "ast/structures/If.h"
#include "ast/values/LambdaFunction.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/LoopBlock.h"
#include "ast/values/CastedValue.h"
#include "ast/values/NumberValue.h"
#include "ast/structures/ExtensionFunction.h"
#include "ast/structures/InitBlock.h"
#include "ast/statements/ThrowStatement.h"
#include "ast/values/SizeOfValue.h"
#include "ast/types/LinkedValueType.h"
#include "ast/statements/UsingStmt.h"
#include "ast/types/LinkedType.h"
#include "ast/types/DynamicType.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/UnsafeBlock.h"
#include "std/chem_string.h"

AnyType* ASTBuildermake_any_type(CSTConverter* converter, uint64_t location) {
    return new (converter->local<AnyType>()) AnyType(location);
}

ArrayType* ASTBuildermake_array_type(CSTConverter* converter, BaseType* elem_type, int array_size, uint64_t location) {
    return new (converter->local<ArrayType>()) ArrayType(elem_type, array_size, location);
}

BigIntType* ASTBuildermake_bigint_type(CSTConverter* converter, uint64_t location) {
    return new (converter->local<BigIntType>()) BigIntType(location);
}

BoolType* ASTBuildermake_bool_type(CSTConverter* converter, uint64_t location) {
    return new (converter->local<BoolType>()) BoolType(location);
}

BoolType* ASTBuildermake_char_type(CSTConverter* converter, uint64_t location) {
    return new (converter->local<BoolType>()) BoolType(location);
}

DoubleType* ASTBuildermake_double_type(CSTConverter* converter, uint64_t location) {
    return new (converter->local<DoubleType>()) DoubleType(location);
}

DynamicType* ASTBuildermake_dynamic_type(CSTConverter* converter, BaseType* child_type, uint64_t location) {
    return new (converter->local<DynamicType>()) DynamicType(child_type, location);
}

FloatType* ASTBuildermake_float_type(CSTConverter* converter, uint64_t location) {
    return new (converter->local<FloatType>()) FloatType(location);
}

FunctionType* ASTBuildermake_func_type(CSTConverter* converter, BaseType* returnType, bool isVariadic, bool isCapturing, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<FunctionType>()) FunctionType({}, returnType, isVariadic, isCapturing, parent_node, location);
}

GenericType* ASTBuildermake_generic_type(CSTConverter* converter, LinkedType* linkedType) {
    return new (converter->local<GenericType>()) GenericType(linkedType);
}

Int128Type* ASTBuildermake_int128_type(CSTConverter* converter, uint64_t location) {
    return new (converter->local<Int128Type>()) Int128Type(location);
}

IntType* ASTBuildermake_int_type(CSTConverter* converter, uint64_t location) {
    return new (converter->local<IntType>()) IntType(location);
}

LinkedType* ASTBuildermake_linked_type(CSTConverter* converter, chem::string* type, ASTNode* linked, uint64_t location) {
    return new (converter->local<LinkedType>()) LinkedType(type->to_std_string(), linked, location);
}

LinkedValueType* ASTBuildermake_linked_value_type(CSTConverter* converter, Value* value, uint64_t location) {
    return new (converter->local<LinkedValueType>()) LinkedValueType(value, location);
}

LiteralType* ASTBuildermake_literal_type(CSTConverter* converter, BaseType* child_type, uint64_t location) {
    return new (converter->local<LiteralType>()) LiteralType(child_type, location);
}

LongType* ASTBuildermake_long_type(CSTConverter* converter, uint64_t location) {
    return new (converter->local<LongType>()) LongType(converter->is64Bit, location);
}

PointerType* ASTBuildermake_ptr_type(CSTConverter* converter, BaseType* child_type, uint64_t location) {
    return new (converter->local<PointerType>()) PointerType(child_type, location);
}

ReferenceType* ASTBuildermake_reference_type(CSTConverter* converter, BaseType* child_type, uint64_t location) {
    return new (converter->local<ReferenceType>()) ReferenceType(child_type, location);
}

ShortType* ASTBuildermake_short_type(CSTConverter* converter, uint64_t location) {
    return new (converter->local<ShortType>()) ShortType(location);
}

StringType* ASTBuildermake_string_type(CSTConverter* converter, uint64_t location) {
    return new (converter->local<StringType>()) StringType(location);
}

UBigIntType* ASTBuildermake_ubigint_type(CSTConverter* converter, uint64_t location) {
    return new (converter->local<UBigIntType>()) UBigIntType(location);
}

UCharType* ASTBuildermake_uchar_type(CSTConverter* converter, uint64_t location) {
    return new (converter->local<UCharType>()) UCharType(location);
}

UInt128Type* ASTBuildermake_uint128_type(CSTConverter* converter, uint64_t location) {
    return new (converter->local<UInt128Type>()) UInt128Type(location);
}

UIntType* ASTBuildermake_uint_type(CSTConverter* converter, uint64_t location) {
    return new (converter->local<UIntType>()) UIntType(location);
}

ULongType* ASTBuildermake_ulong_type(CSTConverter* converter, uint64_t location) {
    return new (converter->local<ULongType>()) ULongType(converter->is64Bit, location);
}

UShortType* ASTBuildermake_ushort_type(CSTConverter* converter, uint64_t location) {
    return new (converter->local<UShortType>()) UShortType(location);
}

VoidType* ASTBuildermake_void_type(CSTConverter* converter, uint64_t location) {
    return new (converter->local<VoidType>()) VoidType(location);
}

AccessChain* ASTBuildermake_access_chain(CSTConverter* converter, ASTNode* parent_node, bool is_node, uint64_t location) {
    return new (converter->local<AccessChain>()) AccessChain(parent_node, is_node, location);
}

AddrOfValue* ASTBuildermake_addr_of_value(CSTConverter* converter, Value* value, uint64_t location) {
    return new (converter->local<AddrOfValue>()) AddrOfValue(value, location);
}

ArrayValue* ASTBuildermake_array_value(CSTConverter* converter, BaseType* type, uint64_t location) {
    return new (converter->local<ArrayValue>()) ArrayValue({}, type, {}, location, *converter->local_allocator);
}

BigIntValue* ASTBuildermake_bigint_value(CSTConverter* converter, long long value, uint64_t location) {
    return new (converter->local<BigIntValue>()) BigIntValue(value, location);
}

BoolValue* ASTBuildermake_bool_value(CSTConverter* converter, bool value, uint64_t location) {
    return new (converter->local<BoolValue>()) BoolValue(value, location);
}

CastedValue* ASTBuildermake_casted_value(CSTConverter* converter, Value* value, BaseType* type, uint64_t location) {
    return new (converter->local<CastedValue>()) CastedValue(value, type, location);
}

CharValue* ASTBuildermake_char_value(CSTConverter* converter, char value, uint64_t location) {
    return new (converter->local<CharValue>()) CharValue(value, location);
}

DereferenceValue* ASTBuildermake_dereference_value(CSTConverter* converter, Value* value, uint64_t location) {
    return new (converter->local<DereferenceValue>()) DereferenceValue(value, location);
}

DoubleValue* ASTBuildermake_double_value(CSTConverter* converter, double value, uint64_t location) {
    return new (converter->local<DoubleValue>()) DoubleValue(value, location);
}

Expression* ASTBuildermake_expression_value(CSTConverter* converter, Value* first, Value* second, Operation op, uint64_t location) {
    return new (converter->local<Expression>()) Expression(first, second, op, converter->is64Bit, location);
}

FloatValue* ASTBuildermake_float_value(CSTConverter* converter, float value, uint64_t location) {
    return new (converter->local<FloatValue>()) FloatValue(value, location);
}

FunctionCall* ASTBuildermake_function_call_value(CSTConverter* converter, uint64_t location) {
    return new (converter->local<FunctionCall>()) FunctionCall({}, location);
}

IndexOperator* ASTBuildermake_index_op_value(CSTConverter* converter, uint64_t location) {
    return new (converter->local<IndexOperator>()) IndexOperator({}, location);
}

Int128Value* ASTBuildermake_int128_value(CSTConverter* converter, uint64_t mag, bool is_neg, uint64_t location) {
    return new (converter->local<Int128Value>()) Int128Value(mag, is_neg, location);
}

IntValue* ASTBuildermake_int_value(CSTConverter* converter, int value, uint64_t location) {
    return new (converter->local<IntValue>()) IntValue(value, location);
}

IsValue* ASTBuildermake_is_value(CSTConverter* converter, Value* value, BaseType* type, bool is_negating, uint64_t location) {
    return new (converter->local<IsValue>()) IsValue(value, type, is_negating, location);
}

LambdaFunction* ASTBuildermake_lambda_function(CSTConverter* converter, Value* value, BaseType* type, bool isVariadic, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<LambdaFunction>()) LambdaFunction({}, {}, isVariadic, Scope(parent_node, location), parent_node, location);
}

CapturedVariable* ASTBuildermake_captured_variable(CSTConverter* converter, chem::string* name, unsigned int index, bool capture_by_ref, long value, uint64_t location) {
    return new (converter->local<CapturedVariable>()) CapturedVariable(name->to_std_string(), index, capture_by_ref, location);
}

LongValue* ASTBuildermake_long_value(CSTConverter* converter, long value, uint64_t location) {
    return new (converter->local<LongValue>()) LongValue(value, converter->is64Bit, location);
}

NegativeValue* ASTBuildermake_negative_value(CSTConverter* converter, Value* value, uint64_t location) {
    return new (converter->local<NegativeValue>()) NegativeValue(value, location);
}

NotValue* ASTBuildermake_not_value(CSTConverter* converter, Value* value, uint64_t location) {
    return new (converter->local<NotValue>()) NotValue(value, location);
}

NullValue* ASTBuildermake_null_value(CSTConverter* converter, uint64_t location) {
    return new (converter->local<NullValue>()) NullValue(location);
}

NumberValue* ASTBuildermake_number_value(CSTConverter* converter, int64_t value, uint64_t location) {
    return new (converter->local<NumberValue>()) NumberValue(value, location);
}

ShortValue* ASTBuildermake_short_value(CSTConverter* converter, short value, uint64_t location) {
    return new (converter->local<ShortValue>()) ShortValue(value, location);
}

SizeOfValue* ASTBuildermake_sizeof_value(CSTConverter* converter, BaseType* type, uint64_t location) {
    return new (converter->local<SizeOfValue>()) SizeOfValue(type, location);
}

StringValue* ASTBuildermake_string_value(CSTConverter* converter, chem::string* value, uint64_t location) {
    return new (converter->local<StringValue>()) StringValue(value->to_std_string(), location);
}

StructMemberInitializer* ASTBuildermake_struct_member_initializer(CSTConverter* converter, chem::string* name, Value* value, StructValue* structValue) {
    return new (converter->local<StructMemberInitializer>()) StructMemberInitializer(name->to_std_string(), value, structValue, nullptr);
}

StructValue* ASTBuildermake_struct_value(CSTConverter* converter, BaseType* ref, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<StructValue>()) StructValue(ref, {}, nullptr, location, parent_node);
}

UBigIntValue* ASTBuildermake_ubigint_value(CSTConverter* converter, unsigned long long value, uint64_t location) {
    return new (converter->local<UBigIntValue>()) UBigIntValue(value, location);
}

UCharValue* ASTBuildermake_uchar_value(CSTConverter* converter, unsigned char value, uint64_t location) {
    return new (converter->local<UCharValue>()) UCharValue(value, location);
}

UInt128Value* ASTBuildermake_uint128_value(CSTConverter* converter, uint64_t low, uint64_t high, uint64_t location) {
    return new (converter->local<UInt128Value>()) UInt128Value(low, high, location);
}

UIntValue* ASTBuildermake_uint_value(CSTConverter* converter, unsigned int value, uint64_t location) {
    return new (converter->local<UIntValue>()) UIntValue(value, location);
}

ULongValue* ASTBuildermake_ulong_value(CSTConverter* converter, unsigned long value, uint64_t location) {
    return new (converter->local<ULongValue>()) ULongValue(value, converter->is64Bit, location);
}

UShortValue* ASTBuildermake_ushort_value(CSTConverter* converter, unsigned short value, uint64_t location) {
    return new (converter->local<UShortValue>()) UShortValue(value, location);
}

ValueNode* ASTBuildermake_value_node(CSTConverter* converter, Value* value, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<ValueNode>()) ValueNode(value, parent_node, location);
}

VariableIdentifier* ASTBuildermake_identifier(CSTConverter* converter, chem::string* value, bool is_ns, uint64_t location) {
    return new (converter->local<VariableIdentifier>()) VariableIdentifier(value->to_std_string(), location, is_ns);
}

VariantCall* ASTBuildermake_variant_call(CSTConverter* converter, AccessChain* chain, uint64_t location) {
    return new (converter->local<VariantCall>()) VariantCall(chain, location);
}

VariantCase* ASTBuildermake_variant_case(CSTConverter* converter, AccessChain* chain, SwitchStatement* stmt, uint64_t location) {
    return new (converter->local<VariantCase>()) VariantCase(chain, stmt, location);
}

VariantCaseVariable* ASTBuildermake_variant_case_variable(CSTConverter* converter, chem::string* name, VariantCase* variant_case, uint64_t location) {
    return new (converter->local<VariantCaseVariable>()) VariantCaseVariable(name->to_std_string(), variant_case, location);
}

AssignStatement* ASTBuildermake_assignment_stmt(CSTConverter* converter, Value* lhs, Value* rhs, Operation op, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<AssignStatement>()) AssignStatement(lhs, rhs, op, parent_node, location);
}

BreakStatement* ASTBuildermake_break_stmt(CSTConverter* converter, LoopASTNode* loop_node, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<BreakStatement>()) BreakStatement(loop_node, parent_node, location);
}

Comment* ASTBuildermake_comment_stmt(CSTConverter* converter, chem::string* value, bool multiline, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<Comment>()) Comment(value->to_std_string(), multiline, parent_node, location);
}

ContinueStatement* ASTBuildermake_continue_stmt(CSTConverter* converter, LoopASTNode* loop_node, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<ContinueStatement>()) ContinueStatement(loop_node, parent_node, location);
}

DestructStmt* ASTBuildermake_destruct_stmt(CSTConverter* converter, Value* array_value, Value* ptr_value, bool is_array, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<DestructStmt>()) DestructStmt(array_value, ptr_value, is_array, parent_node, location);
}

ReturnStatement* ASTBuildermake_return_stmt(CSTConverter* converter, Value* value, FunctionType* decl, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<ReturnStatement>()) ReturnStatement(value, decl, parent_node, location);
}

// TODO switch statement when multiple cases have been handled
//SwitchStatement* ASTBuildermake_return_stmt(CSTConverter* converter, Value* value, FunctionType* decl, ASTNode* parent_node, uint64_t location) {
//    return new (converter->local<SwitchStatement>()) SwitchStatement(value, decl, parent_node, location);
//}

//ThrowStatement* ASTBuildermake_throw_stmt(CSTConverter* converter, Value* value, FunctionType* decl, ASTNode* parent_node, uint64_t location) {
//    return new (converter->local<ThrowStatement>()) ThrowStatement(value, decl, parent_node, location);
//}

TypealiasStatement* ASTBuildermake_typealias_stmt(CSTConverter* converter, chem::string* identifier, BaseType* actual_type, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<TypealiasStatement>()) TypealiasStatement(identifier->to_std_string(), actual_type, parent_node, location, specifier);
}

UsingStmt* ASTBuildermake_using_stmt(CSTConverter* converter, AccessChain* chain, bool is_namespace, uint64_t location) {
    return new (converter->local<UsingStmt>()) UsingStmt(chain, is_namespace, location);
}

VarInitStatement* ASTBuildermake_varinit_stmt(CSTConverter* converter, bool is_const, chem::string* identifier, BaseType* type, Value* value, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<VarInitStatement>()) VarInitStatement(is_const, identifier->to_std_string(), type, value, parent_node, location, specifier);
}

// TODO scope needs a children method to get the nodes PtrVec
Scope* ASTBuildermake_scope(CSTConverter* converter, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<Scope>()) Scope(parent_node, location);
}

DoWhileLoop* ASTBuildermake_do_while_loop(CSTConverter* converter, Value* condition, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<DoWhileLoop>()) DoWhileLoop(condition, LoopScope(parent_node, location), parent_node, location);
}

EnumDeclaration* ASTBuildermake_enum_decl(CSTConverter* converter, chem::string* name, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<EnumDeclaration>()) EnumDeclaration(name->to_std_string(), {}, parent_node, location, specifier);
}

EnumMember* ASTBuildermake_enum_member(CSTConverter* converter, chem::string* name, unsigned int index, Value* init_value, EnumDeclaration* parent_node, uint64_t location) {
    return new (converter->local<EnumMember>()) EnumMember(name->data(), index, init_value, parent_node, location);
}

ForLoop* ASTBuildermake_for_loop(CSTConverter* converter, VarInitStatement* initializer, Value* conditionExpr, ASTNode* incrementerExpr, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<ForLoop>()) ForLoop(initializer, conditionExpr, incrementerExpr, LoopScope(parent_node, location), parent_node, location);
}

FunctionDeclaration* ASTBuildermake_function(CSTConverter* converter, chem::string* name, uint64_t name_location, BaseType* returnType, bool isVariadic, bool hasBody, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<FunctionDeclaration>()) FunctionDeclaration({ name->to_std_string(), name_location }, {}, returnType, isVariadic, parent_node, location, std::nullopt);
}

FunctionParam* ASTBuildermake_function_param(CSTConverter* converter, chem::string* name, BaseType* type, unsigned int index, Value* value, bool implicit, FunctionType* decl, uint64_t location) {
    return new (converter->local<FunctionParam>()) FunctionParam(name->to_std_string(), type, index, value, implicit, decl, location);
}

GenericTypeParameter* ASTBuildermake_generic_param(CSTConverter* converter, chem::string* name, BaseType* at_least_type, BaseType* def_type, ASTNode* parent_node, unsigned int index, uint64_t location) {
    return new (converter->local<GenericTypeParameter>()) GenericTypeParameter(name->to_std_string(), at_least_type, def_type, parent_node, index, location);
}

IfStatement* ASTBuildermake_if_stmt(CSTConverter* converter, Value* condition, bool is_value, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<IfStatement>()) IfStatement(condition, Scope(parent_node, location), {}, std::nullopt, parent_node, is_value, location);
}

ImplDefinition* ASTBuildermake_impl_def(CSTConverter* converter, BaseType* interface_type, BaseType* struct_type, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<ImplDefinition>()) ImplDefinition(interface_type, struct_type, parent_node, location);
}

InitBlock* ASTBuildermake_init_block(CSTConverter* converter, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<InitBlock>()) InitBlock(Scope(parent_node, location), parent_node, location);
}

InterfaceDefinition* ASTBuildermake_interface_def(CSTConverter* converter, chem::string* name, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<InterfaceDefinition>()) InterfaceDefinition(name->to_std_string(), parent_node, location, specifier);
}

Namespace* ASTBuildermake_namespace(CSTConverter* converter, chem::string* name, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<Namespace>()) Namespace(name->to_std_string(), parent_node, location, specifier);
}

StructDefinition* ASTBuildermake_struct_def(CSTConverter* converter, chem::string* name, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<StructDefinition>()) StructDefinition(name->to_std_string(), parent_node, location, specifier);
}

StructMember* ASTBuildermake_struct_member(CSTConverter* converter, chem::string* name, BaseType* type, Value* defValue, bool isConst, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<StructMember>()) StructMember(name->to_std_string(), type, defValue, parent_node, location, isConst, specifier);
}

UnionDef* ASTBuildermake_union_def(CSTConverter* converter, chem::string* name, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (converter->local<UnionDef>()) UnionDef(name->to_std_string(), parent_node, location, specifier);
}

UnsafeBlock* ASTBuildermake_unsafe_block(CSTConverter* converter, ASTNode* node, uint64_t location) {
    return new (converter->local<UnsafeBlock>()) UnsafeBlock(Scope(node, location), location);
}

WhileLoop* ASTBuildermake_while_loop(CSTConverter* converter, Value* condition, ASTNode* node, uint64_t location) {
    return new (converter->local<WhileLoop>()) WhileLoop(condition, LoopScope(node, location), node, location);
}

VariantDefinition* ASTBuildermake_variant_def(CSTConverter* converter, chem::string* name, AccessSpecifier specifier, ASTNode* node, uint64_t location) {
    return new (converter->local<VariantDefinition>()) VariantDefinition(name->to_std_string(), node, location, specifier);
}

VariantMember* ASTBuildermake_variant_member(CSTConverter* converter, chem::string* name, VariantDefinition* parent_node, uint64_t location) {
    return new (converter->local<VariantMember>()) VariantMember(name->to_std_string(), parent_node, location);
}

VariantMemberParam* ASTBuildermake_variant_member_param(CSTConverter* converter, chem::string* name, unsigned int index, bool is_const, BaseType* type, Value* defValue, VariantMember* parent_node, uint64_t location) {
    return new (converter->local<VariantMemberParam>()) VariantMemberParam(name->to_std_string(), index, is_const, type, defValue, parent_node, location);
}

// ------------------------------AST Methods begin here-----------------------------------------------

std::vector<FunctionParam*>* FunctionTypeget_params(FunctionType* func_type) {
    return &func_type->params;
}

std::vector<BaseType*>* GenericTypeget_types(GenericType* gen_type) {
    return &gen_type->types;
}

Value* AccessChainas_value(AccessChain* chain) {
    // c++ casting is required because Value is in the second place in inheritance
    // and the first ASTNode is virtual containing a pointer
    return chain;
}

std::vector<ChainValue*>* AccessChainget_values(AccessChain* chain) {
    return &chain->values;
}

std::vector<Value*>* ArrayValueget_values(ArrayValue* value) {
    return &value->values;
}

void ArrayValueadd_size(ArrayValue* value, unsigned int size) {
    value->sizes.emplace_back(size);
}

std::vector<Value*>* FunctionCallget_args(FunctionCall* value) {
    return &value->values;
}

std::vector<Value*>* IndexOperatorget_values(IndexOperator* op) {
    return &op->values;
}

std::vector<FunctionParam*>* LambdaFunctionget_params(LambdaFunction* lambdaFunc) {
    return &lambdaFunc->params;
}

std::vector<CapturedVariable*>* LambdaFunctionget_capture_list(LambdaFunction* lambdaFunc) {
    return &lambdaFunc->captureList;
}

std::vector<ASTNode*>* LambdaFunctionget_body(LambdaFunction* lambdaFunc) {
    return &lambdaFunc->scope.nodes;
}

void StructValueadd_value(StructValue* structValue, chem::string* name, StructMemberInitializer* initializer) {
    structValue->values[name->to_std_string()] = initializer;
}

void VariantCaseadd_variable(VariantCase* variantCase, VariantCaseVariable* variable) {
    variantCase->identifier_list.emplace_back(std::move(variable->name), variable->variant_case, variable->location);
}

std::vector<ASTNode*>* DoWhileLoopget_body(DoWhileLoop* loop) {
    return &loop->body.nodes;
}

std::vector<ASTNode*>* WhileLoopget_body(WhileLoop* loop) {
    return &loop->body.nodes;
}

std::vector<ASTNode*>* ForLoopget_body(ForLoop* loop) {
    return &loop->body.nodes;
}

void EnumDeclarationadd_member(EnumDeclaration* decl, EnumMember* member) {
    decl->members[member->name] = member;
}

std::vector<FunctionParam*>* FunctionDeclarationget_params(FunctionDeclaration* decl) {
    return &decl->params;
}

std::vector<GenericTypeParameter*>* FunctionDeclarationget_generic_params(FunctionDeclaration* decl) {
    return &decl->generic_params;
}

std::vector<ASTNode*>* FunctionDeclarationadd_body(FunctionDeclaration* decl) {
    if(!decl->body.has_value()) {
        decl->body.emplace(decl->parent_node, decl->location);
    }
    return &decl->body.value().nodes;
}

std::vector<ASTNode*>* IfStatementget_body(IfStatement* stmt) {
    return &stmt->ifBody.nodes;
}

std::vector<ASTNode*>* IfStatementadd_else_body(IfStatement* stmt) {
    if(!stmt->elseBody.has_value()) {
        stmt->elseBody.emplace(stmt->parent_node, stmt->location);
    }
    return &stmt->elseBody.value().nodes;
}

std::vector<ASTNode*>* IfStatementadd_else_if(IfStatement* stmt, Value* condition) {
    stmt->elseIfs.emplace_back(condition, Scope(stmt->parent_node, stmt->location));
    return &stmt->elseIfs.back().second.nodes;
}

void ImplDefinitionadd_function(ImplDefinition* definition, FunctionDeclaration* decl) {
    definition->insert_multi_func(decl);
}

void StructDefinitionadd_member(StructDefinition* definition, chem::string* name, BaseDefMember* member) {
    definition->variables[name->to_std_string()] = member;
}

void StructDefinitionadd_function(StructDefinition* definition, FunctionDeclaration* decl) {
    definition->insert_multi_func(decl);
}

std::vector<GenericTypeParameter*>* StructDefinitionget_generic_params(StructDefinition* definition) {
    return &definition->generic_params;
}

void InterfaceDefinitionadd_function(InterfaceDefinition* definition, FunctionDeclaration* decl) {
    definition->insert_multi_func(decl);
}

std::vector<ASTNode*>* Namespaceget_body(Namespace* ns) {
    return &ns->nodes;
}

std::vector<ASTNode*>* UnsafeBlockget_body(UnsafeBlock* ub) {
    return &ub->scope.nodes;
}

void UnionDefinitionadd_member(UnionDef* definition, chem::string* name, BaseDefMember* member) {
    definition->variables[name->to_std_string()] = member;
}

void UnionDefinitionadd_function(UnionDef* definition, FunctionDeclaration* decl) {
    definition->insert_multi_func(decl);
}

std::vector<GenericTypeParameter*>* UnionDefinitionget_generic_params(UnionDef* definition) {
    return &definition->generic_params;
}

void VariantDefinitionadd_member(VariantDefinition* definition, chem::string* name, BaseDefMember* member) {
    definition->variables[name->to_std_string()] = member;
}

void VariantMemberadd_param(VariantMember* member, VariantMemberParam* param) {
    member->values[param->name] = param;
}

void InitBlockadd_initializer(InitBlock* block, chem::string* name, bool is_inherited_type, Value* value) {
    block->initializers[name->to_std_string()] = { is_inherited_type, value };
}