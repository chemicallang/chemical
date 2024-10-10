import "AccessSpecifier.ch"
import "./CSTToken.ch"

// The Base Structs

struct ASTNode {}

struct BaseType {}

struct Value {}

// The Types

struct AnyType : BaseType {}

struct ArrayType : BaseType {}

struct BigIntType : BaseType {}

struct BoolType : BaseType {}

struct DoubleType : BaseType {}

struct DynamicType : BaseType {}

struct FloatType : BaseType {}

struct FunctionType : BaseType {

    func get_params(&self) : *VecRef<FunctionParam>;

}

struct GenericType : BaseType {

    func get_types(&self) : *VecRef<BaseType>;

}

struct Int128Type : BaseType {}

struct IntType : BaseType {}

struct LinkedType : BaseType {}

struct LinkedValueType : BaseType {}

struct LiteralType : BaseType {}

struct LongType : BaseType {}

struct PointerType : BaseType {}

struct ReferenceType : BaseType {}

struct ShortType : BaseType {}

struct StringType : BaseType {}

struct UBigIntType : BaseType {}

struct UCharType : BaseType {}

struct UInt128Type : BaseType {}

struct UIntType : BaseType {}

struct ULongType : BaseType {}

struct UShortType : BaseType {}

struct VoidType : BaseType {}

// The Values

struct AccessChain : ASTNode, Value {

    func get_values(&self) : *VecRef<Value>;

}

struct AddrOfValue : Value {}

struct ArrayValue : Value {

    func get_values(&self) : *VecRef<Value>;

    func add_size(&self, size : uint);

}

struct BigIntValue : Value {}

struct BoolValue : Value {}

struct CastedValue : Value {}

struct CharValue : Value {}

struct DereferenceValue : Value {}

struct DoubleValue : Value {}

struct Expression : Value {}

struct FloatValue : Value {}

struct FunctionCall : Value {

    func get_args(&self) : *VecRef<Value>;

}

struct IndexOperator : Value {

    func get_values(&self) : *VecRef<Value>;

}

struct Int128Value : Value {}

struct IntValue : Value {}

struct IsValue : Value {}

struct LambdaFunction : Value {

    func get_params(&self) : *VecRef<FunctionParam>;

    func get_capture_list(&self) : *VecRef<CapturedVariable>;

    func get_body(&self) : *VecRef<ASTNode>;

}

struct LongValue : Value {}

struct NegativeValue : Value {}

struct NotValue : Value {}

struct NullValue : Value {}

struct NumberValue : Value {}

struct ShortValue : Value {}

struct SizeOfValue : Value {}

struct StringValue : Value {}

struct StructValue : Value {

    func add_value(&self, name : *string, initializer : *StructMemberInitializer)

}

struct UBigIntValue : Value {}

struct UCharValue : Value {}

struct UInt128Value : Value {}

struct UIntValue : Value {}

struct ULongValue : Value {}

struct UShortValue : Value {}

struct ValueNode : ASTNode {}

struct VariableIdentifier : Value {}

// The ASTNodes

struct StructMemberInitializer : ASTNode {}

struct CapturedVariable : ASTNode {}

struct VariantCall : ASTNode {}

struct VariantCase : ASTNode {

    func add_variable(&self, variable : *VariantCaseVariable);

}

struct VariantCaseVariable : ASTNode {}

struct AssignStatement : ASTNode {}

struct BreakStatement : ASTNode {}

struct Comment : ASTNode {}

struct ContinueStatement : ASTNode {}

struct DestructStmt : ASTNode {}

struct ReturnStatement : ASTNode {}

struct TypealiasStatement : ASTNode {}

struct UsingStmt : ASTNode {}

struct VarInitStatement : ASTNode {}

struct Scope : ASTNode {}

struct LoopASTNode : ASTNode {

}

struct DoWhileLoop : LoopASTNode {

    func get_body(&self) : *VecRef<ASTNode>;

}

struct EnumDeclaration : ASTNode {

    func add_member(&self, member : *EnumMember)

}

struct EnumMember : ASTNode {}

struct ForLoop : LoopASTNode {

    func get_body(&self) : *VecRef<ASTNode>;

}

struct SwitchStatement {

}

struct FunctionDeclaration : ASTNode {

    func get_params(&self) : *VecRef<FunctionParam>;

