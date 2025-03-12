// Copyright (c) Chemical Language Foundation 2025.

#include "ASTBuilderCBI.h"
#include "ASTCBI.h"
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
#include "ast/values/AccessChain.h"
#include "ast/values/ValueNode.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/IntValue.h"
#include "ast/values/UInt128Value.h"
#include "ast/values/LongValue.h"
#include "ast/values/ULongValue.h"
#include "ast/values/Negative.h"
#include "ast/values/BlockValue.h"
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
#include "ast/statements/SymResNode.h"
#include "ast/values/SymResValue.h"
#include "std/chem_string.h"

constexpr LocatedIdentifier LOC_ID(const chem::string_view& identifier, SourceLocation location) {
#ifdef LSP_BUILD
    return { identifier, location };
#else
    return { identifier };
#endif
}

void* ASTBuilderallocate_with_cleanup(ASTAllocator* allocator, std::size_t obj_size, std::size_t alignment, void* cleanup_fn) {
    return (void*) allocator->allocate_with_cleanup(obj_size, alignment, cleanup_fn);
}

BaseType* ASTBuildercreateValueType(ASTAllocator* allocator, ASTNode* node) {
    return node->create_value_type(*allocator);
}

BaseType* ASTBuildercreateType(ASTAllocator* allocator, Value* value) {
    return value->create_type(*allocator);
}

SymResNode* ASTBuildermake_sym_res_node(ASTAllocator* allocator, void* decl_fn, void* repl_fn, void* data_ptr, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<SymResNode>()) SymResNode(allocator, (SymResNodeDeclarationFn) decl_fn, (SymResNodeReplacementFn) repl_fn, data_ptr, parent_node, location);
}

SymResValue* ASTBuildermake_sym_res_value(ASTAllocator* allocator, void* repl_fn, void* data_ptr, uint64_t location) {
    return new (allocator->allocate<SymResValue>()) SymResValue(allocator, (SymResValueReplacementFn) repl_fn, data_ptr, location);
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
    // TODO function type doesn't take a parent_node
    return new (allocator->allocate<FunctionType>()) FunctionType(returnType, isVariadic, isCapturing, location);
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
    return new (allocator->allocate<LinkedType>()) LinkedType(*type, linked, location);
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

FunctionCall* ASTBuildermake_function_call_value(ASTAllocator* allocator, ChainValue* parent_val, uint64_t location) {
    return new (allocator->allocate<FunctionCall>()) FunctionCall(parent_val, location);
}

IndexOperator* ASTBuildermake_index_op_value(ASTAllocator* allocator, ChainValue* parent_val, uint64_t location) {
    return new (allocator->allocate<IndexOperator>()) IndexOperator(parent_val, location);
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
    return new (allocator->allocate<LambdaFunction>()) LambdaFunction(isVariadic, parent_node, location);
}

CapturedVariable* ASTBuildermake_captured_variable(ASTAllocator* allocator, chem::string_view* name, unsigned int index, bool capture_by_ref, long value, uint64_t location) {
    // TODO passing nullptr as parent in captured variable
    return new (allocator->allocate<CapturedVariable>()) CapturedVariable(*name, index, capture_by_ref, nullptr, location);
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
    return new (allocator->allocate<StringValue>()) StringValue(*value, location);
}

StructValue* ASTBuildermake_struct_value(ASTAllocator* allocator, BaseType* ref, ASTNode* parent_node, uint64_t location) {
    // TODO do not take parent_node as parameter
    return new (allocator->allocate<StructValue>()) StructValue(ref, location);
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

BlockValue* ASTBuildermake_block_value(ASTAllocator* allocator, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<BlockValue>()) BlockValue(parent_node, location);
}

ValueNode* ASTBuildermake_value_node(ASTAllocator* allocator, Value* value, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<ValueNode>()) ValueNode(value, parent_node, location);
}

VariableIdentifier* ASTBuildermake_identifier(ASTAllocator* allocator, chem::string_view* value, bool is_ns, uint64_t location) {
    return new (allocator->allocate<VariableIdentifier>()) VariableIdentifier(*value, location, is_ns);
}

VariantCase* ASTBuildermake_variant_case(ASTAllocator* allocator, Value* parent_value, SwitchStatement* stmt, uint64_t location) {
    return new (allocator->allocate<VariantCase>()) VariantCase(parent_value, stmt, location);
}

VariantCaseVariable* ASTBuildermake_variant_case_variable(ASTAllocator* allocator, chem::string_view* name, VariableIdentifier* parent_val, SwitchStatement* switch_stmt, uint64_t location) {
    return new (allocator->allocate<VariantCaseVariable>()) VariantCaseVariable(*name, parent_val, switch_stmt, location);
}

