import "AccessSpecifier.ch"
import "./PtrVec.ch"
import "@std/string_view.ch"
import "./BatchAllocator.ch"
import "./ValueKind.ch"
import "./SymbolResolver.ch"
import "./Operation.ch"

using namespace std;

// The Base Structs

struct ASTNode {}

struct BaseType {}

struct IntNType : BaseType {}

struct Value {

    func getKind(&self) : ValueKind

}

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

struct AccessChain : Value {

    func get_values(&self) : *VecRef<Value>;

}

struct ValueWrapperNode : ASTNode {}

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

    func add_value(&self, name : &string_view, initializer : *StructMemberInitializer)

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

struct Scope : ASTNode {

    func getNodes(&self) : *VecRef<ASTNode>;

}

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

    func add_initializer(&self, name : &string_view, is_inherited_type : bool, value : *Value);

}

struct InterfaceDefinition : ASTNode {

    func add_function(decl : *FunctionDeclaration)

}

struct Namespace : ASTNode {

     func get_body(&self) : *VecRef<ASTNode>;

}

struct StructDefinition : ASTNode {

    func add_member(name : &string_view, member : *StructMember)

    func add_function(decl : *FunctionDeclaration)

    func get_generic_params(&self) : *VecRef<GenericTypeParameter>;

}

struct StructMember : ASTNode {}

struct UnionDef : ASTNode {

    func add_member(name : &string_view, member : *StructMember)

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

    func add_member(&self, name : &string_view, member : *StructMember)

}

struct VariantMember : ASTNode {

    func add_param(&self, param : *VariantMemberParam);

}

struct VariantMemberParam : ASTNode {}

struct SymResNode : ASTNode {}

struct SymResValue : Value {}

typealias SymResNodeDeclarationFn = (allocator : *mut ASTBuilder, resolver : *mut SymbolResolver, data_ptr : **mut void) => void;

typealias SymResNodeReplacementFn = (allocator : *mut ASTBuilder, resolver : *mut SymbolResolver, data : *mut void) => *mut ASTNode

typealias SymResValueReplacementFn= (allocator : *mut ASTBuilder, resolver : *mut SymbolResolver, data : *mut void) => *mut Value

@compiler.interface
public struct ASTBuilder : BatchAllocator {

    func allocate_with_cleanup(&self, obj_size : size_t, alignment : size_t, cleanup_fn : (obj : *void) => void) : *mut void;

    // the decl_fn is only called if the node is a top level node and not inside a function
    // to track, allocate the root node and set the data_ptr to it, otherwise set it to null
    // in the repl_fn check if it is not set, allocate it if not, also declare will only declare now as a nested level node
    // since the top level nodes can be accessed via nodes above it, this is required
    func make_sym_res_node(&self, decl_fn : SymResNodeDeclarationFn, repl_fn : SymResNodeReplacementFn, data_ptr : void*, parent_node : ASTNode*, location : ubigint) : *mut SymResNode

    func make_sym_res_value(&self, repl_fn : SymResValueReplacementFn, data_ptr : void*, location : ubigint) : *mut SymResValue;

    func make_any_type(&self, location : ubigint) : *mut AnyType

    func make_array_type(&self, elem_type : *BaseType, array_size : int, location : ubigint) : *mut ArrayType

    func make_bigint_type(&self, location : ubigint) : *mut BigIntType

    func make_bool_type(&self, location : ubigint) : *mut BoolType

    func make_char_type(&self, location : ubigint) : *mut BoolType

    func make_double_type(&self, location : ubigint) : *mut DoubleType

    func make_dynamic_type(&self, child_type : *BaseType, location : ubigint) : *mut DynamicType

    func make_float_type(&self, location : ubigint) : *mut FloatType

    func make_func_type(&self, returnType : *BaseType, isVariadic : bool, isCapturing : bool, parent : *ASTNode, location : ubigint) : *mut FunctionType

    func make_generic_type(&self, linkedType : *LinkedType) : *mut GenericType

    func make_int128_type(&self, location : ubigint) : *mut Int128Type

    func make_int_type(&self, location : ubigint) : *mut IntType

    func make_linked_type(&self, type : &string_view, linked : *ASTNode, location : ubigint) : *mut LinkedType

    func make_linked_value_type(&self, value : *Value, location : ubigint) : *mut LinkedValueType

    func make_literal_type(&self, child_type : *BaseType, location : ubigint) : *mut LiteralType

    func make_long_type(&self, is64Bit : bool, location : ubigint) : *mut LongType

