
using namespace std;

// The Base Structs

public enum IntNTypeKind {

    Char,
    Short,
    Int,
    Long,
    LongLong,
    Int128,

    I8,
    I16,
    I32,
    I64,

    UChar,
    UShort,
    UInt,
    ULong,
    ULongLong,
    UInt128,

    U8,
    U16,
    U32,
    U64,

};

public struct IntNType : BaseType {

    func get_intn_type_kind(&self) : IntNTypeKind

}

// The Types

public struct AnyType : BaseType {}

public struct ArrayType : BaseType {}

public struct I8Type : IntNType {}
public struct I16Type : IntNType {}
public struct I32Type : IntNType {}
public struct I64Type : IntNType {}

public struct U8Type : IntNType {}
public struct U16Type : IntNType {}
public struct U32Type : IntNType {}
public struct U64Type : IntNType {}

public struct BoolType : BaseType {}

public struct DoubleType : BaseType {}

public struct DynamicType : BaseType {}

public struct FloatType : BaseType {}

public struct FunctionType : BaseType {

    func get_params(&self) : *mut VecRef<FunctionParam>;

}

public struct GenericType : BaseType {

    func getLinkedType(&self) : *mut LinkedType;

}

public struct Int128Type : IntNType {}

public struct IntType : IntNType {}

public struct CharType : IntNType {}

public struct LinkedType : BaseType {

    func getLinkedNode(&self) : *mut ASTNode

}

public struct LinkedValueType : BaseType {}

public struct LiteralType : BaseType {}

public struct LongType : IntNType {}

public struct LongLongType : IntNType {}

public struct PointerType : BaseType {}

public struct ReferenceType : BaseType {

    func getChildType(&self) : *mut BaseType

}

public struct ShortType : IntNType {}

public struct StringType : BaseType {}

public struct UBigIntType : IntNType {}

public struct UCharType : IntNType {}

public struct UInt128Type : IntNType {}

public struct UIntType : IntNType {}

public struct ULongType : IntNType {}

public struct ULongLongType : IntNType {}

public struct UShortType : IntNType {}

public struct VoidType : BaseType {}

// The Values

public struct AccessChain : Value {

    func get_values(&self) : *mut VecRef<Value>;

}

public struct ValueWrapperNode : ASTNode {}

public struct AccessChainNode : ASTNode {}

public struct FunctionCallNode : ASTNode {

    func get_args(&self) : *mut VecRef<Value>;

}

public struct AddrOfValue : Value {}

public struct ArrayValue : Value {

    func get_values(&self) : *mut VecRef<Value>;

}

public struct BigIntValue : Value {}

public struct BoolValue : Value {}

public struct CastedValue : Value {}

public struct CharValue : Value {}

public struct DereferenceValue : Value {}

public struct DoubleValue : Value {}

public struct Expression : Value {}

public struct FloatValue : Value {}

public struct ExpressiveString : Value {

    func getValues(&self) : *mut VecRef<Value>;

}

public struct FunctionCall : ChainValue {

    func get_args(&self) : *mut VecRef<Value>;

}

public struct IndexOperator : ChainValue {

    func get_idx_ptr(&self) : *mut *mut Value

}

public func (idx : &mut IndexOperator) get_idx() : *mut Value {
    return *idx.get_idx_ptr()
}

public struct Int128Value : Value {}

public struct IntValue : Value {}

public struct IsValue : Value {}

public struct LambdaFunction : Value {

    func get_params(&self) : *mut VecRef<FunctionParam>;

    func get_capture_list(&self) : *mut VecRef<CapturedVariable>;

    func get_body(&self) : *mut VecRef<ASTNode>;

}

public struct LongValue : Value {}

public struct NegativeValue : Value {}

public struct NotValue : Value {}

public struct NullValue : Value {}

public struct NumberValue : Value {}

public struct ShortValue : Value {}

public struct SizeOfValue : Value {}

public struct StringValue : Value {}