AssignStatement* ASTBuildermake_assignment_stmt(ASTAllocator* allocator, Value* lhs, Value* rhs, Operation op, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<AssignStatement>()) AssignStatement(lhs, rhs, op, parent_node, location);
}

BreakStatement* ASTBuildermake_break_stmt(ASTAllocator* allocator, LoopASTNode* loop_node, ASTNode* parent_node, uint64_t location) {
    // TODO do not take loop_node
    return new (allocator->allocate<BreakStatement>()) BreakStatement(parent_node, location);
}

ContinueStatement* ASTBuildermake_continue_stmt(ASTAllocator* allocator, LoopASTNode* loop_node, ASTNode* parent_node, uint64_t location) {
    // TODO do not take loop_node
    return new (allocator->allocate<ContinueStatement>()) ContinueStatement(parent_node, location);
}

DestructStmt* ASTBuildermake_destruct_stmt(ASTAllocator* allocator, Value* array_value, Value* ptr_value, bool is_array, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<DestructStmt>()) DestructStmt(array_value, ptr_value, is_array, parent_node, location);
}

ReturnStatement* ASTBuildermake_return_stmt(ASTAllocator* allocator, Value* value, FunctionTypeBody* decl, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<ReturnStatement>()) ReturnStatement(value, parent_node, location);
}

// TODO switch statement when multiple cases have been handled
//SwitchStatement* ASTBuildermake_return_stmt(ASTAllocator* allocator, Value* value, FunctionType* decl, ASTNode* parent_node, uint64_t location) {
//    return new (allocator->allocate<SwitchStatement>()) SwitchStatement(value, decl, parent_node, location);
//}

//ThrowStatement* ASTBuildermake_throw_stmt(ASTAllocator* allocator, Value* value, FunctionType* decl, ASTNode* parent_node, uint64_t location) {
//    return new (allocator->allocate<ThrowStatement>()) ThrowStatement(value, decl, parent_node, location);
//}

TypealiasStatement* ASTBuildermake_typealias_stmt(ASTAllocator* allocator, chem::string_view* identifier, uint64_t id_loc, BaseType* actual_type, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<TypealiasStatement>()) TypealiasStatement(LOC_ID(*identifier, id_loc), actual_type, parent_node, location, specifier);
}

UsingStmt* ASTBuildermake_using_stmt(ASTAllocator* allocator, AccessChain* chain, ASTNode* parent_node, bool is_namespace, uint64_t location) {
    return new (allocator->allocate<UsingStmt>()) UsingStmt(chain, parent_node, is_namespace, location);
}

VarInitStatement* ASTBuildermake_varinit_stmt(ASTAllocator* allocator, bool is_const, bool is_reference, chem::string_view* identifier, uint64_t id_loc, BaseType* type, Value* value, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<VarInitStatement>()) VarInitStatement(is_const, is_reference, LOC_ID(*identifier, id_loc), type, value, parent_node, location, specifier);
}

// TODO scope needs a children method to get the nodes PtrVec
Scope* ASTBuildermake_scope(ASTAllocator* allocator, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<Scope>()) Scope(parent_node, location);
}

DoWhileLoop* ASTBuildermake_do_while_loop(ASTAllocator* allocator, Value* condition, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<DoWhileLoop>()) DoWhileLoop(condition, parent_node, location);
}

EnumDeclaration* ASTBuildermake_enum_decl(ASTAllocator* allocator, chem::string_view* name, uint64_t name_loc, IntNType* underlying_type, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<EnumDeclaration>()) EnumDeclaration(LOC_ID(*name, name_loc), underlying_type, parent_node, location, specifier);
}

EnumMember* ASTBuildermake_enum_member(ASTAllocator* allocator, chem::string_view* name, unsigned int index, Value* init_value, EnumDeclaration* parent_node, uint64_t location) {
    return new (allocator->allocate<EnumMember>()) EnumMember(name->data(), index, init_value, parent_node, location);
}

ForLoop* ASTBuildermake_for_loop(ASTAllocator* allocator, VarInitStatement* initializer, Value* conditionExpr, ASTNode* incrementerExpr, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<ForLoop>()) ForLoop(initializer, conditionExpr, incrementerExpr, parent_node, location);
}

FunctionDeclaration* ASTBuildermake_function(ASTAllocator* allocator, chem::string_view* name, uint64_t name_location, BaseType* returnType, bool isVariadic, bool hasBody, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<FunctionDeclaration>()) FunctionDeclaration(LOC_ID(*name, name_location), returnType, isVariadic, parent_node, location);
}

