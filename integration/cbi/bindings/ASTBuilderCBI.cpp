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
#include "ast/statements/ValueWrapperNode.h"
#include "ast/values/SizeOfValue.h"
#include "ast/types/LinkedValueType.h"
#include "ast/statements/UsingStmt.h"
#include "ast/types/LinkedType.h"
#include "ast/types/DynamicType.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/UnsafeBlock.h"
#include "std/chem_string.h"

/**
 * this function doesn't work, we need to allocate the string on the allocator
 * @deprecated
 */
constexpr LocatedIdentifier LOC_ID(const std::string& identifier, SourceLocation location) {
#ifdef LSP_BUILD
    return { identifier, location };
#else
    return { chem::string_view(identifier.data(), identifier.size()) };
#endif
}

void* ASTBuilderallocate_with_cleanup(ASTAllocator* allocator, std::size_t obj_size, std::size_t alignment, void* cleanup_fn) {
    return (void*) allocator->allocate_with_cleanup(obj_size, alignment, cleanup_fn);
}

AnyType* ASTBuildermake_any_type(ASTAllocator* allocator, uint64_t location) {
    return new (allocator->allocate<AnyType>()) AnyType(location);
}

ArrayType* ASTBuildermake_array_type(ASTAllocator* allocator, BaseType* elem_type, int array_size, uint64_t location) {
    return new (allocator->allocate<ArrayType>()) ArrayType(elem_type, array_size, location);
}

BigIntType* ASTBuildermake_bigint_type(ASTAllocator* allocator, uint64_t location) {
    return new (allocator->allocate<BigIntType>()) BigIntType(location);
}

BoolType* ASTBuildermake_bool_type(ASTAllocator* allocator, uint64_t location) {
    return new (allocator->allocate<BoolType>()) BoolType(location);
}

BoolType* ASTBuildermake_char_type(ASTAllocator* allocator, uint64_t location) {
    return new (allocator->allocate<BoolType>()) BoolType(location);
}

DoubleType* ASTBuildermake_double_type(ASTAllocator* allocator, uint64_t location) {
    return new (allocator->allocate<DoubleType>()) DoubleType(location);
}

DynamicType* ASTBuildermake_dynamic_type(ASTAllocator* allocator, BaseType* child_type, uint64_t location) {
    return new (allocator->allocate<DynamicType>()) DynamicType(child_type, location);
}

FloatType* ASTBuildermake_float_type(ASTAllocator* allocator, uint64_t location) {
    return new (allocator->allocate<FloatType>()) FloatType(location);
}

FunctionType* ASTBuildermake_func_type(ASTAllocator* allocator, BaseType* returnType, bool isVariadic, bool isCapturing, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<FunctionType>()) FunctionType({}, returnType, isVariadic, isCapturing, parent_node, location);
}

GenericType* ASTBuildermake_generic_type(ASTAllocator* allocator, LinkedType* linkedType) {
    return new (allocator->allocate<GenericType>()) GenericType(linkedType);
}

Int128Type* ASTBuildermake_int128_type(ASTAllocator* allocator, uint64_t location) {
    return new (allocator->allocate<Int128Type>()) Int128Type(location);
}

IntType* ASTBuildermake_int_type(ASTAllocator* allocator, uint64_t location) {
    return new (allocator->allocate<IntType>()) IntType(location);
}

LinkedType* ASTBuildermake_linked_type(ASTAllocator* allocator, chem::string_view* type, ASTNode* linked, uint64_t location) {
    return new (allocator->allocate<LinkedType>()) LinkedType(type->str(), linked, location);
}

LinkedValueType* ASTBuildermake_linked_value_type(ASTAllocator* allocator, Value* value, uint64_t location) {
    return new (allocator->allocate<LinkedValueType>()) LinkedValueType(value, location);
}

LiteralType* ASTBuildermake_literal_type(ASTAllocator* allocator, BaseType* child_type, uint64_t location) {
    return new (allocator->allocate<LiteralType>()) LiteralType(child_type, location);
}

LongType* ASTBuildermake_long_type(ASTAllocator* allocator, bool is64Bit, uint64_t location) {
    return new (allocator->allocate<LongType>()) LongType(is64Bit, location);
}

