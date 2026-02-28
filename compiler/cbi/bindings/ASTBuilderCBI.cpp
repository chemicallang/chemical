// Copyright (c) Chemical Language Foundation 2025.

#include "ASTBuilderCBI.h"
#include "ASTCBI.h"
#include "compiler/cbi/model/ASTBuilder.h"
#include "ast/base/TypeBuilder.h"
#include "ast/types/AnyType.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/statements/VarInit.h"
#include "ast/types/IntNType.h"
#include "ast/types/VoidType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/values/ExpressiveString.h"
#include "ast/types/LinkedType.h"
#include "ast/statements/Assignment.h"
#include "ast/types/ArrayType.h"
#include "ast/values/StringValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/Expression.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/VariantCase.h"
#include "ast/values/BoolValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/AccessChain.h"
#include "ast/values/ValueNode.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/Negative.h"
#include "ast/values/BlockValue.h"
#include "ast/values/NullValue.h"
#include "ast/values/IsValue.h"
#include "ast/values/VariantCaseVariable.h"
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
#include "ast/values/EmbeddedValue.h"
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
#include "ast/statements/AccessChainNode.h"
#include "ast/values/ArrayValue.h"
#include "ast/structures/If.h"
#include "ast/values/LambdaFunction.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/LoopBlock.h"
#include "ast/values/CastedValue.h"
#include "ast/statements/ThrowStatement.h"
#include "ast/statements/ValueWrapperNode.h"
#include "ast/values/SizeOfValue.h"
#include "ast/types/LinkedValueType.h"
#include "ast/statements/UsingStmt.h"
#include "ast/types/LinkedType.h"
#include "ast/types/DynamicType.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/EnumMember.h"
#include "ast/structures/UnsafeBlock.h"
#include "std/chem_string.h"
#include "compiler/symres/SymResLinkBodyAPI.h"
#include "ast/statements/EmbeddedNode.h"

constexpr LocatedIdentifier LOC_ID(const chem::string_view& identifier, SourceLocation location) {
#ifdef LSP_BUILD
    return { identifier };
#else
    return { identifier };
#endif
}

void* ASTBuilderallocate_with_cleanup(ASTBuilder* builder, std::size_t obj_size, std::size_t alignment, void* cleanup_fn) {
    return (void*) builder->allocator->allocate_with_cleanup(obj_size, alignment, cleanup_fn);
}

void ASTBuilderstore_cleanup(ASTBuilder* builder, void* obj, void* cleanup_fn) {
    builder->allocator->store_cleanup_fn(obj, cleanup_fn);
}

void take_chemical_values(std::vector<Value*>& values, ValueSpan* chemical_values) {
    if(chemical_values->size == 0) return;
    values.reserve(chemical_values->size);
    auto start = chemical_values->ptr;
    const auto end = start + chemical_values->size;
    while(start != end) {
        values.emplace_back(*start);
        start++;
    }
}

void take_chemical_nodes(std::vector<ASTNode*>& nodes, NodeSpan* chemical_nodes) {
    if(chemical_nodes->size == 0) return;
    nodes.reserve(chemical_nodes->size);
    auto start = chemical_nodes->ptr;
    const auto end = start + chemical_nodes->size;
    while(start != end) {
        nodes.emplace_back(*start);
        start++;
    }
}

EmbeddedNode* ASTBuildermake_embedded_node(ASTBuilder* builder, chem::string_view* name, void* data_ptr, void* known_type_fn, void* child_res_fn, NodeSpan* chemical_nodes, ValueSpan* chemical_values, ASTNode* parent_node, uint64_t location) {
    const auto node = new (builder->allocate<EmbeddedNode>()) EmbeddedNode(
            *name,
            data_ptr,
            (EmbeddedNodeKnownTypeFunc*) known_type_fn,
            (EmbeddedNodeChildResolutionFunc*) child_res_fn,
            parent_node,
            location
    );
    take_chemical_nodes(node->chemical_nodes, chemical_nodes);
    take_chemical_values(node->chemical_values, chemical_values);
    return node;
}

EmbeddedValue* ASTBuildermake_embedded_value(ASTBuilder* builder, chem::string_view* name, void* data_ptr, BaseType* type, NodeSpan* chemical_nodes, ValueSpan* chemical_values, uint64_t location) {
    const auto value = new (builder->allocate<EmbeddedValue>()) EmbeddedValue(
            *name,
            data_ptr,
            type,
            location
    );
    take_chemical_nodes(value->chemical_nodes, chemical_nodes);
    take_chemical_values(value->chemical_values, chemical_values);
    return value;
}

AnyType* ASTBuildermake_any_type(ASTBuilder* builder, uint64_t location) {
    return builder->typeBuilder.getAnyType();
}

ArrayType* ASTBuildermake_array_type(ASTBuilder* builder, BaseType* elem_type, int array_size, uint64_t location) {
    return new (builder->allocate<ArrayType>()) ArrayType({elem_type, location}, array_size);
}