public struct StructValue : Value {

    func add_value(&self, name : &string_view, value : *mut Value)

}

public struct BlockValue : Value {

    func get_body(&self) : *mut VecRef<ASTNode>;

    func setCalculatedValue(&self, value : *mut Value)

}

public struct UBigIntValue : Value {}

public struct UCharValue : Value {}

public struct UInt128Value : Value {}

public struct UIntValue : Value {}

public struct ULongValue : Value {}

public struct UShortValue : Value {}

public struct ValueNode : ASTNode {}

public struct VariableIdentifier : ChainValue {}

public struct EmbeddedValue : Value {

    func getDataPtr(&self) : *void

}

// The ASTNodes

public struct StructMemberInitializer : ASTNode {}

public struct CapturedVariable : ASTNode {}

public struct VariantCase : ASTNode {

    func add_variable(&self, variable : *VariantCaseVariable);

}

public struct VariantCaseVariable : ASTNode {}

public struct AssignStatement : ASTNode {}

public struct BreakStatement : ASTNode {}

public struct ContinueStatement : ASTNode {}

public struct DestructStmt : ASTNode {}

public struct ReturnStatement : ASTNode {}

public struct TypealiasStatement : ASTNode {

    func getActualType(&self) : *mut BaseType

}

public struct UsingStmt : ASTNode {}

public struct VarInitStatement : ASTNode {}

public struct Scope : ASTNode {

    func getNodes(&self) : *mut VecRef<ASTNode>;

}

public struct LoopASTNode : ASTNode {

}

public struct DoWhileLoop : LoopASTNode {

    func get_body(&self) : *mut VecRef<ASTNode>;

}

public struct EnumDeclaration : ASTNode {

    func add_member(&self, member : *EnumMember)

}

public struct EnumMember : ASTNode {}

public struct ForLoop : LoopASTNode {

    func get_body(&self) : *mut VecRef<ASTNode>;

}

public struct SwitchStatement {

}

public struct FunctionDeclaration : ASTNode {

    func get_params(&self) : *mut VecRef<FunctionParam>;

    func add_body(&self) : *mut VecRef<ASTNode>

}

public struct FunctionParam : ASTNode {}

public struct GenericTypeParameter : ASTNode {}

public struct IfStatement : ASTNode {

    func get_body(&self) : *mut VecRef<ASTNode>

    func add_else_body(&self) : *mut VecRef<ASTNode>

    func add_else_if(&self, condition : *Value) : *mut VecRef<ASTNode>

}

public struct ImplDefinition : ASTNode {

    func add_function(builder : *ASTBuilder, decl : *FunctionDeclaration)

}

public struct InitBlock : ASTNode {

    func add_initializer(&self, name : &string_view, value : *Value);

}

public struct InterfaceDefinition : ASTNode {

    func add_function(builder : *ASTBuilder, decl : *FunctionDeclaration)

}

public struct Namespace : ASTNode {

     func get_body(&self) : *mut VecRef<ASTNode>;

}

public struct StructDefinition : ASTNode {

    func add_member(name : &string_view, member : *StructMember)

    func add_function(builder : *ASTBuilder, decl : *FunctionDeclaration)

}

public struct StructMember : ASTNode {}

public struct UnionDef : ASTNode {

    func add_member(name : &string_view, member : *StructMember)

    func add_function(builder : *ASTBuilder, decl : *FunctionDeclaration)

}

public struct UnsafeBlock : ASTNode {

    func get_body(&self) : *mut VecRef<ASTNode>;

}

public struct WhileLoop : LoopASTNode {

    func get_body(&self) : *mut VecRef<ASTNode>;

}

public struct VariantDefinition : ASTNode {

    func add_member(&self, name : &string_view, member : *StructMember)

}

public struct VariantMember : ASTNode {

    func add_param(&self, param : *VariantMemberParam);

}

public struct EmbeddedNode : ASTNode {

    func getDataPtr(&self) : *void

}