PointerType* ASTBuildermake_ptr_type(ASTAllocator* allocator, BaseType* child_type, uint64_t location) {
    return new (allocator->allocate<PointerType>()) PointerType(child_type, location);
}

ReferenceType* ASTBuildermake_reference_type(ASTAllocator* allocator, BaseType* child_type, uint64_t location) {
    return new (allocator->allocate<ReferenceType>()) ReferenceType(child_type, location);
}

ShortType* ASTBuildermake_short_type(ASTAllocator* allocator, uint64_t location) {
    return new (allocator->allocate<ShortType>()) ShortType(location);
}

StringType* ASTBuildermake_string_type(ASTAllocator* allocator, uint64_t location) {
    return new (allocator->allocate<StringType>()) StringType(location);
}

UBigIntType* ASTBuildermake_ubigint_type(ASTAllocator* allocator, uint64_t location) {
    return new (allocator->allocate<UBigIntType>()) UBigIntType(location);
}

UCharType* ASTBuildermake_uchar_type(ASTAllocator* allocator, uint64_t location) {
    return new (allocator->allocate<UCharType>()) UCharType(location);
}

UInt128Type* ASTBuildermake_uint128_type(ASTAllocator* allocator, uint64_t location) {
    return new (allocator->allocate<UInt128Type>()) UInt128Type(location);
}

UIntType* ASTBuildermake_uint_type(ASTAllocator* allocator, uint64_t location) {
    return new (allocator->allocate<UIntType>()) UIntType(location);
}

ULongType* ASTBuildermake_ulong_type(ASTAllocator* allocator, bool is64Bit, uint64_t location) {
    return new (allocator->allocate<ULongType>()) ULongType(is64Bit, location);
}

UShortType* ASTBuildermake_ushort_type(ASTAllocator* allocator, uint64_t location) {
    return new (allocator->allocate<UShortType>()) UShortType(location);
}

VoidType* ASTBuildermake_void_type(ASTAllocator* allocator, uint64_t location) {
    return new (allocator->allocate<VoidType>()) VoidType(location);
}

AccessChain* ASTBuildermake_access_chain(ASTAllocator* allocator, bool is_node, uint64_t location) {
    return new (allocator->allocate<AccessChain>()) AccessChain(is_node, location);
}

ValueWrapperNode* ASTBuildermake_value_wrapper(ASTAllocator* allocator, Value* value, ASTNode* parent_node) {
    return new (allocator->allocate<ValueWrapperNode>()) ValueWrapperNode(value, parent_node);
}

AddrOfValue* ASTBuildermake_addr_of_value(ASTAllocator* allocator, Value* value, uint64_t location) {
    return new (allocator->allocate<AddrOfValue>()) AddrOfValue(value, location);
}

ArrayValue* ASTBuildermake_array_value(ASTAllocator* allocator, BaseType* type, uint64_t location) {
    return new (allocator->allocate<ArrayValue>()) ArrayValue({}, type, {}, location, *allocator);
}

BigIntValue* ASTBuildermake_bigint_value(ASTAllocator* allocator, long long value, uint64_t location) {
    return new (allocator->allocate<BigIntValue>()) BigIntValue(value, location);
}

BoolValue* ASTBuildermake_bool_value(ASTAllocator* allocator, bool value, uint64_t location) {
    return new (allocator->allocate<BoolValue>()) BoolValue(value, location);
}

CastedValue* ASTBuildermake_casted_value(ASTAllocator* allocator, Value* value, BaseType* type, uint64_t location) {
    return new (allocator->allocate<CastedValue>()) CastedValue(value, type, location);
}

CharValue* ASTBuildermake_char_value(ASTAllocator* allocator, char value, uint64_t location) {
    return new (allocator->allocate<CharValue>()) CharValue(value, location);
}

DereferenceValue* ASTBuildermake_dereference_value(ASTAllocator* allocator, Value* value, uint64_t location) {
    return new (allocator->allocate<DereferenceValue>()) DereferenceValue(value, location);
}

DoubleValue* ASTBuildermake_double_value(ASTAllocator* allocator, double value, uint64_t location) {
    return new (allocator->allocate<DoubleValue>()) DoubleValue(value, location);
}