BoolType* ASTBuildermake_bool_type(ASTBuilder* builder, uint64_t location) {
    return builder->typeBuilder.getBoolType();
}

CharType* ASTBuilderget_char_type(ASTBuilder* builder) { return builder->typeBuilder.getCharType(); }
ShortType* ASTBuilderget_short_type(ASTBuilder* builder) { return builder->typeBuilder.getShortType(); }
IntType* ASTBuilderget_int_type(ASTBuilder* builder) { return builder->typeBuilder.getIntType(); }
LongType* ASTBuilderget_long_type(ASTBuilder* builder) { return builder->typeBuilder.getLongType(); }
LongLongType* ASTBuilderget_longlong_type(ASTBuilder* builder) { return builder->typeBuilder.getLongLongType(); }

UCharType* ASTBuilderget_uchar_type(ASTBuilder* builder) { return builder->typeBuilder.getUCharType(); }
UShortType* ASTBuilderget_ushort_type(ASTBuilder* builder) { return builder->typeBuilder.getUShortType(); }
UIntType* ASTBuilderget_uint_type(ASTBuilder* builder) { return builder->typeBuilder.getUIntType(); }
ULongType* ASTBuilderget_ulong_type(ASTBuilder* builder) { return builder->typeBuilder.getULongType(); }
ULongLongType* ASTBuilderget_ulonglong_type(ASTBuilder* builder) { return builder->typeBuilder.getULongLongType(); }

I8Type* ASTBuilderget_i8_type(ASTBuilder* builder) { return builder->typeBuilder.getI8Type(); }
I16Type* ASTBuilderget_i16_type(ASTBuilder* builder) { return builder->typeBuilder.getI16Type(); }
I32Type* ASTBuilderget_i32_type(ASTBuilder* builder) { return builder->typeBuilder.getI32Type(); }
I64Type* ASTBuilderget_i64_type(ASTBuilder* builder) { return builder->typeBuilder.getI64Type(); }
Int128Type* ASTBuilderget_i128_type(ASTBuilder* builder) { return builder->typeBuilder.getInt128Type(); }

U8Type* ASTBuilderget_u8_type(ASTBuilder* builder) { return builder->typeBuilder.getU8Type(); }
U16Type* ASTBuilderget_u16_type(ASTBuilder* builder) { return builder->typeBuilder.getU16Type(); }
U32Type* ASTBuilderget_u32_type(ASTBuilder* builder) { return builder->typeBuilder.getU32Type(); }
U64Type* ASTBuilderget_u64_type(ASTBuilder* builder) { return builder->typeBuilder.getU64Type(); }
UInt128Type* ASTBuilderget_u128_type(ASTBuilder* builder) { return builder->typeBuilder.getUInt128Type(); }

DoubleType* ASTBuildermake_double_type(ASTBuilder* builder, uint64_t location) {
    return builder->typeBuilder.getDoubleType();
}

DynamicType* ASTBuildermake_dynamic_type(ASTBuilder* builder, BaseType* child_type, uint64_t location) {
    return new (builder->allocate<DynamicType>()) DynamicType(child_type);
}

FloatType* ASTBuildermake_float_type(ASTBuilder* builder, uint64_t location) {
    return builder->typeBuilder.getFloatType();
}

FunctionType* ASTBuildermake_func_type(ASTBuilder* builder, BaseType* returnType, bool isVariadic, bool isCapturing, ASTNode* parent_node, uint64_t location) {
    // TODO function type doesn't take a parent_node
    return new (builder->allocate<FunctionType>()) FunctionType({returnType, location}, isVariadic, isCapturing, location);
}

GenericType* ASTBuildermake_generic_type(ASTBuilder* builder, LinkedType* linkedType) {
    return new (builder->allocate<GenericType>()) GenericType(linkedType);
}

LinkedType* ASTBuildermake_linked_type(ASTBuilder* builder, chem::string_view* type, ASTNode* linked, uint64_t location) {
    return new (builder->allocate<LinkedType>()) LinkedType(linked);
}

LinkedValueType* ASTBuildermake_linked_value_type(ASTBuilder* builder, Value* value, uint64_t location) {
    return new (builder->allocate<LinkedValueType>()) LinkedValueType(value);
}

LiteralType* ASTBuildermake_literal_type(ASTBuilder* builder, BaseType* child_type, uint64_t location) {
    return new (builder->allocate<LiteralType>()) LiteralType(child_type);
}

PointerType* ASTBuildermake_ptr_type(ASTBuilder* builder, BaseType* child_type, uint64_t location) {
    return new (builder->allocate<PointerType>()) PointerType(child_type, false);
}

ReferenceType* ASTBuildermake_reference_type(ASTBuilder* builder, BaseType* child_type, uint64_t location) {
    return new (builder->allocate<ReferenceType>()) ReferenceType(child_type, false);
}

StringType* ASTBuildermake_string_type(ASTBuilder* builder, uint64_t location) {
    return builder->typeBuilder.getStringType();
}