    func make_ptr_type(&self, child_type : *BaseType, location : ubigint) : *mut PointerType

    func make_reference_type(&self, child_type : *BaseType, location : ubigint) : *mut ReferenceType

    func make_short_type(&self, location : ubigint) : *mut ShortType

    func make_string_type(&self, location : ubigint) : *mut StringType

    func make_ubigint_type(&self, location : ubigint) : *mut UBigIntType

    func make_uchar_type(&self, location : ubigint) : *mut UCharType

    func make_uint128_type(&self, location : ubigint) : *mut UInt128Type

    func make_uint_type(&self, location : ubigint) : *mut UIntType

    func make_ulong_type(&self, is64Bit : bool, location : ubigint) : *mut ULongType

    func make_ushort_type(&self, location : ubigint) : *mut UShortType

    func make_void_type(&self, location : ubigint) : *mut VoidType

    func make_access_chain(&self, is_node : bool, location : ubigint) : *mut AccessChain

    func make_value_wrapper(&self, value : *Value, parent_node : *ASTNode) : *ValueWrapperNode

    func make_addr_of_value(&self, value : *Value, location : ubigint) : *mut AddrOfValue

    func make_array_value(&self, type : *BaseType, location : ubigint) : *mut ArrayValue

    func make_bigint_value(&self, value : bigint, location : ubigint) : *mut BigIntValue

    func make_bool_value(&self, value : bool, location : ubigint) : *mut BoolValue

    func make_casted_value(&self, value : *Value, type : *BaseType, location : ubigint) : *mut CastedValue

    func make_char_value(&self, value : char, location : ubigint) : *mut CharValue

    func make_dereference_value(&self, value : *Value, location : ubigint) : *mut DereferenceValue

    func make_double_value(&self, value : double, location : ubigint) : *mut DoubleValue

    func make_expression_value(&self, first : *Value, second : *Value, op : Operation, is64Bit : bool, location : ubigint) : *mut Expression

    func make_float_value(&self, value : float, location : ubigint) : *mut FloatValue

    func make_function_call_value(&self, parent_val : *Value, location : ubigint) : *mut FunctionCall

    func make_index_op_value(&self, parent_val : *Value, location : ubigint) : *mut IndexOperator

    func make_int128_value(&self, mag : ubigint, is_neg : bool, location : ubigint) : *mut Int128Value

    func make_int_value(&self, value : int, location : ubigint) : *mut IntValue

    func make_is_value(&self, value : *Value, type : *BaseType, is_negating : bool, location : ubigint) : *mut IsValue

    func make_lambda_function(&self, value : *Value, type : *BaseType, isVariadic : bool, parent_node : *ASTNode, location : ubigint) : *mut LambdaFunction

    func make_captured_variable(&self, name : &string_view, index : uint, capture_by_ref : bool, value : long, location : ubigint) : *mut CapturedVariable

    func make_long_value(&self, value : long, is64Bit : bool, location : ubigint) : *mut LongValue

    func make_negative_value(&self, value : *Value, location : ubigint) : *mut NegativeValue

    func make_not_value(&self, value : *Value, location : ubigint) : *mut NotValue

    func make_null_value(&self, location : ubigint) : *mut NullValue

    func make_number_value(&self, value : bigint, location : ubigint) : *mut NumberValue

    func make_short_value(&self, value : short, location : ubigint) : *mut ShortValue

    func make_sizeof_value(&self, type : *BaseType, location : ubigint) : *mut SizeOfValue

    func make_string_value(&self, value : &string_view, location : ubigint) : *mut StringValue

    func make_struct_member_initializer(&self, name : &string_view, value : *Value, structValue : *StructValue) : *mut StructMemberInitializer

    func make_struct_value(&self, ref : *BaseType, parent_node : *ASTNode, location : ubigint) : *mut StructValue

    func make_ubigint_value(&self, value : ubigint, location : ubigint) : *mut UBigIntValue

    func make_uchar_value(&self, value : uchar, location : ubigint) : *mut UCharValue

    func make_uint128_value(&self, low : ubigint, high : ubigint, location : ubigint) : *mut UInt128Value

    func make_uint_value(&self, value : uint, location : ubigint) : *mut UIntValue

    func make_ulong_value(&self, value : ulong, is64Bit : bool, location : ubigint) : *mut ULongValue

    func make_ushort_value(&self, value : ushort, location : ubigint) : *mut UShortValue