Expression* ASTBuildermake_expression_value(ASTAllocator* allocator, Value* first, Value* second, Operation op, bool is64Bit, uint64_t location) {
    return new (allocator->allocate<Expression>()) Expression(first, second, op, is64Bit, location);
}

FloatValue* ASTBuildermake_float_value(ASTAllocator* allocator, float value, uint64_t location) {
    return new (allocator->allocate<FloatValue>()) FloatValue(value, location);
}

FunctionCall* ASTBuildermake_function_call_value(ASTAllocator* allocator, uint64_t location) {
    return new (allocator->allocate<FunctionCall>()) FunctionCall({}, location);
}

IndexOperator* ASTBuildermake_index_op_value(ASTAllocator* allocator, uint64_t location) {
    return new (allocator->allocate<IndexOperator>()) IndexOperator({}, location);
}

Int128Value* ASTBuildermake_int128_value(ASTAllocator* allocator, uint64_t mag, bool is_neg, uint64_t location) {
    return new (allocator->allocate<Int128Value>()) Int128Value(mag, is_neg, location);
}

IntValue* ASTBuildermake_int_value(ASTAllocator* allocator, int value, uint64_t location) {
    return new (allocator->allocate<IntValue>()) IntValue(value, location);
}

IsValue* ASTBuildermake_is_value(ASTAllocator* allocator, Value* value, BaseType* type, bool is_negating, uint64_t location) {
    return new (allocator->allocate<IsValue>()) IsValue(value, type, is_negating, location);
}

LambdaFunction* ASTBuildermake_lambda_function(ASTAllocator* allocator, Value* value, BaseType* type, bool isVariadic, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<LambdaFunction>()) LambdaFunction({}, {}, isVariadic, Scope(parent_node, location), parent_node, location);
}

CapturedVariable* ASTBuildermake_captured_variable(ASTAllocator* allocator, chem::string_view* name, unsigned int index, bool capture_by_ref, long value, uint64_t location) {
    return new (allocator->allocate<CapturedVariable>()) CapturedVariable(name->str(), index, capture_by_ref, location);
}

LongValue* ASTBuildermake_long_value(ASTAllocator* allocator, long value, bool is64Bit, uint64_t location) {
    return new (allocator->allocate<LongValue>()) LongValue(value, is64Bit, location);
}

NegativeValue* ASTBuildermake_negative_value(ASTAllocator* allocator, Value* value, uint64_t location) {
    return new (allocator->allocate<NegativeValue>()) NegativeValue(value, location);
}

NotValue* ASTBuildermake_not_value(ASTAllocator* allocator, Value* value, uint64_t location) {
    return new (allocator->allocate<NotValue>()) NotValue(value, location);
}

NullValue* ASTBuildermake_null_value(ASTAllocator* allocator, uint64_t location) {
    return new (allocator->allocate<NullValue>()) NullValue(location);
}

NumberValue* ASTBuildermake_number_value(ASTAllocator* allocator, int64_t value, uint64_t location) {
    return new (allocator->allocate<NumberValue>()) NumberValue(value, location);
}

ShortValue* ASTBuildermake_short_value(ASTAllocator* allocator, short value, uint64_t location) {
    return new (allocator->allocate<ShortValue>()) ShortValue(value, location);
}

SizeOfValue* ASTBuildermake_sizeof_value(ASTAllocator* allocator, BaseType* type, uint64_t location) {
    return new (allocator->allocate<SizeOfValue>()) SizeOfValue(type, location);
}

StringValue* ASTBuildermake_string_value(ASTAllocator* allocator, chem::string_view* value, uint64_t location) {
    return new (allocator->allocate<StringValue>()) StringValue(value->str(), location);
}

StructMemberInitializer* ASTBuildermake_struct_member_initializer(ASTAllocator* allocator, chem::string_view* name, Value* value, StructValue* structValue) {
    return new (allocator->allocate<StructMemberInitializer>()) StructMemberInitializer(name->str(), value, structValue, nullptr);
}

StructValue* ASTBuildermake_struct_value(ASTAllocator* allocator, BaseType* ref, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<StructValue>()) StructValue(ref, {}, nullptr, location, parent_node);
}

UBigIntValue* ASTBuildermake_ubigint_value(ASTAllocator* allocator, unsigned long long value, uint64_t location) {
    return new (allocator->allocate<UBigIntValue>()) UBigIntValue(value, location);
}