VoidType* ASTBuildermake_void_type(ASTBuilder* builder, uint64_t location) {
    return builder->typeBuilder.getVoidType();
}

static inline void initialize(AccessChain* chain, chem::span<Value*>& values) {
    for(const auto value : values) {
        chain->values.emplace_back(value);
    }
    chain->setType(values.back()->getType());
}

AccessChain* ASTBuildermake_access_chain(ASTBuilder* builder, chem::span<Value*>* values, uint64_t location) {
    const auto chain = new (builder->allocate<AccessChain>()) AccessChain(location);
    initialize(chain, *values);
    return chain;
}

ASTNode* ASTBuildermake_access_chain_node(ASTBuilder* builder, chem::span<Value*>* values, ASTNode* parent_node, uint64_t location) {
    const auto chain = new (builder->allocate<AccessChainNode>()) AccessChainNode(location, parent_node);
    initialize(&chain->chain, *values);
    return chain;
}

ValueWrapperNode* ASTBuildermake_value_wrapper(ASTBuilder* builder, Value* value, ASTNode* parent_node) {
    return new (builder->allocate<ValueWrapperNode>()) ValueWrapperNode(value, parent_node);
}

AddrOfValue* ASTBuildermake_addr_of_value(ASTBuilder* builder, Value* value, uint64_t location) {
    // TODO get if this is a mutable reference
    return new (builder->allocate<AddrOfValue>()) AddrOfValue(value, true, location);
}

ArrayValue* ASTBuildermake_array_value(ASTBuilder* builder, BaseType* type, uint64_t location) {
    return new (builder->allocate<ArrayValue>()) ArrayValue({type, location}, location, *builder->allocator);
}

IntNumValue* ASTBuildermake_bigint_value(ASTBuilder* builder, long long value, uint64_t location) {
    return new (builder->allocate<IntNumValue>()) IntNumValue(value, builder->typeBuilder.getI64Type(), location);
}

BoolValue* ASTBuildermake_bool_value(ASTBuilder* builder, bool value, uint64_t location) {
    return new (builder->allocate<BoolValue>()) BoolValue(value, builder->typeBuilder.getBoolType(), location);
}

CastedValue* ASTBuildermake_casted_value(ASTBuilder* builder, Value* value, BaseType* type, uint64_t location) {
    return new (builder->allocate<CastedValue>()) CastedValue(value, {type, location}, location);
}

IntNumValue* ASTBuildermake_char_value(ASTBuilder* builder, char value, uint64_t location) {
    return new (builder->allocate<IntNumValue>()) IntNumValue(value, builder->typeBuilder.getCharType(), location);
}

DereferenceValue* ASTBuildermake_dereference_value(ASTBuilder* builder, Value* value, BaseType* type, uint64_t location) {
    return new (builder->allocate<DereferenceValue>()) DereferenceValue(value, type, location);
}

DoubleValue* ASTBuildermake_double_value(ASTBuilder* builder, double value, uint64_t location) {
    return new (builder->allocate<DoubleValue>()) DoubleValue(value, builder->typeBuilder.getDoubleType(), location);
}

Expression* ASTBuildermake_expression_value(ASTBuilder* builder, Value* first, Value* second, Operation op, uint64_t location) {
    return new (builder->allocate<Expression>()) Expression(first, second, op, location);
}

FloatValue* ASTBuildermake_float_value(ASTBuilder* builder, float value, uint64_t location) {
    return new (builder->allocate<FloatValue>()) FloatValue(value, builder->typeBuilder.getFloatType(), location);
}

FunctionCall* ASTBuildermake_function_call_value(ASTBuilder* builder, Value* parent_val, uint64_t location) {
    const auto call = new (builder->allocate<FunctionCall>()) FunctionCall(parent_val, location);
    const auto func_type = parent_val->getType()->canonical()->as_function_type();
    if(func_type) {
        call->setType(func_type->returnType);
    } else {
        // TODO: user is probably calling a variant member
    }
    return call;
}

AccessChainNode* ASTBuildermake_function_call_node(ASTBuilder* builder, Value* parent_val, ASTNode* parent_node, uint64_t location) {
    //TODO: dedicated function call node
    // WHEN dedicated function call node is created, please note that get args on function call node exists
    // which assumes the parameter is a access chain node (fix in both cbi as well)
    const auto call = ASTBuildermake_function_call_value(builder, parent_val, location);
    const auto node = new (builder->allocate<AccessChainNode>()) AccessChainNode(location, parent_node);
    node->chain.values.emplace_back(call);
    node->chain.setType(call->getType());
    return node;
}

IndexOperator* ASTBuildermake_index_op_value(ASTBuilder* builder, Value* parent_val, uint64_t location) {
    return new (builder->allocate<IndexOperator>()) IndexOperator(parent_val, location);
}

IntNumValue* ASTBuildermake_int128_value(ASTBuilder* builder, uint64_t mag, bool is_neg, uint64_t location) {
    // TODO: is_neg ignored
    return new (builder->allocate<IntNumValue>()) IntNumValue(mag, builder->typeBuilder.getInt128Type(), location);
}