    func get_generic_params(&self) : *VecRef<GenericTypeParameter>;

    func add_body(&self) : *VecRef<ASTNode>

}

struct FunctionParam : ASTNode {}

struct GenericTypeParameter : ASTNode {}

struct IfStatement : ASTNode {

    func get_body(&self) : *VecRef<ASTNode>

    func add_else_body(&self) : *VecRef<ASTNode>

    func add_else_if(&self, condition : *Value) : *VecRef<ASTNode>

}

struct ImplDefinition : ASTNode {

    func add_function(decl : *FunctionDeclaration)

}

struct InitBlock : ASTNode {

    func add_initializer(&self, name : *string, is_inherited_type : bool, value : *Value);

}

struct InterfaceDefinition : ASTNode {

    func add_function(decl : *FunctionDeclaration)

}

struct Namespace : ASTNode {

     func get_body(&self) : *VecRef<ASTNode>;

}

struct StructDefinition : ASTNode {

    func add_member(name : *string, member : *StructMember)

    func add_function(decl : *FunctionDeclaration)

    func get_generic_params(&self) : *VecRef<GenericTypeParameter>;

}

struct StructMember : ASTNode {}

struct UnionDef : ASTNode {

    func add_member(name : *string, member : *StructMember)

    func add_function(decl : *FunctionDeclaration)

    func get_generic_params(&self) : *VecRef<GenericTypeParameter>;

}

struct UnsafeBlock : ASTNode {

    func get_body(&self) : *VecRef<ASTNode>;

}

struct WhileLoop : LoopASTNode {

    func get_body(&self) : *VecRef<ASTNode>;

}

struct VariantDefinition : ASTNode {

    func add_member(&self, name : *string, member : *StructMember)

}

struct VariantMember : ASTNode {

    func add_param(&self, param : *VariantMemberParam);

}

struct VariantMemberParam : ASTNode {}

@compiler:interface
public struct ASTBuilder {

    func make_any_type(&self, token : *CSTToken) : *AnyType

    func make_array_type(&self, elem_type : *BaseType, array_size : int, token : *CSTToken) : *ArrayType

    func make_bigint_type(&self, token : *CSTToken) : *BigIntType

    func make_bool_type(&self, token : *CSTToken) : *BoolType

    func make_char_type(&self, token : *CSTToken) : *BoolType

    func make_double_type(&self, token : *CSTToken) : *DoubleType

    func make_dynamic_type(&self, child_type : *BaseType, token : *CSTToken) : *DynamicType

    func make_float_type(&self, token : *CSTToken) : *FloatType

    func make_func_type(&self, returnType : *BaseType, isVariadic : bool, isCapturing : bool, parent : *ASTNode, token : *CSTToken) : *FunctionType

    func make_generic_type(&self, linkedType : *LinkedType) : *GenericType

    func make_int128_type(&self, token : *CSTToken) : *Int128Type

    func make_int_type(&self, token : *CSTToken) : *IntType

    func make_linked_type(&self, type : *string, linked : *ASTNode, token : *CSTToken) : *LinkedType

    func make_linked_value_type(&self, value : *Value, token : *CSTToken) : *LinkedValueType

    func make_literal_type(&self, child_type : *BaseType, token : *CSTToken) : *LiteralType

    func make_long_type(&self, token : *CSTToken) : *LongType

    func make_ptr_type(&self, child_type : *BaseType, token : *CSTToken) : *PointerType

    func make_reference_type(&self, child_type : *BaseType, token : *CSTToken) : *ReferenceType

    func make_short_type(&self, token : *CSTToken) : *ShortType

    func make_string_type(&self, token : *CSTToken) : *StringType

    func make_ubigint_type(&self, token : *CSTToken) : *UBigIntType

    func make_uchar_type(&self, token : *CSTToken) : *UCharType

    func make_uint128_type(&self, token : *CSTToken) : *UInt128Type

    func make_uint_type(&self, token : *CSTToken) : *UIntType

    func make_ulong_type(&self, token : *CSTToken) : *ULongType

    func make_ushort_type(&self, token : *CSTToken) : *UShortType

    func make_void_type(&self, token : *CSTToken) : *VoidType

    func make_access_chain(&self, parent_node : *ASTNode, is_node : bool, token : *CSTToken) : *AccessChain

    func make_addr_of_value(&self, value : *Value, token : *CSTToken) : *AddrOfValue