FunctionParam* ASTBuildermake_function_param(ASTAllocator* allocator, chem::string_view* name, BaseType* type, unsigned int index, Value* value, bool implicit, FunctionType* decl, uint64_t location) {
    // TODO casting function type as parent node, this is wrong
    return new (allocator->allocate<FunctionParam>()) FunctionParam(*name, type, index, value, implicit, (ASTNode*) decl, location);
}

GenericTypeParameter* ASTBuildermake_generic_param(ASTAllocator* allocator, chem::string_view* name, BaseType* at_least_type, BaseType* def_type, ASTNode* parent_node, unsigned int index, uint64_t location) {
    return new (allocator->allocate<GenericTypeParameter>()) GenericTypeParameter(*name, at_least_type, def_type, parent_node, index, location);
}

IfStatement* ASTBuildermake_if_stmt(ASTAllocator* allocator, Value* condition, bool is_value, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<IfStatement>()) IfStatement(condition, parent_node, is_value, location);
}

ImplDefinition* ASTBuildermake_impl_def(ASTAllocator* allocator, BaseType* interface_type, BaseType* struct_type, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<ImplDefinition>()) ImplDefinition(interface_type, struct_type, parent_node, location);
}

InitBlock* ASTBuildermake_init_block(ASTAllocator* allocator, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<InitBlock>()) InitBlock(parent_node, location);
}

InterfaceDefinition* ASTBuildermake_interface_def(ASTAllocator* allocator, chem::string_view* name, uint64_t name_location, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<InterfaceDefinition>()) InterfaceDefinition(LOC_ID(*name, name_location), parent_node, location, specifier);
}

Namespace* ASTBuildermake_namespace(ASTAllocator* allocator, chem::string_view* name, uint64_t name_location, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<Namespace>()) Namespace(LOC_ID(*name, name_location), parent_node, location, specifier);
}

StructDefinition* ASTBuildermake_struct_def(ASTAllocator* allocator, chem::string_view* name, uint64_t name_location, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<StructDefinition>()) StructDefinition(LOC_ID(*name, name_location), parent_node, location, specifier);
}

StructMember* ASTBuildermake_struct_member(ASTAllocator* allocator, chem::string_view* name, BaseType* type, Value* defValue, bool isConst, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<StructMember>()) StructMember(*name, type, defValue, parent_node, location, isConst, specifier);
}

UnionDef* ASTBuildermake_union_def(ASTAllocator* allocator, chem::string_view* name, uint64_t name_location, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (allocator->allocate<UnionDef>()) UnionDef(LOC_ID(*name, name_location), parent_node, location, specifier);
}

UnsafeBlock* ASTBuildermake_unsafe_block(ASTAllocator* allocator, ASTNode* node, uint64_t location) {
    return new (allocator->allocate<UnsafeBlock>()) UnsafeBlock(node, location);
}

WhileLoop* ASTBuildermake_while_loop(ASTAllocator* allocator, Value* condition, ASTNode* node, uint64_t location) {
    return new (allocator->allocate<WhileLoop>()) WhileLoop(condition, node, location);
}

VariantDefinition* ASTBuildermake_variant_def(ASTAllocator* allocator, chem::string_view* name, uint64_t name_location, AccessSpecifier specifier, ASTNode* node, uint64_t location) {
    return new (allocator->allocate<VariantDefinition>()) VariantDefinition(LOC_ID(*name, name_location), node, location, specifier);
}

VariantMember* ASTBuildermake_variant_member(ASTAllocator* allocator, chem::string_view* name, VariantDefinition* parent_node, uint64_t location) {
    return new (allocator->allocate<VariantMember>()) VariantMember(*name, parent_node, location);
}

VariantMemberParam* ASTBuildermake_variant_member_param(ASTAllocator* allocator, chem::string_view* name, unsigned int index, bool is_const, BaseType* type, Value* defValue, VariantMember* parent_node, uint64_t location) {
    return new (allocator->allocate<VariantMemberParam>()) VariantMemberParam(*name, index, is_const, type, defValue, parent_node, location);
}

// ------------------------------AST Methods begin here-----------------------------------------------

// Value mtehods

int ValuegetKind(Value* value) {
    return static_cast<int>(value->val_kind());
}

bool Valuelink(Value* value, Value** ptr_ref, BaseType* expected_type, SymbolResolver* resolver) {
    return value->link(*resolver, *ptr_ref, expected_type);
}

ASTNode* ValuegetLinkedNode(Value* value) {
    return value->linked_node();
}