IntNumValue* ASTBuildermake_int_value(ASTBuilder* builder, int value, uint64_t location) {
    return new (builder->allocate<IntNumValue>()) IntNumValue(value, builder->typeBuilder.getIntType(), location);
}

IsValue* ASTBuildermake_is_value(ASTBuilder* builder, Value* value, BaseType* type, bool is_negating, uint64_t location) {
    return new (builder->allocate<IsValue>()) IsValue(value, {type, location}, is_negating, builder->typeBuilder.getBoolType(), location);
}

LambdaFunction* ASTBuildermake_lambda_function(ASTBuilder* builder, Value* value, BaseType* type, bool isVariadic, ASTNode* parent_node, uint64_t location) {
    return new (builder->allocate<LambdaFunction>()) LambdaFunction(isVariadic, parent_node, location);
}

CapturedVariable* ASTBuildermake_captured_variable(ASTBuilder* builder, chem::string_view* name, unsigned int index, bool capture_by_ref, bool mutable_ref, long value, uint64_t location) {
    // TODO passing nullptr as parent in captured variable
    return new (builder->allocate<CapturedVariable>()) CapturedVariable(*name, index, capture_by_ref, mutable_ref, nullptr, location);
}

IntNumValue* ASTBuildermake_long_value(ASTBuilder* builder, long value, uint64_t location) {
    return new (builder->allocate<IntNumValue>()) IntNumValue(value, builder->typeBuilder.getLongType(), location);
}

NegativeValue* ASTBuildermake_negative_value(ASTBuilder* builder, Value* value, uint64_t location) {
    return new (builder->allocate<NegativeValue>()) NegativeValue(value, location);
}

NotValue* ASTBuildermake_not_value(ASTBuilder* builder, Value* value, uint64_t location) {
    return new (builder->allocate<NotValue>()) NotValue(value, location);
}

NullValue* ASTBuildermake_null_value(ASTBuilder* builder, uint64_t location) {
    return new (builder->allocate<NullValue>()) NullValue(builder->typeBuilder.getNullPtrType(), location);
}

IntNumValue* ASTBuildermake_number_value(ASTBuilder* builder, uint64_t value, uint64_t location) {
    if(value < INT32_MAX) {
        return new (builder->allocate<IntNumValue>()) IntNumValue(value, builder->typeBuilder.getI32Type(), location);
    } else {
        return new (builder->allocate<IntNumValue>()) IntNumValue(value, builder->typeBuilder.getI64Type(), location);
    }
}

IntNumValue* ASTBuildermake_short_value(ASTBuilder* builder, short value, uint64_t location) {
    return new (builder->allocate<IntNumValue>()) IntNumValue(value, builder->typeBuilder.getShortType(), location);
}

SizeOfValue* ASTBuildermake_sizeof_value(ASTBuilder* builder, BaseType* type, uint64_t location) {
    return new (builder->allocate<SizeOfValue>()) SizeOfValue({type, location}, builder->typeBuilder.getU64Type(), location);
}

StringValue* ASTBuildermake_string_value(ASTBuilder* builder, chem::string_view* value, uint64_t location) {
    return new (builder->allocate<StringValue>()) StringValue(*value, builder->typeBuilder.getStringType(), location);
}

StructValue* ASTBuildermake_struct_value(ASTBuilder* builder, BaseType* ref, ASTNode* parent_node, uint64_t location) {
    // TODO do not take parent_node as parameter
    return new (builder->allocate<StructValue>()) StructValue(ref, location);
}

IntNumValue* ASTBuildermake_ubigint_value(ASTBuilder* builder, unsigned long long value, uint64_t location) {
    return new (builder->allocate<IntNumValue>()) IntNumValue(value, builder->typeBuilder.getU64Type(), location);
}

IntNumValue* ASTBuildermake_uchar_value(ASTBuilder* builder, unsigned char value, uint64_t location) {
    return new (builder->allocate<IntNumValue>()) IntNumValue(value, builder->typeBuilder.getUCharType(), location);
}

IntNumValue* ASTBuildermake_uint128_value(ASTBuilder* builder, uint64_t low, uint64_t high, uint64_t location) {
    // TODO: high ignored
    return new (builder->allocate<IntNumValue>()) IntNumValue(low, builder->typeBuilder.getUInt128Type(), location);
}

IntNumValue* ASTBuildermake_uint_value(ASTBuilder* builder, unsigned int value, uint64_t location) {
    return new (builder->allocate<IntNumValue>()) IntNumValue(value, builder->typeBuilder.getUIntType(), location);
}

IntNumValue* ASTBuildermake_ulong_value(ASTBuilder* builder, unsigned long value, uint64_t location) {
    return new (builder->allocate<IntNumValue>()) IntNumValue(value, builder->typeBuilder.getULongType(), location);
}