    func make_array_value(&self, type : *BaseType, token : *CSTToken) : *ArrayValue

    func make_bigint_value(&self, value : bigint, token : *CSTToken) : *BigIntValue

    func make_bool_value(&self, value : bool, token : *CSTToken) : *BoolValue

    func make_casted_value(&self, value : *Value, type : *BaseType, token : *CSTToken) : *CastedValue

    func make_char_value(&self, value : char, token : *CSTToken) : *CharValue

    func make_dereference_value(&self, value : *Value, token : *CSTToken) : *DereferenceValue

    func make_double_value(&self, value : double, token : *CSTToken) : *DoubleValue

    func make_expression_value(&self, first : *Value, second : *Value, op : Operation, token : *CSTToken) : *Expression

    func make_float_value(&self, value : float, token : *CSTToken) : *FloatValue

    func make_function_call_value(&self, token : *CSTToken) : *FunctionCall

    func make_index_op_value(&self, token : *CSTToken) : *IndexOperator

    func make_int128_value(&self, mag : ubigint, is_neg : bool, token : *CSTToken) : *Int128Value

    func make_int_value(&self, value : int, token : *CSTToken) : *IntValue

    func make_is_value(&self, value : *Value, type : *BaseType, is_negating : bool, token : *CSTToken) : *IsValue

    func make_lambda_function(&self, value : *Value, type : *BaseType, isVariadic : bool, parent_node : *ASTNode, token : *CSTToken) : *LambdaFunction

    func make_captured_variable(&self, name : *string, index : uint, capture_by_ref : bool, value : long, token : *CSTToken) : *CapturedVariable

    func make_long_value(&self, value : long, token : *CSTToken) : *LongValue

    func make_negative_value(&self, value : *Value, token : *CSTToken) : *NegativeValue

    func make_not_value(&self, value : *Value, token : *CSTToken) : *NotValue

    func make_null_value(&self, token : *CSTToken) : *NullValue

    func make_number_value(&self, value : bigint, token : *CSTToken) : *NumberValue

    func make_short_value(&self, value : short, token : *CSTToken) : *ShortValue

    func make_sizeof_value(&self, type : *BaseType, token : *CSTToken) : *SizeOfValue

    func make_string_value(&self, value : *string, token : *CSTToken) : *StringValue

    func make_struct_member_initializer(&self, name : *string, value : *Value, structValue : *StructValue) : *StructMemberInitializer

    func make_struct_value(&self, ref : *BaseType, parent_node : *ASTNode, token : *CSTToken) : *StructValue

    func make_ubigint_value(&self, value : ubigint, token : *CSTToken) : *UBigIntValue

    func make_uchar_value(&self, value : uchar, token : *CSTToken) : *UCharValue

    func make_uint128_value(&self, low : ubigint, high : ubigint, token : *CSTToken) : *UInt128Value

    func make_uint_value(&self, value : uint, token : *CSTToken) : *UIntValue

    func make_ulong_value(&self, value : ulong, token : *CSTToken) : *ULongValue

    func make_ushort_value(&self, value : ushort, token : *CSTToken) : *UShortValue

    func make_value_node(&self, value : *Value, parent_node : *ASTNode, token : *CSTToken) : *ValueNode

    func make_identifier(&self, value : *string, is_ns : bool, token : *CSTToken) : *VariableIdentifier

    func make_variant_call(&self, chain : *AccessChain, token : *CSTToken) : *VariantCall

    func make_variant_case(&self, chain : *AccessChain, stmt : *SwitchStatement, token : *CSTToken) : *VariantCase

    func make_variant_case_variable(&self, name : *string, variant_case : *VariantCase, token : *CSTToken) : *VariantCaseVariable

    func make_assignment_stmt(&self, lhs : *Value, rhs : *Value, op : Operation, parent_node : *ASTNode, token : *CSTToken) : *AssignStatement

    func make_break_stmt(&self, loop_node : *LoopASTNode, parent_node : *ASTNode, token : *CSTToken) : *BreakStatement

    func make_comment_stmt(&self, value : *string, multiline : bool, parent_node : *ASTNode, token : *CSTToken) : *Comment

    func make_continue_stmt(&self, loop_node : *LoopASTNode, parent_node : *ASTNode, token : *CSTToken) : *ContinueStatement