public struct VariantMemberParam : ASTNode {}

public type EmbeddedNodeSymbolResolveFunc = (resolver : *mut SymbolResolver, value : *mut EmbeddedNode) => bool;

public type EmbeddedNodeReplacementFunc = (builder : *ASTBuilder, value : *mut EmbeddedNode) => *ASTNode

public type EmbeddedNodeKnownTypeFunc = (value : *EmbeddedNode) => *BaseType

public type EmbeddedNodeChildResolutionFunc = (value : *EmbeddedNode, name : &std::string_view) => *ASTNode

public type EmbeddedNodeTraversalFunc = (node : *EmbeddedNode, data : *void, traverse : (data : *void, item : *mut ASTAny) => bool) => void

public type EmbeddedValueSymbolResolveFunc = (resolver : *SymbolResolver, value : *EmbeddedValue) => bool

public type EmbeddedValueReplacementFunc = (builder : *ASTBuilder, value : *EmbeddedValue) => *Value

public type EmbeddedValueTraversalFunc = (value : *EmbeddedValue, data : *void, traverse : (data : *void, item : *mut ASTAny) => bool) => void

@compiler.interface
public struct ASTBuilder {

    var allocator : *mut BatchAllocator

    func allocate_with_cleanup(&self, obj_size : size_t, alignment : size_t, cleanup_fn : (obj : *void) => void) : *mut void;

    func store_cleanup(&self, obj : *void, cleanup_fn : (obj : *void) => void);

    func createType(&self, value : *mut Value) : *mut BaseType

    func make_embedded_node(&self,
        name : &std::string_view,
        data_ptr : *void,
        known_type_fn : EmbeddedNodeKnownTypeFunc,
        child_res_fn : EmbeddedNodeChildResolutionFunc,
        chemical_nodes : std::span<*mut ASTNode>,
        chemical_values : std::span<*mut Value>,
        parent_node : *ASTNode,
        location : ubigint
    ) : *mut EmbeddedNode

    func make_embedded_value(&self,
        name : &std::string_view,
        data_ptr : *void,
        type : *mut BaseType,
        chemical_nodes : std::span<*mut ASTNode>,
        chemical_values : std::span<*mut Value>,
        location : ubigint
    ) : *mut EmbeddedValue

    func make_any_type(&self, location : ubigint) : *mut AnyType

    func make_array_type(&self, elem_type : *BaseType, array_size : int, location : ubigint) : *mut ArrayType

    // chemical integer types

    func get_i8_type(&self) : *mut I8Type;

    func get_i16_type(&self) : *mut I16Type;

    func get_i32_type(&self) : *mut I32Type;

    func get_i64_type(&self) : *mut I64Type;

    func get_i128_type(&self) : *mut I64Type;

    // chemical integer types (unsigned)

    func get_u8_type(&self) : *mut U8Type;

    func get_u16_type(&self) : *mut U16Type;

    func get_u32_type(&self) : *mut U32Type;

    func get_u64_type(&self) : *mut U64Type;

    func get_u128_type(&self) : *mut U64Type;

    // c like integer types

    func get_char_type(&self) : *mut CharType

    func get_short_type(&self) : *mut ShortType

    func get_int_type(&self) : *mut IntType

    func get_long_type(&self) : *mut LongType

    func get_longlong_type(&self) : *mut LongLongType

    // c like integer types (unsigned)

    func get_uchar_type(&self) : *mut UCharType;

    func get_ushort_type(&self) : *mut UShortType;

    func get_uint_type(&self) : *mut UIntType;

    func get_ulong_type(&self) : *mut ULongType;

    func get_ulonglong_type(&self) : *mut ULongLongType;

    // other types

    func make_bool_type(&self, location : ubigint) : *mut BoolType

    func make_double_type(&self, location : ubigint) : *mut DoubleType

    func make_dynamic_type(&self, child_type : *BaseType, location : ubigint) : *mut DynamicType

    func make_float_type(&self, location : ubigint) : *mut FloatType