IntNumValue* ASTBuildermake_ushort_value(ASTBuilder* builder, unsigned short value, uint64_t location) {
    return new (builder->allocate<IntNumValue>()) IntNumValue(value, builder->typeBuilder.getUShortType(), location);
}

BlockValue* ASTBuildermake_block_value(ASTBuilder* builder, ASTNode* parent_node, uint64_t location) {
    return new (builder->allocate<BlockValue>()) BlockValue(parent_node, location);
}

ValueNode* ASTBuildermake_value_node(ASTBuilder* builder, Value* value, ASTNode* parent_node, uint64_t location) {
    return new (builder->allocate<ValueNode>()) ValueNode(value, parent_node, location);
}

VariableIdentifier* ASTBuildermake_identifier(ASTBuilder* builder, chem::string_view* value, ASTNode* linked, bool is_ns, uint64_t location) {
    const auto id = new (builder->allocate<VariableIdentifier>()) VariableIdentifier(*value, location, is_ns);
    id->linked = linked;
    id->setType(linked->known_type());
    return id;
}

VariantCase* ASTBuildermake_variant_case(ASTBuilder* builder, VariantMember* member, SwitchStatement* stmt, uint64_t location) {
    return new (builder->allocate<VariantCase>()) VariantCase(member, stmt, builder->typeBuilder.getVoidType(), location);
}

VariantCaseVariable* ASTBuildermake_variant_case_variable(ASTBuilder* builder, chem::string_view* name, VariantMemberParam* param, SwitchStatement* switch_stmt, uint64_t location) {
    return new (builder->allocate<VariantCaseVariable>()) VariantCaseVariable(*name, param, switch_stmt, location);
}

AssignStatement* ASTBuildermake_assignment_stmt(ASTBuilder* builder, Value* lhs, Value* rhs, Operation op, ASTNode* parent_node, uint64_t location) {
    return new (builder->allocate<AssignStatement>()) AssignStatement(lhs, rhs, op, parent_node, location);
}

BreakStatement* ASTBuildermake_break_stmt(ASTBuilder* builder, LoopASTNode* loop_node, ASTNode* parent_node, uint64_t location) {
    // TODO do not take loop_node
    return new (builder->allocate<BreakStatement>()) BreakStatement(parent_node, location);
}

ContinueStatement* ASTBuildermake_continue_stmt(ASTBuilder* builder, LoopASTNode* loop_node, ASTNode* parent_node, uint64_t location) {
    // TODO do not take loop_node
    return new (builder->allocate<ContinueStatement>()) ContinueStatement(parent_node, location);
}

DestructStmt* ASTBuildermake_destruct_stmt(ASTBuilder* builder, Value* array_value, Value* ptr_value, bool is_array, ASTNode* parent_node, uint64_t location) {
    return new (builder->allocate<DestructStmt>()) DestructStmt(array_value, ptr_value, is_array, false, parent_node, location);
}

ReturnStatement* ASTBuildermake_return_stmt(ASTBuilder* builder, Value* value, FunctionTypeBody* decl, ASTNode* parent_node, uint64_t location) {
    return new (builder->allocate<ReturnStatement>()) ReturnStatement(value, parent_node, location);
}

// TODO switch statement when multiple cases have been handled
//SwitchStatement* ASTBuildermake_return_stmt(ASTBuilder* builder, Value* value, FunctionType* decl, ASTNode* parent_node, uint64_t location) {
//    return new (builder->allocate<SwitchStatement>()) SwitchStatement(value, decl, parent_node, location);
//}

//ThrowStatement* ASTBuildermake_throw_stmt(ASTBuilder* builder, Value* value, FunctionType* decl, ASTNode* parent_node, uint64_t location) {
//    return new (builder->allocate<ThrowStatement>()) ThrowStatement(value, decl, parent_node, location);
//}

TypealiasStatement* ASTBuildermake_typealias_stmt(ASTBuilder* builder, chem::string_view* identifier, uint64_t id_loc, BaseType* actual_type, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (builder->allocate<TypealiasStatement>()) TypealiasStatement(LOC_ID(*identifier, id_loc), { actual_type, id_loc }, parent_node, location, specifier);
}

UsingStmt* ASTBuildermake_using_stmt(ASTBuilder* builder, AccessChain* chain, ASTNode* parent_node, bool is_namespace, uint64_t location) {
    return new (builder->allocate<UsingStmt>()) UsingStmt(chain, parent_node, is_namespace, location);
}

VarInitStatement* ASTBuildermake_varinit_stmt(ASTBuilder* builder, bool is_const, bool is_reference, chem::string_view* identifier, uint64_t id_loc, BaseType* type, Value* value, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (builder->allocate<VarInitStatement>()) VarInitStatement(is_const, is_reference, LOC_ID(*identifier, id_loc), {type, location}, value, parent_node, location, specifier);
}

// TODO scope needs a children method to get the nodes PtrVec
Scope* ASTBuildermake_scope(ASTBuilder* builder, ASTNode* parent_node, uint64_t location) {
    return new (builder->allocate<Scope>()) Scope(parent_node, location);
}