    func make_destruct_stmt(&self, array_value : *Value, ptr_value : *Value, is_array : bool, parent_node : *ASTNode, token : *CSTToken) : *DestructStmt

    func make_return_stmt(&self, value : *Value, decl : *FunctionType, parent_node : *ASTNode, token : *CSTToken) : *ReturnStatement

    //SwitchStatement* ASTBuildermake_return_stmt(CSTConverter* converter, Value* value, FunctionType* decl, ASTNode* parent_node, token : CSTToken*);

    //ThrowStatement* ASTBuildermake_throw_stmt(CSTConverter* converter, Value* value, FunctionType* decl, ASTNode* parent_node, token : CSTToken*);

    func make_typealias_stmt(&self, identifier : *string, actual_type : *BaseType, specifier : AccessSpecifier, parent_node : *ASTNode, token : *CSTToken) : *TypealiasStatement

    func make_using_stmt(&self, chain : *AccessChain, is_namespace : bool, token : *CSTToken) : *UsingStmt

    func make_varinit_stmt(&self, is_const : bool, identifier : *string, type : *BaseType, value : *Value, specifier : AccessSpecifier, parent_node : *ASTNode, token : *CSTToken) : *VarInitStatement

    func make_scope(&self, parent_node : *ASTNode, token : *CSTToken) : *Scope

    func make_do_while_loop(&self, condition : *Value, parent_node : *ASTNode, token : *CSTToken) : *DoWhileLoop

    func make_enum_decl(&self, name : *string, specifier : AccessSpecifier, parent_node : *ASTNode, token : *CSTToken) : *EnumDeclaration

    func make_enum_member(&self, name : *string, index : uint, parent_node : *EnumDeclaration, token : *CSTToken) : *EnumMember

    func make_for_loop(&self, initializer : *VarInitStatement, conditionExpr : *Value, incrementerExpr : *ASTNode, parent_node : *ASTNode, token : *CSTToken) : *ForLoop

    func make_function(&self, name : *string, returnType : *BaseType, isVariadic : bool, hasBody : bool, parent_node : *ASTNode, token : *CSTToken) : *FunctionDeclaration

    func make_function_param(&self, name : *string, type : *BaseType, index : uint, value : *Value, implicit : bool, decl : *FunctionType, token : *CSTToken) : *FunctionParam

    func make_generic_param(&self, name : *string, def_type : *BaseType, parent_node : *ASTNode, index : uint, token : *CSTToken) : *GenericTypeParameter

    func make_if_stmt(&self, condition : *Value, is_value : bool, parent_node : *ASTNode, token : *CSTToken) : *IfStatement

    func make_impl_def(&self, interface_type : *BaseType, struct_type : *BaseType, parent_node : *ASTNode, token : *CSTToken) : *ImplDefinition

    func make_init_block(&self, parent_node : *ASTNode, token : *CSTToken) : *InitBlock

    func make_interface_def(&self, name : *string, specifier : AccessSpecifier, parent_node : *ASTNode, token : *CSTToken) : *InterfaceDefinition

    func make_namespace(&self, name : *string, specifier : AccessSpecifier, parent_node : *ASTNode, token : *CSTToken) : *Namespace

    func make_struct_def(&self, name : *string, specifier : AccessSpecifier, parent_node : *ASTNode, token : *CSTToken) : *StructDefinition

    func make_struct_member(&self, name : *string, type : *BaseType, defValue : *Value, isConst : bool, specifier : AccessSpecifier, parent_node : *ASTNode, token : *CSTToken) : *StructMember

    func make_union_def(&self, name : *string, specifier : AccessSpecifier, parent_node : *ASTNode, token : *CSTToken) : *UnionDef

    func make_unsafe_block(&self, node : *ASTNode, token : *CSTToken) : *UnsafeBlock

    func make_while_loop(&self, condition : *Value, node : *ASTNode, token : *CSTToken) : *WhileLoop

    func make_variant_def(&self, name : *string, specifier : AccessSpecifier, node : *ASTNode, token : *CSTToken) : *VariantDefinition

    func make_variant_member(&self, name : *string, parent_node : *VariantDefinition, token : *CSTToken) : *VariantMember

    func make_variant_member_param(&self, name : *string, index : uint, type : *BaseType, defValue : *Value, parent_node : *VariantMember, token : *CSTToken) : *VariantMemberParam


}