    func make_func_type(&self, returnType : *BaseType, isVariadic : bool, isCapturing : bool, parent : *ASTNode, location : ubigint) : *mut FunctionType

    func make_generic_type(&self, linkedType : *LinkedType) : *mut GenericType

    func make_linked_type(&self, type : &string_view, linked : *ASTNode, location : ubigint) : *mut LinkedType

    func make_linked_value_type(&self, value : *Value, location : ubigint) : *mut LinkedValueType

    func make_literal_type(&self, child_type : *BaseType, location : ubigint) : *mut LiteralType

    func make_long_type(&self, location : ubigint) : *mut LongType

    func make_ptr_type(&self, child_type : *BaseType, location : ubigint) : *mut PointerType

    func make_reference_type(&self, child_type : *BaseType, location : ubigint) : *mut ReferenceType

    func make_string_type(&self, location : ubigint) : *mut StringType

    func make_void_type(&self, location : ubigint) : *mut VoidType

    func make_access_chain(&self, values : &std::span<*mut ChainValue>, location : ubigint) : *mut AccessChain

    func make_access_chain_node(&self, values : &std::span<*mut ChainValue>, parent_node : *mut ASTNode, location : ubigint) : *mut AccessChainNode

    func make_value_wrapper(&self, value : *Value, parent_node : *ASTNode) : *mut ValueWrapperNode

    func make_addr_of_value(&self, value : *Value, location : ubigint) : *mut AddrOfValue

    func make_array_value(&self, type : *BaseType, location : ubigint) : *mut ArrayValue

    func make_bigint_value(&self, value : bigint, location : ubigint) : *mut BigIntValue

    func make_bool_value(&self, value : bool, location : ubigint) : *mut BoolValue

    func make_casted_value(&self, value : *Value, type : *BaseType, location : ubigint) : *mut CastedValue

    func make_char_value(&self, value : char, location : ubigint) : *mut CharValue

    func make_dereference_value(&self, value : *Value, type : *mut BaseType, location : ubigint) : *mut DereferenceValue

    func make_double_value(&self, value : double, location : ubigint) : *mut DoubleValue

    func make_expression_value(&self, first : *Value, second : *Value, op : Operation, location : ubigint) : *mut Expression

    func make_float_value(&self, value : float, location : ubigint) : *mut FloatValue

    func make_function_call_value(&self, parent_val : *Value, location : ubigint) : *mut FunctionCall

    func make_function_call_node(&self, parent_val : *Value, parent_node : *mut ASTNode, location : ubigint) : *mut FunctionCallNode

    func make_index_op_value(&self, parent_val : *Value, location : ubigint) : *mut IndexOperator

    func make_int128_value(&self, mag : ubigint, is_neg : bool, location : ubigint) : *mut Int128Value

    func make_int_value(&self, value : int, location : ubigint) : *mut IntValue

    func make_is_value(&self, value : *Value, type : *BaseType, is_negating : bool, location : ubigint) : *mut IsValue

    func make_lambda_function(&self, value : *Value, type : *BaseType, isVariadic : bool, parent_node : *ASTNode, location : ubigint) : *mut LambdaFunction

    func make_captured_variable(&self, name : &string_view, index : uint, capture_by_ref : bool, mutable_ref : bool, value : long, location : ubigint) : *mut CapturedVariable

    func make_long_value(&self, value : long, location : ubigint) : *mut LongValue

    func make_negative_value(&self, value : *Value, location : ubigint) : *mut NegativeValue

    func make_not_value(&self, value : *Value, location : ubigint) : *mut NotValue

    func make_null_value(&self, location : ubigint) : *mut NullValue

    func make_number_value(&self, value : ubigint, location : ubigint) : *mut NumberValue

    func make_short_value(&self, value : short, location : ubigint) : *mut ShortValue

    func make_sizeof_value(&self, type : *BaseType, location : ubigint) : *mut SizeOfValue