DoWhileLoop* ASTBuildermake_do_while_loop(ASTBuilder* builder, Value* condition, ASTNode* parent_node, uint64_t location) {
    return new (builder->allocate<DoWhileLoop>()) DoWhileLoop(condition, parent_node, location);
}

EnumDeclaration* ASTBuildermake_enum_decl(ASTBuilder* builder, chem::string_view* name, uint64_t name_loc, IntNType* underlying_type, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (builder->allocate<EnumDeclaration>()) EnumDeclaration(LOC_ID(*name, name_loc), {underlying_type, location}, parent_node, location, specifier);
}

EnumMember* ASTBuildermake_enum_member(ASTBuilder* builder, chem::string_view* name, unsigned int index, Value* init_value, EnumDeclaration* parent_node, uint64_t location) {
    return new (builder->allocate<EnumMember>()) EnumMember(name->data(), index, init_value, parent_node, location);
}

ForLoop* ASTBuildermake_for_loop(ASTBuilder* builder, VarInitStatement* initializer, Value* conditionExpr, ASTNode* incrementerExpr, ASTNode* parent_node, uint64_t location) {
    return new (builder->allocate<ForLoop>()) ForLoop(initializer, conditionExpr, incrementerExpr, parent_node, location);
}

FunctionDeclaration* ASTBuildermake_function(ASTBuilder* builder, chem::string_view* name, uint64_t name_location, BaseType* returnType, bool isVariadic, bool hasBody, ASTNode* parent_node, uint64_t location) {
    return new (builder->allocate<FunctionDeclaration>()) FunctionDeclaration(LOC_ID(*name, name_location), {returnType, location}, isVariadic, parent_node, location);
}

FunctionParam* ASTBuildermake_function_param(ASTBuilder* builder, chem::string_view* name, BaseType* type, unsigned int index, Value* value, bool implicit, FunctionType* decl, uint64_t location) {
    // TODO casting function type as parent node, this is wrong
    return new (builder->allocate<FunctionParam>()) FunctionParam(*name, {type, location}, index, value, implicit, (ASTNode*) decl, location);
}

GenericTypeParameter* ASTBuildermake_generic_param(ASTBuilder* builder, chem::string_view* name, BaseType* def_type, ASTNode* parent_node, unsigned int index, uint64_t location) {
    return new (builder->allocate<GenericTypeParameter>()) GenericTypeParameter(*name, {def_type, location}, parent_node, index, location);
}

IfStatement* ASTBuildermake_if_stmt(ASTBuilder* builder, Value* condition, ASTNode* parent_node, uint64_t location) {
    return new (builder->allocate<IfStatement>()) IfStatement(condition, parent_node, location);
}

ImplDefinition* ASTBuildermake_impl_def(ASTBuilder* builder, BaseType* interface_type, BaseType* struct_type, ASTNode* parent_node, uint64_t location) {
    return new (builder->allocate<ImplDefinition>()) ImplDefinition({interface_type, location}, {struct_type, location}, parent_node, location);
}

InterfaceDefinition* ASTBuildermake_interface_def(ASTBuilder* builder, chem::string_view* name, uint64_t name_location, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (builder->allocate<InterfaceDefinition>()) InterfaceDefinition(LOC_ID(*name, name_location), parent_node, location, specifier);
}

Namespace* ASTBuildermake_namespace(ASTBuilder* builder, chem::string_view* name, uint64_t name_location, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (builder->allocate<Namespace>()) Namespace(LOC_ID(*name, name_location), parent_node, location, specifier);
}

StructDefinition* ASTBuildermake_struct_def(ASTBuilder* builder, chem::string_view* name, uint64_t name_location, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (builder->allocate<StructDefinition>()) StructDefinition(LOC_ID(*name, name_location), parent_node, location, specifier);
}

StructMember* ASTBuildermake_struct_member(ASTBuilder* builder, chem::string_view* name, BaseType* type, Value* defValue, bool isConst, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (builder->allocate<StructMember>()) StructMember(*name, {type, location}, defValue, parent_node, location, isConst, specifier);
}

UnionDef* ASTBuildermake_union_def(ASTBuilder* builder, chem::string_view* name, uint64_t name_location, AccessSpecifier specifier, ASTNode* parent_node, uint64_t location) {
    return new (builder->allocate<UnionDef>()) UnionDef(LOC_ID(*name, name_location), parent_node, location, specifier);
}

UnsafeBlock* ASTBuildermake_unsafe_block(ASTBuilder* builder, ASTNode* node, uint64_t location) {
    return new (builder->allocate<UnsafeBlock>()) UnsafeBlock(node, location);
}

WhileLoop* ASTBuildermake_while_loop(ASTBuilder* builder, Value* condition, ASTNode* node, uint64_t location) {
    return new (builder->allocate<WhileLoop>()) WhileLoop(condition, node, location);
}