// ASTNode methods

int ASTNodegetKind(ASTNode* node) {
    return static_cast<int>(node->kind());
}

void ASTNodedeclare_top_level(ASTNode* node, ASTNode** ptr_ref, SymbolResolver* resolver) {
    node->declare_top_level(*resolver, *ptr_ref);
}

void ASTNodedeclare_and_link(ASTNode* node, ASTNode** ptr_ref, SymbolResolver* resolver) {
    node->declare_and_link(*resolver, *ptr_ref);
}

// BaseType methods

int BaseTypegetKind(BaseType* type) {
    return static_cast<int>(type->kind());
}

bool BaseTypelink(BaseType* type, BaseType** ptr_ref, SymbolResolver* resolver) {
    return type->link(*resolver);
}

ASTNode* BaseTypegetLinkedNode(BaseType* type) {
    return type->linked_node();
}

// Other methods

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

void StructValueadd_value(StructValue* structValue, chem::string_view* name, Value* initializer) {
    structValue->values.emplace(*name, StructMemberInitializer { *name, initializer });
}

void VariantCaseadd_variable(VariantCase* variantCase, VariantCaseVariable* variable) {
    // TODO remove this method
    variantCase->identifier_list.emplace_back(std::move(variable->name), nullptr, nullptr, variable->encoded_location());
}

std::vector<ASTNode*>* ScopegetNodes(Scope* scope) {
    return &scope->nodes;
}

void Scopelink_sequentially(Scope* scope, SymbolResolver* resolver) {
    scope->link_sequentially(*resolver);
}

void Scopelink_asynchronously(Scope* scope, SymbolResolver* resolver) {
    scope->link_asynchronously(*resolver);
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

std::vector<ASTNode*>* FunctionDeclarationadd_body(FunctionDeclaration* decl) {
    if(!decl->body.has_value()) {
        decl->body.emplace(decl->parent(), decl->ASTNode::encoded_location());
    }
    return &decl->body.value().nodes;
}

std::vector<ASTNode*>* IfStatementget_body(IfStatement* stmt) {
    return &stmt->ifBody.nodes;
}

std::vector<ASTNode*>* IfStatementadd_else_body(IfStatement* stmt) {
    if(!stmt->elseBody.has_value()) {
        stmt->elseBody.emplace(stmt->parent(), stmt->ASTNode::encoded_location());
    }
    return &stmt->elseBody.value().nodes;
}

std::vector<ASTNode*>* IfStatementadd_else_if(IfStatement* stmt, Value* condition) {
    stmt->elseIfs.emplace_back(condition, Scope(stmt->parent(), stmt->ASTNode::encoded_location()));
    return &stmt->elseIfs.back().second.nodes;
}

void ImplDefinitionadd_function(ImplDefinition* definition, ASTAllocator* astAllocator, FunctionDeclaration* decl) {
    definition->insert_multi_func(*astAllocator, decl);
}

void StructDefinitionadd_member(StructDefinition* definition, chem::string_view* name, BaseDefMember* member) {
    definition->variables[*name] = member;
}

void StructDefinitionadd_function(StructDefinition* definition, ASTAllocator* astAllocator, FunctionDeclaration* decl) {
    definition->insert_multi_func(*astAllocator, decl);
}

void InterfaceDefinitionadd_function(InterfaceDefinition* definition, ASTAllocator* astAllocator, FunctionDeclaration* decl) {
    definition->insert_multi_func(*astAllocator, decl);
}

std::vector<ASTNode*>* Namespaceget_body(Namespace* ns) {
    return &ns->nodes;
}

std::vector<ASTNode*>* UnsafeBlockget_body(UnsafeBlock* ub) {
    return &ub->scope.nodes;
}

std::vector<ASTNode*>* BlockValueget_body(BlockValue* bv) {
    return &bv->scope.nodes;
}

void UnionDefinitionadd_member(UnionDef* definition, chem::string_view* name, BaseDefMember* member) {
    definition->variables[*name] = member;
}

void UnionDefinitionadd_function(UnionDef* definition, ASTAllocator* astAllocator, FunctionDeclaration* decl) {
    definition->insert_multi_func(*astAllocator, decl);
}

void VariantDefinitionadd_member(VariantDefinition* definition, chem::string_view* name, BaseDefMember* member) {
    definition->variables[*name] = member;
}

void VariantMemberadd_param(VariantMember* member, VariantMemberParam* param) {
    member->values[param->name] = param;
}

void InitBlockadd_initializer(InitBlock* block, chem::string_view* name, Value* value) {
    block->initializers[*name] = { value };
}