    func make_value_node(&self, value : *Value, parent_node : *ASTNode, location : ubigint) : *mut ValueNode

    func make_identifier(&self, value : &string_view, is_ns : bool, location : ubigint) : *mut VariableIdentifier

    func make_variant_call(&self, chain : *AccessChain, location : ubigint) : *mut VariantCall

    func make_variant_case(&self, chain : *AccessChain, stmt : *SwitchStatement, location : ubigint) : *mut VariantCase

    func make_variant_case_variable(&self, name : &string_view, parent_val : *VariableIdentifier, stmt : *SwitchStatement, location : ubigint) : *mut VariantCaseVariable

    func make_assignment_stmt(&self, lhs : *Value, rhs : *Value, op : Operation, parent_node : *ASTNode, location : ubigint) : *mut AssignStatement

    func make_break_stmt(&self, loop_node : *LoopASTNode, parent_node : *ASTNode, location : ubigint) : *BreakStatement

    func make_comment_stmt(&self, value : &string_view, multiline : bool, parent_node : *ASTNode, location : ubigint) : *mut Comment

    func make_continue_stmt(&self, loop_node : *LoopASTNode, parent_node : *ASTNode, location : ubigint) : *mut ContinueStatement

    func make_destruct_stmt(&self, array_value : *Value, ptr_value : *Value, is_array : bool, parent_node : *ASTNode, location : ubigint) : *mut DestructStmt

    func make_return_stmt(&self, value : *Value, decl : *FunctionType, parent_node : *ASTNode, location : ubigint) : *mut ReturnStatement

    //SwitchStatement* ASTBuildermake_return_stmt(CSTConverter* converter, Value* value, FunctionType* decl, ASTNode* parent_node, location : ubigint);

    //ThrowStatement* ASTBuildermake_throw_stmt(CSTConverter* converter, Value* value, FunctionType* decl, ASTNode* parent_node, location : ubigint);

    func make_typealias_stmt(&self, identifier : &string_view, id_loc : ubigint, actual_type : *BaseType, specifier : AccessSpecifier, parent_node : *ASTNode, location : ubigint) : *mut TypealiasStatement

    func make_using_stmt(&self, chain : *AccessChain, parent_node : *ASTNode, is_namespace : bool, location : ubigint) : *mut UsingStmt

    func make_varinit_stmt(&self, is_const : bool, identifier : &string_view, id_loc : ubigint, type : *BaseType, value : *Value, specifier : AccessSpecifier, parent_node : *ASTNode, location : ubigint) : *mut VarInitStatement

    func make_scope(&self, parent_node : *ASTNode, location : ubigint) : *mut Scope

    func make_do_while_loop(&self, condition : *Value, parent_node : *ASTNode, location : ubigint) : *mut DoWhileLoop

    func make_enum_decl(&self, name : &string_view, name_loc : ubigint, underlying_type : *mut IntNType, specifier : AccessSpecifier, parent_node : *ASTNode, location : ubigint) : *mut EnumDeclaration

    func make_enum_member(&self, name : &string_view, index : uint, init_value : *Value, parent_node : *EnumDeclaration, location : ubigint) : *mut EnumMember

    func make_for_loop(&self, initializer : *VarInitStatement, conditionExpr : *Value, incrementerExpr : *ASTNode, parent_node : *ASTNode, location : ubigint) : *mut ForLoop

    func make_function(&self, name : &string_view, name_location : ubigint, returnType : *BaseType, isVariadic : bool, hasBody : bool, parent_node : *ASTNode, location : ubigint) : *mut FunctionDeclaration

    func make_function_param(&self, name : &string_view, type : *BaseType, index : uint, value : *Value, implicit : bool, decl : *FunctionType, location : ubigint) : *mut FunctionParam

    func make_generic_param(&self, name : &string_view, at_least_type : *BaseType, def_type : *BaseType, parent_node : *ASTNode, index : uint, location : ubigint) : *mut GenericTypeParameter

    func make_if_stmt(&self, condition : *Value, is_value : bool, parent_node : *ASTNode, location : ubigint) : *mut IfStatement

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

@comptime
func <T> (builder : &mut ASTBuilder) allocate() : *mut T {
    // TODO get destructor function
    var delete_fn = compiler::get_child_fn(T, "delete");
    if(delete_fn != null) {
        return compiler::wrap(builder.allocate_with_cleanup(#sizeof(T), #alignof(T), delete_fn))
    } else {
        return compiler::wrap(builder.allocate_size(#sizeof(T), #alignof(T)))
    }
}