VariantDefinition* ASTBuildermake_variant_def(ASTBuilder* builder, chem::string_view* name, uint64_t name_location, AccessSpecifier specifier, ASTNode* node, uint64_t location) {
    return new (builder->allocate<VariantDefinition>()) VariantDefinition(LOC_ID(*name, name_location), node, location, specifier);
}

VariantMember* ASTBuildermake_variant_member(ASTBuilder* builder, chem::string_view* name, VariantDefinition* parent_node, uint64_t location) {
    return new (builder->allocate<VariantMember>()) VariantMember(*name, parent_node, location);
}

VariantMemberParam* ASTBuildermake_variant_member_param(ASTBuilder* builder, chem::string_view* name, unsigned int index, bool is_const, BaseType* type, Value* defValue, VariantMember* parent_node, uint64_t location) {
    return new (builder->allocate<VariantMemberParam>()) VariantMemberParam(*name, index, is_const, {type, location}, defValue, parent_node, location);
}

// ------------------------------AST Methods begin here-----------------------------------------------

// Value methods

int ASTAnygetAnyKind(ASTAny* any) {
    return static_cast<int>(any->any_kind());
}

int ValuegetKind(Value* value) {
    return static_cast<int>(value->val_kind());
}

BaseType* ValuegetType(Value* value) {
    return value->getType();
}

uint64_t ValuegetEncodedLocation(Value* value) {
    return value->encoded_location().encoded;
}

// ASTNode methods

uint64_t ASTNodegetEncodedLocation(ASTNode* node) {
    return node->encoded_location().encoded;
}

int ASTNodegetKind(ASTNode* node) {
    return static_cast<int>(node->kind());
}

ASTNode* ASTNodechild(ASTNode* node, chem::string_view* name) {
    return node->child(*name);
}

// BaseType methods

int BaseTypegetKind(BaseType* type) {
    return static_cast<int>(type->kind());
}

int IntNTypeget_intn_type_kind(IntNType* type) {
    return static_cast<int>(type->IntNKind());
}

ASTNode* LinkedTypegetLinkedNode(LinkedType* type) {
    return type->linked;
}

LinkedType* GenericTypegetLinkedType(GenericType* type) {
    return type->referenced;
}

BaseType* PointerTypegetChildType(PointerType* type) {
    return type->type;
}

BaseType* ReferenceTypegetChildType(ReferenceType* type) {
    return type->type;
}

BaseType* TypealiasStatementgetActualType(TypealiasStatement* type) {
    return type->actual_type;
}

// Other methods

std::vector<FunctionParam*>* FunctionTypeget_params(FunctionType* func_type) {
    return &func_type->params;
}

Value* AccessChainas_value(AccessChain* chain) {
    // c++ casting is required because Value is in the second place in inheritance
    // and the first ASTNode is virtual containing a pointer
    return chain;
}

std::vector<Value*>* AccessChainget_values(AccessChain* chain) {
    return &chain->values;
}

std::vector<Value*>* ArrayValueget_values(ArrayValue* value) {
    return &value->values;
}

std::vector<Value*>* ExpressiveStringgetValues(ExpressiveString* value) {
    return &value->values;
}

std::vector<Value*>* FunctionCallget_args(FunctionCall* value) {
    return &value->values;
}

std::vector<Value*>* FunctionCallNodeget_args(AccessChainNode* node) {
    return &node->chain.values.back()->as_func_call_unsafe()->values;
}