    func make_string_value(&self, value : &string_view, location : ubigint) : *mut StringValue

    func make_struct_value(&self, ref : *BaseType, parent_node : *ASTNode, location : ubigint) : *mut StructValue

    func make_ubigint_value(&self, value : ubigint, location : ubigint) : *mut UBigIntValue

    func make_uchar_value(&self, value : uchar, location : ubigint) : *mut UCharValue

    func make_uint128_value(&self, low : ubigint, high : ubigint, location : ubigint) : *mut UInt128Value

    func make_uint_value(&self, value : uint, location : ubigint) : *mut UIntValue

    func make_ulong_value(&self, value : ulong, location : ubigint) : *mut ULongValue

    func make_ushort_value(&self, value : ushort, location : ubigint) : *mut UShortValue

    func make_block_value(&self, parent_node : *ASTNode, location : ubigint) : *mut BlockValue

    func make_value_node(&self, value : *Value, parent_node : *ASTNode, location : ubigint) : *mut ValueNode

    func make_identifier(&self, value : &string_view, linked : *mut ASTNode, is_ns : bool, location : ubigint) : *mut VariableIdentifier

    func make_variant_case(&self, mem : *VariantMember, stmt : *SwitchStatement, location : ubigint) : *mut VariantCase

    func make_variant_case_variable(&self, name : &string_view, param : *VariantMemberParam, stmt : *SwitchStatement, location : ubigint) : *mut VariantCaseVariable

    func make_assignment_stmt(&self, lhs : *Value, rhs : *Value, op : Operation, parent_node : *ASTNode, location : ubigint) : *mut AssignStatement

    func make_break_stmt(&self, loop_node : *LoopASTNode, parent_node : *ASTNode, location : ubigint) : *mut BreakStatement

    func make_continue_stmt(&self, loop_node : *LoopASTNode, parent_node : *ASTNode, location : ubigint) : *mut ContinueStatement

    func make_destruct_stmt(&self, array_value : *Value, ptr_value : *Value, is_array : bool, parent_node : *ASTNode, location : ubigint) : *mut DestructStmt

    func make_return_stmt(&self, value : *Value, decl : *FunctionType, parent_node : *ASTNode, location : ubigint) : *mut ReturnStatement

    //SwitchStatement* ASTBuildermake_return_stmt(CSTConverter* converter, Value* value, FunctionType* decl, ASTNode* parent_node, location : ubigint);

    //ThrowStatement* ASTBuildermake_throw_stmt(CSTConverter* converter, Value* value, FunctionType* decl, ASTNode* parent_node, location : ubigint);

    func make_typealias_stmt(&self, identifier : &string_view, id_loc : ubigint, actual_type : *BaseType, specifier : AccessSpecifier, parent_node : *ASTNode, location : ubigint) : *mut TypealiasStatement

    func make_using_stmt(&self, chain : *AccessChain, parent_node : *ASTNode, is_namespace : bool, location : ubigint) : *mut UsingStmt

    func make_varinit_stmt(&self, is_const : bool, is_reference : bool, identifier : &string_view, id_loc : ubigint, type : *BaseType, value : *Value, specifier : AccessSpecifier, parent_node : *ASTNode, location : ubigint) : *mut VarInitStatement

    func make_scope(&self, parent_node : *ASTNode, location : ubigint) : *mut Scope

    func make_do_while_loop(&self, condition : *Value, parent_node : *ASTNode, location : ubigint) : *mut DoWhileLoop

    func make_enum_decl(&self, name : &string_view, name_loc : ubigint, underlying_type : *mut IntNType, specifier : AccessSpecifier, parent_node : *ASTNode, location : ubigint) : *mut EnumDeclaration

    func make_enum_member(&self, name : &string_view, index : uint, init_value : *Value, parent_node : *EnumDeclaration, location : ubigint) : *mut EnumMember

    func make_for_loop(&self, initializer : *VarInitStatement, conditionExpr : *Value, incrementerExpr : *ASTNode, parent_node : *ASTNode, location : ubigint) : *mut ForLoop