UCharValue* ASTBuildermake_uchar_value(ASTAllocator* allocator, unsigned char value, uint64_t location) {
    return new (allocator->allocate<UCharValue>()) UCharValue(value, location);
}

UInt128Value* ASTBuildermake_uint128_value(ASTAllocator* allocator, uint64_t low, uint64_t high, uint64_t location) {
    return new (allocator->allocate<UInt128Value>()) UInt128Value(low, high, location);
}

UIntValue* ASTBuildermake_uint_value(ASTAllocator* allocator, unsigned int value, uint64_t location) {
    return new (allocator->allocate<UIntValue>()) UIntValue(value, location);
}

ULongValue* ASTBuildermake_ulong_value(ASTAllocator* allocator, unsigned long value, bool is64Bit, uint64_t location) {
    return new (allocator->allocate<ULongValue>()) ULongValue(value, is64Bit, location);
}

UShortValue* ASTBuildermake_ushort_value(ASTAllocator* allocator, unsigned short value, uint64_t location) {
    return new (allocator->allocate<UShortValue>()) UShortValue(value, location);
}

ValueNode* ASTBuildermake_value_node(ASTAllocator* allocator, Value* value, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<ValueNode>()) ValueNode(value, parent_node, location);
}

VariableIdentifier* ASTBuildermake_identifier(ASTAllocator* allocator, chem::string_view* value, bool is_ns, uint64_t location) {
    return new (allocator->allocate<VariableIdentifier>()) VariableIdentifier(*value, location, is_ns);
}

VariantCall* ASTBuildermake_variant_call(ASTAllocator* allocator, AccessChain* chain, uint64_t location) {
    return new (allocator->allocate<VariantCall>()) VariantCall(chain, location);
}

VariantCase* ASTBuildermake_variant_case(ASTAllocator* allocator, AccessChain* chain, SwitchStatement* stmt, uint64_t location) {
    return new (allocator->allocate<VariantCase>()) VariantCase(chain, stmt, location);
}

VariantCaseVariable* ASTBuildermake_variant_case_variable(ASTAllocator* allocator, chem::string_view* name, VariantCase* variant_case, uint64_t location) {
    return new (allocator->allocate<VariantCaseVariable>()) VariantCaseVariable(name->str(), variant_case, location);
}

AssignStatement* ASTBuildermake_assignment_stmt(ASTAllocator* allocator, Value* lhs, Value* rhs, Operation op, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<AssignStatement>()) AssignStatement(lhs, rhs, op, parent_node, location);
}

BreakStatement* ASTBuildermake_break_stmt(ASTAllocator* allocator, LoopASTNode* loop_node, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<BreakStatement>()) BreakStatement(loop_node, parent_node, location);
}

Comment* ASTBuildermake_comment_stmt(ASTAllocator* allocator, chem::string_view* value, bool multiline, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<Comment>()) Comment(value->str(), multiline, parent_node, location);
}

ContinueStatement* ASTBuildermake_continue_stmt(ASTAllocator* allocator, LoopASTNode* loop_node, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<ContinueStatement>()) ContinueStatement(loop_node, parent_node, location);
}

DestructStmt* ASTBuildermake_destruct_stmt(ASTAllocator* allocator, Value* array_value, Value* ptr_value, bool is_array, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<DestructStmt>()) DestructStmt(array_value, ptr_value, is_array, parent_node, location);
}

ReturnStatement* ASTBuildermake_return_stmt(ASTAllocator* allocator, Value* value, FunctionType* decl, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<ReturnStatement>()) ReturnStatement(value, decl, parent_node, location);
}

// TODO switch statement when multiple cases have been handled
//SwitchStatement* ASTBuildermake_return_stmt(ASTAllocator* allocator, Value* value, FunctionType* decl, ASTNode* parent_node, uint64_t location) {
//    return new (allocator->allocate<SwitchStatement>()) SwitchStatement(value, decl, parent_node, location);
//}

//ThrowStatement* ASTBuildermake_throw_stmt(ASTAllocator* allocator, Value* value, FunctionType* decl, ASTNode* parent_node, uint64_t location) {
//    return new (allocator->allocate<ThrowStatement>()) ThrowStatement(value, decl, parent_node, location);
//}