Value** IndexOperatorget_idx_ptr(IndexOperator* op) {
    return &op->idx;
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
    variantCase->identifier_list.emplace_back(variable);
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

void StructDefinitionadd_member(StructDefinition* definition, BaseDefMember* member) {
    definition->insert_variable_no_check(member);
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

void BlockValuesetCalculatedValue(BlockValue* bv, Value* value) {
    bv->setCalculatedValue(value);
}

void UnionDefinitionadd_member(UnionDef* definition, BaseDefMember* member) {
    definition->insert_variable(member);
}

void UnionDefinitionadd_function(UnionDef* definition, ASTAllocator* astAllocator, FunctionDeclaration* decl) {
    definition->insert_multi_func(*astAllocator, decl);
}

void VariantDefinitionadd_member(VariantDefinition* definition, BaseDefMember* member) {
    definition->insert_variable(member);
}

void VariantMemberadd_param(VariantMember* member, VariantMemberParam* param) {
    member->values[param->name] = param;
}

void* EmbeddedNodegetDataPtr(EmbeddedNode* node) {
    return node->data_ptr;
}

void* EmbeddedValuegetDataPtr(EmbeddedValue* value) {
    return value->data_ptr;
}

std::vector<BaseDefMember*>* VariantDefinitiongetMembers(VariantDefinition* def) {
    return (std::vector<BaseDefMember*>*) &def->variables();
}

BaseType* FunctionDeclarationgetReturnType(FunctionDeclaration* decl) {
    return decl->returnType;
}

void FunctionParamgetName(chem::string_view* view, FunctionParam* param) {
    *view = param->name;
}

BaseType* FunctionParamgetType(FunctionParam* param) {
    return param->type;
}

void BaseDefMembergetName(chem::string_view* view, BaseDefMember* member) {
    *view = member->name;
}

BaseType* BaseDefMembergetType(BaseDefMember* member) {
    if (member->kind() == ASTNodeKind::StructMember) {
        return static_cast<StructMember*>(member)->type;
    }
    // TODO handle other member types if needed
    return nullptr;
}

std::vector<BaseDefMember*>* StructDefinitiongetMembers(StructDefinition* def) {
    return (std::vector<BaseDefMember*>*) &def->variables();
}

std::vector<ASTNode*>* StructDefinitiongetFunctions(StructDefinition* def) {
    return (std::vector<ASTNode*>*) &def->functions();
}

std::vector<ASTNode*>* InterfaceDefinitiongetFunctions(InterfaceDefinition* def) {
    return (std::vector<ASTNode*>*) &def->functions();
}

std::vector<EnumMember*>* EnumDeclarationgetMembers(EnumDeclaration* decl) {
    auto vec = new std::vector<EnumMember*>();
    vec->reserve(decl->members.size());
    for(auto& pair : decl->members) {
        vec->push_back(pair.second);
    }
    return vec;
}

void FunctionDeclarationgetName(chem::string_view* view, FunctionDeclaration* decl) {
    *view = decl->name_view();
}

void StructDefinitiongetName(chem::string_view* view, StructDefinition* def) {
    *view = def->identifier.identifier;
}

void InterfaceDefinitiongetName(chem::string_view* view, InterfaceDefinition* def) {
    *view = def->identifier.identifier;
}

void NamespacegetName(chem::string_view* view, Namespace* ns) {
    *view = ns->identifier.identifier;
}

void EnumDeclarationgetName(chem::string_view* view, EnumDeclaration* decl) {
    *view = decl->name_view();
}

void EnumMembergetName(chem::string_view* view, EnumMember* member) {
    *view = member->name;
}

void VariantDefinitiongetName(chem::string_view* view, VariantDefinition* def) {
    *view = def->identifier.identifier;
}

void UnionDefinitiongetName(chem::string_view* view, UnionDef* def) {
    *view = def->identifier.identifier;
}

void FunctionDeclarationgetAttributes(FuncDeclAttributesCBI* out, FunctionDeclaration* decl) {
    auto& attrs = decl->attrs;
    out->specifier = static_cast<int32_t>(attrs.specifier);
    out->is_comptime = attrs.is_comptime;
    out->is_compiler_decl = attrs.is_compiler_decl;
    out->multi_func_index = attrs.multi_func_index;
    out->inline_strategy = static_cast<int32_t>(attrs.inline_strategy);
    out->is_extern = attrs.is_extern;
    out->is_cpp_mangle = attrs.is_cpp_mangle;
    out->deprecated = attrs.deprecated;
    out->is_implicit = attrs.is_implicit;
    out->is_noReturn = attrs.is_noReturn;
    out->is_constructor_fn = attrs.is_constructor_fn;
    out->is_copy_fn = attrs.is_copy_fn;
    out->is_delete_fn = attrs.is_delete_fn;
    out->is_unsafe = attrs.is_unsafe;
    out->is_override = attrs.is_override;
    out->has_usage = attrs.has_usage;
    out->no_mangle = attrs.no_mangle;
    out->is_generated = attrs.is_generated;
    out->std_call = attrs.std_call;
    out->dll_import = attrs.dll_import;
    out->dll_export = attrs.dll_export;
}

void InterfaceDefinitiongetAttributes(InterfaceDefinitionAttrsCBI* out, InterfaceDefinition* def) {
    auto& attrs = def->attrs;
    out->specifier = static_cast<int32_t>(attrs.specifier);
    out->has_implementation = attrs.has_implementation;
    out->deprecated = attrs.deprecated;
    out->is_static = attrs.is_static;
    out->is_no_mangle = attrs.is_no_mangle;
    out->is_extern = attrs.is_extern;
}

void TypealiasStatementgetAttributes(TypealiasDeclAttributesCBI* out, TypealiasStatement* stmt) {
    auto& attrs = stmt->attrs;
    out->specifier = static_cast<int32_t>(attrs.specifier);
    out->is_comptime = attrs.is_comptime;
    out->deprecated = attrs.deprecated;
    out->is_no_mangle = attrs.is_no_mangle;
    out->is_inlined = attrs.is_inlined;
}

int ASTNodegetAccessSpecifier(ASTNode* node) {
    return static_cast<int>(node->specifier());
}

void TypealiasStatementgetName(chem::string_view* view, TypealiasStatement* stmt) {
    *view = stmt->name_view();
}

std::size_t GenericTypegetArgumentCount(GenericType* type) {
    return type->types.size();
}

BaseType* GenericTypegetArgumentType(GenericType* type, std::size_t index) {
    return type->types[index];
}

uint64_t GenericTypegetArgumentLocation(GenericType* type, std::size_t index) {
    return type->types[index].encoded_location().encoded;
}