    func make_function(&self, name : &string_view, name_location : ubigint, returnType : *BaseType, isVariadic : bool, hasBody : bool, parent_node : *ASTNode, location : ubigint) : *mut FunctionDeclaration

    func make_function_param(&self, name : &string_view, type : *BaseType, index : uint, value : *Value, implicit : bool, decl : *FunctionType, location : ubigint) : *mut FunctionParam

    func make_generic_param(&self, name : &string_view, def_type : *BaseType, parent_node : *ASTNode, index : uint, location : ubigint) : *mut GenericTypeParameter

    func make_if_stmt(&self, condition : *Value, parent_node : *ASTNode, location : ubigint) : *mut IfStatement

    func make_impl_def(&self, interface_type : *BaseType, struct_type : *BaseType, parent_node : *ASTNode, location : ubigint) : *mut ImplDefinition

    func make_init_block(&self, parent_node : *ASTNode, location : ubigint) : *mut InitBlock

    func make_interface_def(&self, name : &string_view, name_loc : ubigint, specifier : AccessSpecifier, parent_node : *ASTNode, location : ubigint) : *mut InterfaceDefinition

    func make_namespace(&self, name : &string_view, name_loc : ubigint, specifier : AccessSpecifier, parent_node : *ASTNode, location : ubigint) : *mut Namespace

    func make_struct_def(&self, name : &string_view, name_loc : ubigint, specifier : AccessSpecifier, parent_node : *ASTNode, location : ubigint) : *mut StructDefinition

    func make_struct_member(&self, name : &string_view, type : *BaseType, defValue : *Value, isConst : bool, specifier : AccessSpecifier, parent_node : *ASTNode, location : ubigint) : *mut StructMember

    func make_union_def(&self, name : &string_view, name_loc : ubigint, specifier : AccessSpecifier, parent_node : *ASTNode, location : ubigint) : *mut UnionDef

    func make_unsafe_block(&self, node : *ASTNode, location : ubigint) : *mut UnsafeBlock

    func make_while_loop(&self, condition : *Value, node : *ASTNode, location : ubigint) : *mut WhileLoop

    func make_variant_def(&self, name : &string_view, name_loc : ubigint, specifier : AccessSpecifier, node : *ASTNode, location : ubigint) : *mut VariantDefinition

    func make_variant_member(&self, name : &string_view, parent_node : *VariantDefinition, location : ubigint) : *mut VariantMember

    func make_variant_member_param(&self, name : &string_view, index : uint, is_const : bool, type : *BaseType, defValue : *Value, parent_node : *VariantMember, location : ubigint) : *mut VariantMemberParam

}

public func (builder : &mut ASTBuilder) allocate_size(obj_size : size_t, alignment : size_t) : *mut char {
    return builder.allocator.allocate_size(obj_size, alignment);
}

// adds +1 to size to accomodate for the last null terminator \0 in string
// allocates string with size
public func (builder : &mut ASTBuilder) allocate_str_size(size : size_t) : *mut char {
    return builder.allocator.allocate_str_size(size);
}

public func (builder : &mut ASTBuilder) allocate_str(data : *char, size : size_t) : *mut char {
    return builder.allocator.allocate_str(data, size);
}

public func (builder : &mut ASTBuilder) allocate_view(view : &std::string_view) : std::string_view {
    return builder.allocator.allocate_view(view);
}

public comptime func <T> (builder : &mut ASTBuilder) allocate() : *mut T {
    // TODO get destructor function
    var delete_fn = intrinsics::get_child_fn<T>("delete") as (obj : *void) => void;
    if(delete_fn != null) {
        return intrinsics::wrap(builder.allocate_with_cleanup(sizeof(T), alignof(T), delete_fn)) as *mut T
    } else {
        return intrinsics::wrap(builder.allocate_size(sizeof(T), alignof(T))) as *mut T
    }
}