TypealiasStatement* ASTBuildermake_typealias_stmt(ASTAllocator* allocator, chem::string_view* identifier, uint64_t id_loc, BaseType* actual_type, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<TypealiasStatement>()) TypealiasStatement(LOC_ID(identifier->str(), id_loc), actual_type, parent_node, location, specifier);
}

UsingStmt* ASTBuildermake_using_stmt(ASTAllocator* allocator, AccessChain* chain, ASTNode* parent_node, bool is_namespace, uint64_t location) {
    return new (allocator->allocate<UsingStmt>()) UsingStmt(chain, parent_node, is_namespace, location);
}

VarInitStatement* ASTBuildermake_varinit_stmt(ASTAllocator* allocator, bool is_const, chem::string_view* identifier, uint64_t id_loc, BaseType* type, Value* value, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<VarInitStatement>()) VarInitStatement(is_const, LOC_ID(identifier->str(), id_loc), type, value, parent_node, location, specifier);
}

// TODO scope needs a children method to get the nodes PtrVec
Scope* ASTBuildermake_scope(ASTAllocator* allocator, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<Scope>()) Scope(parent_node, location);
}

DoWhileLoop* ASTBuildermake_do_while_loop(ASTAllocator* allocator, Value* condition, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<DoWhileLoop>()) DoWhileLoop(condition, LoopScope(parent_node, location), parent_node, location);
}

EnumDeclaration* ASTBuildermake_enum_decl(ASTAllocator* allocator, chem::string_view* name, uint64_t name_loc, IntNType* underlying_type, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<EnumDeclaration>()) EnumDeclaration(LOC_ID(name->str(), name_loc), {}, underlying_type, parent_node, location, specifier);
}

EnumMember* ASTBuildermake_enum_member(ASTAllocator* allocator, chem::string_view* name, unsigned int index, Value* init_value, EnumDeclaration* parent_node, uint64_t location) {
    return new (allocator->allocate<EnumMember>()) EnumMember(name->data(), index, init_value, parent_node, location);
}

ForLoop* ASTBuildermake_for_loop(ASTAllocator* allocator, VarInitStatement* initializer, Value* conditionExpr, ASTNode* incrementerExpr, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<ForLoop>()) ForLoop(initializer, conditionExpr, incrementerExpr, LoopScope(parent_node, location), parent_node, location);
}

FunctionDeclaration* ASTBuildermake_function(ASTAllocator* allocator, chem::string_view* name, uint64_t name_location, BaseType* returnType, bool isVariadic, bool hasBody, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<FunctionDeclaration>()) FunctionDeclaration(LOC_ID(name->str(), name_location), {}, returnType, isVariadic, parent_node, location, std::nullopt);
}

FunctionParam* ASTBuildermake_function_param(ASTAllocator* allocator, chem::string_view* name, BaseType* type, unsigned int index, Value* value, bool implicit, FunctionType* decl, uint64_t location) {
    return new (allocator->allocate<FunctionParam>()) FunctionParam(name->str(), type, index, value, implicit, decl, location);
}

GenericTypeParameter* ASTBuildermake_generic_param(ASTAllocator* allocator, chem::string_view* name, BaseType* at_least_type, BaseType* def_type, ASTNode* parent_node, unsigned int index, uint64_t location) {
    return new (allocator->allocate<GenericTypeParameter>()) GenericTypeParameter(name->str(), at_least_type, def_type, parent_node, index, location);
}

IfStatement* ASTBuildermake_if_stmt(ASTAllocator* allocator, Value* condition, bool is_value, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<IfStatement>()) IfStatement(condition, Scope(parent_node, location), {}, std::nullopt, parent_node, is_value, location);
}

ImplDefinition* ASTBuildermake_impl_def(ASTAllocator* allocator, BaseType* interface_type, BaseType* struct_type, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<ImplDefinition>()) ImplDefinition(interface_type, struct_type, parent_node, location);
}

InitBlock* ASTBuildermake_init_block(ASTAllocator* allocator, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<InitBlock>()) InitBlock(Scope(parent_node, location), parent_node, location);
}

InterfaceDefinition* ASTBuildermake_interface_def(ASTAllocator* allocator, chem::string_view* name, uint64_t name_location, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<InterfaceDefinition>()) InterfaceDefinition(LOC_ID(name->str(), name_location), parent_node, location, specifier);
}

Namespace* ASTBuildermake_namespace(ASTAllocator* allocator, chem::string_view* name, uint64_t name_location, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<Namespace>()) Namespace(LOC_ID(name->str(), name_location), parent_node, location, specifier);
}

StructDefinition* ASTBuildermake_struct_def(ASTAllocator* allocator, chem::string_view* name, uint64_t name_location, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<StructDefinition>()) StructDefinition(LOC_ID(name->str(), name_location), parent_node, location, specifier);
}

StructMember* ASTBuildermake_struct_member(ASTAllocator* allocator, chem::string_view* name, BaseType* type, Value* defValue, bool isConst, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<StructMember>()) StructMember(name->str(), type, defValue, parent_node, location, isConst, specifier);
}

UnionDef* ASTBuildermake_union_def(ASTAllocator* allocator, chem::string_view* name, uint64_t name_location, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<UnionDef>()) UnionDef(LOC_ID(name->str(), name_location), parent_node, location, specifier);
}

UnsafeBlock* ASTBuildermake_unsafe_block(ASTAllocator* allocator, ASTNode* node, uint64_t location) {
    return new (allocator->allocate<UnsafeBlock>()) UnsafeBlock(Scope(node, location), location);
}

WhileLoop* ASTBuildermake_while_loop(ASTAllocator* allocator, Value* condition, ASTNode* node, uint64_t location) {
    return new (allocator->allocate<WhileLoop>()) WhileLoop(condition, LoopScope(node, location), node, location);
}

VariantDefinition* ASTBuildermake_variant_def(ASTAllocator* allocator, chem::string_view* name, uint64_t name_location, AccessSpecifier specifier, ASTNode* node, uint64_t location) {
    return new (allocator->allocate<VariantDefinition>()) VariantDefinition(LOC_ID(name->str(), name_location), node, location, specifier);
}

VariantMember* ASTBuildermake_variant_member(ASTAllocator* allocator, chem::string_view* name, VariantDefinition* parent_node, uint64_t location) {
    return new (allocator->allocate<VariantMember>()) VariantMember(name->str(), parent_node, location);
}

VariantMemberParam* ASTBuildermake_variant_member_param(ASTAllocator* allocator, chem::string_view* name, unsigned int index, bool is_const, BaseType* type, Value* defValue, VariantMember* parent_node, uint64_t location) {
    return new (allocator->allocate<VariantMemberParam>()) VariantMemberParam(name->str(), index, is_const, type, defValue, parent_node, location);
}

// ------------------------------AST Methods begin here-----------------------------------------------

int ValuegetKind(Value* value) {
    return static_cast<int>(value->val_kind());
}

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

void StructValueadd_value(StructValue* structValue, chem::string_view* name, StructMemberInitializer* initializer) {
    structValue->values[name->str()] = initializer;
}

void VariantCaseadd_variable(VariantCase* variantCase, VariantCaseVariable* variable) {
    variantCase->identifier_list.emplace_back(std::move(variable->name), variable->variant_case, variable->location);
}

std::vector<ASTNode*>* ScopegetNodes(Scope* scope) {
    return &scope->nodes;
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

void StructDefinitionadd_member(StructDefinition* definition, chem::string_view* name, BaseDefMember* member) {
    definition->variables[name->str()] = member;
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

void UnionDefinitionadd_member(UnionDef* definition, chem::string_view* name, BaseDefMember* member) {
    definition->variables[name->str()] = member;
}

void UnionDefinitionadd_function(UnionDef* definition, FunctionDeclaration* decl) {
    definition->insert_multi_func(decl);
}

std::vector<GenericTypeParameter*>* UnionDefinitionget_generic_params(UnionDef* definition) {
    return &definition->generic_params;
}

void VariantDefinitionadd_member(VariantDefinition* definition, chem::string_view* name, BaseDefMember* member) {
    definition->variables[name->str()] = member;
}

void VariantMemberadd_param(VariantMember* member, VariantMemberParam* param) {
    member->values[param->name] = param;
}

void InitBlockadd_initializer(InitBlock* block, chem::string_view* name, bool is_inherited_type, Value* value) {
    block->initializers[name->str()] = { is_inherited_type, value };
}