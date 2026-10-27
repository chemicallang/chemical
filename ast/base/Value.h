// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ASTAny.h"
#include "ast/utils/Operation.h"
#include <vector>
#include <optional>
#include "DebugCast.h"
#include "ASTAllocator.h"

class SymbolResolver;

class TypeBuilder;

struct TargetData;

#include "BaseTypeKind.h"
#include "ValueKind.h"
#include "std/chem_string_view.h"

class FunctionDeclaration;

class InterfaceDefinition;

class VarInitStatement;

class BaseType;

class ASTDiagnoser;

/**
 * @brief Base class for all values in the AST.
 */
class Value : public ASTAny {
private:

    /**
     * the value kind is stored like this
     */
    ValueKind const _kind;

    /**
     * the type of the value is calculated during symbol resolution
     */
    BaseType* _type;

    /**
     * encoded source location
     */
    SourceLocation _location;

public:

    /**
     * default constructor
     */
    inline explicit constexpr Value(ValueKind k, SourceLocation loc) noexcept : _kind(k), _location(loc), _type(nullptr) {

    };

    /**
     * default constructor
     */
    inline explicit constexpr Value(
            ValueKind k,
            BaseType* type,
            SourceLocation loc
    ) noexcept : _kind(k), _type(type), _location(loc) {

    };

    /**
     * deleted copy constructor
     */
    Value(const Value& other) = delete;

    /**
     * default move constructor
     */
    Value(Value&& other) = default;

    /**
     * any kind of 'value' is returned
     */
    ASTAnyKind any_kind() final {
        return ASTAnyKind::Value;
    }

    /**
     * get the type for this value
     */
    inline BaseType* getType() const noexcept {
        return _type;
    }

    /**
     * you must be sure that type is retained for lifetime of this value
     * this changes the type of the value
     */
    inline void setType(BaseType* type) noexcept {
        _type = type;
    }

    /**
     * get the kind of this value
     */
    inline ValueKind kind() const noexcept {
        return _kind;
    }

    /**
     * get the value kind of this value
     */
    inline ValueKind val_kind() const noexcept {
        return _kind;
    }

    /**
     * get the encoded source location
     */
    inline SourceLocation encoded_location() const noexcept {
        return _location;
    }

    /**
     * change the encoded location of this value
     */
    inline void set_encoded_location(SourceLocation loc) noexcept {
        _location = loc;
    }

    /**
     * extracts a value from a node
     */
    static Value* get_first_value_from_value_node(ASTNode* node);

    /**
     * check if value is an integer literal
     */
    bool isValueIntegerLiteral();

    /**
     * check if value is a r value in frontend
     */
    bool isValueRValueInFrontend();

    /**
     * will check contents, are they of the same kind
     * this method means are both chain values functionally equal
     * they don't mean to check if they have same names, but if both were exchanged
     * they won't have an effect on code generation
     */
    bool is_equal(Value* other, ValueKind this_kind, ValueKind other_kind);

    /**
     * a helper function
     */
    bool is_equal(Value* other) {
        return is_equal(other, val_kind(), other->val_kind());
    }

    /**
     * it must return the node that will be used to find the next node in the access chain
     * this node is extracted from the node that was linked to in link method
     * @return return node that signifies the fragment in the access chain
     */
    virtual ASTNode* linked_node() {
        return nullptr;
    }

    /**
     * last identifier of this chain value
     */
    VariableIdentifier* get_last_id();

    /**
     * get byte size of this value
     */
    virtual uint64_t byte_size(const TargetData& data);

    /**
     * if this value has a child by this name, it should return a pointer to it
     */
    virtual Value* child(InterpretScope& scope, const chem::string_view& name);

    /**
     * give representation of the value as it appears in source
     * this doesn't need to be 100% accurate
     * @return
     */
    std::string representation();

    /**
     * get the last linked if this is a chain value
     */
    ASTNode* get_chain_last_linked();

    /**
     * if this is a chain id, we report assignment to function param, var init
     */
    void report_assignment_of_chain_id();

    /**
     * if this is a chain id, we report address has been taken to function param, var init
     */
    void report_addr_taken_of_chain_id();

    /**
     * is this value has a pointer type (includes strings)
     */
    [[nodiscard]]
    bool is_pointer();

    /**
     * is this value has a pointer type (includes strings)
     */
    [[nodiscard]]
    bool is_pointer_or_ref();

    /**
     * type is only returned when the value is stored in a storage location
     * like var decl, struct member, variant member param qualify
     * function param for example doesn't qualify
     */
    BaseType* get_stored_value_type(ASTAllocator& allocator);

    /**
     * check if this value is a stored pointer
     */
    bool is_stored_ptr_or_ref(ASTAllocator& allocator);

    /**
     * check if this value is a stored reference
     */
    bool is_stored_ref(ASTAllocator& allocator);

    /**
     * check if this value is a pointer or reference
     */
    bool is_ptr_or_ref(ASTAllocator& allocator);

    /**
     * check if this value is a reference (reference type)
     */
    bool is_ref(ASTAllocator& allocator);

    /**
     * this checks if this value is mutable
     * basically something that you can assign to
     */
    bool check_is_mutable(bool assigning);

    /**
     * check if this is a function call
     */
    bool is_func_call();

    /**
     * check if this value is a reference and moved
     */

    /**
     * is an int num value, this includes referenced values
     */
    bool is_value_int_n() {
        return kind() == ValueKind::IntN;
    }

    /**
     * return true if this value is a reference (VariableIdentifier)
     */
    bool reference();

    /**
     * This method is overridden by primitive values like int, float... to return true
     * This method returns whether it's a literal
     * @return true when the value is primitive
     */
    virtual bool primitive() {
        return true;
    }

    /**
     * This method allows to make a copy of the current value
     * this makes a deep copy, so it should be used with care
     */
    virtual Value* copy(ASTAllocator& allocator);

    /**
     * scope value is the value that would be put on the interpret scope value map
     * if user writes var x = 5, we can just place a copy of 5 on the interpret scope
     *
     * if user writes var e = 1 + 1, we need to calculate 1 + 1, that would mean a new value is created
     * and then we put that value on the scope, so expressions can override this method to provide the value
     *
     * this method will always create a copy of the value, which means the caller must handle the returned value
     */
    Value* scope_value(InterpretScope& scope);

    /**
     * is value computable at compile time
     */
    virtual bool compile_time_computable() {
        return primitive();
    }

    /**
     * This returns Value pointer to the object that represents the real value
     * This can be used to get the value after evaluation
     * for ex : identifier represents a variable that contains a value, its not a value itself, but yields a value
     * so it can find its value and return pointer to the Value object that actually holds the value
     */
    virtual Value* evaluated_value(InterpretScope& scope) {
        return this;
    }

    /**
     * a function to be overridden by bool values to return actual values
     */
    bool get_the_bool();

    /**
     * will get the string from the value
     * this only works on string values
     */
    const chem::string_view& get_the_string();

    /**
     * get number value if this value is any interger
     * otherwise nullopt
     */
    std::optional<uint64_t> get_number();

    /**
     * virtual default destructor
     */
    virtual ~Value();

    // --------------------------------------------
    // ------- isValue methods begin here ---------
    // --------------------------------------------

    static constexpr inline bool isIntN(ValueKind k) {
        return k == ValueKind::IntN;
    }

    static constexpr inline bool isFloat(ValueKind k) {
        return k == ValueKind::Float;
    }

    static constexpr inline bool isDouble(ValueKind k) {
        return k == ValueKind::Double;
    }

    static constexpr inline bool isBool(ValueKind k) {
        return k == ValueKind::Bool;
    }

    static constexpr inline bool isString(ValueKind k) {
        return k == ValueKind::String;
    }

    static constexpr inline bool isExpression(ValueKind k) {
        return k == ValueKind::Expression;
    }

    static constexpr inline bool isArrayValue(ValueKind k) {
        return k == ValueKind::ArrayValue;
    }

    static constexpr inline bool isStructValue(ValueKind k) {
        return k == ValueKind::StructValue;
    }

    static constexpr inline bool isLambdaFunc(ValueKind k) {
        return k == ValueKind::LambdaFunc;
    }

    static constexpr inline bool isIfValue(ValueKind k) {
        return k == ValueKind::IfValue;
    }

    static constexpr inline bool isAccessChain(ValueKind k) {
        return k == ValueKind::AccessChain;
    }

    static constexpr inline bool isChainValue(ValueKind k) {
        return k == ValueKind::Identifier || k == ValueKind::FunctionCall || k == ValueKind::IndexOperator || k == ValueKind::AccessChain;
    }

    static constexpr inline bool isSwitchValue(ValueKind k) {
        return k == ValueKind::SwitchValue;
    }

    static constexpr inline bool isLoopValue(ValueKind k) {
        return k == ValueKind::LoopValue;
    }

    static constexpr inline bool isNewValue(ValueKind k) {
        return k == ValueKind::NewValue;
    }

    static constexpr inline bool isIsValue(ValueKind k) {
        return k == ValueKind::IsValue;
    }

    static constexpr inline bool isDereferenceValue(ValueKind k) {
        return k == ValueKind::DereferenceValue;
    }

    static constexpr inline bool isRetStructParamValue(ValueKind k) {
        return k == ValueKind::RetStructParamValue;
    }

    static constexpr inline bool isCastedValue(ValueKind k) {
        return k == ValueKind::CastedValue;
    }

    static constexpr inline bool isIdentifier(ValueKind k) {
        // a GenericInstIdentifier IS a VariableIdentifier (it only adds the generic
        // arguments of a bare generic function reference), so it is reported as an
        // identifier here and as_identifier()/as_identifier_unsafe() return it
        return k == ValueKind::Identifier || k == ValueKind::GenericInstIdentifier;
    }

    static constexpr inline bool isGenericInstIdentifier(ValueKind k) {
        return k == ValueKind::GenericInstIdentifier;
    }

    static constexpr inline bool isIndexOperator(ValueKind k) {
        return k == ValueKind::IndexOperator;
    }

    static constexpr inline bool isFunctionCall(ValueKind k) {
        return k == ValueKind::FunctionCall;
    }

    static constexpr inline bool isNegativeValue(ValueKind k) {
        return k == ValueKind::NegativeValue;
    }

    static constexpr inline bool isNotValue(ValueKind k) {
        return k == ValueKind::NotValue;
    }

    static constexpr inline bool isBitwiseNot(ValueKind k) {
        return k == ValueKind::BitwiseNot;
    }

    static constexpr inline bool isNullValue(ValueKind k) {
        return k == ValueKind::NullValue;
    }

    static constexpr inline bool isSizeOfValue(ValueKind k) {
        return k == ValueKind::SizeOfValue;
    }

    static constexpr inline bool isOffsetOfValue(ValueKind k) {
        return k == ValueKind::OffsetOfValue;
    }

    static constexpr inline bool isVariantCase(ValueKind k) {
        return k == ValueKind::VariantCase;
    }

    static constexpr inline bool isAddrOfValue(ValueKind k) {
        return k == ValueKind::AddrOfValue;
    }

    // --------------------------------------------
    // ------- as_value methods begin here ---------
    // --------------------------------------------

    inline FloatValue* as_float_value() {
        return isFloat(val_kind()) ? ((FloatValue*) this) : nullptr;
    }

    inline DoubleValue* as_double_value() {
        return isDouble(val_kind()) ? ((DoubleValue*) this) : nullptr;
    }

    inline BoolValue* as_bool_value() {
        return isBool(val_kind()) ? ((BoolValue*) this) : nullptr;
    }

    inline StringValue* as_string_value() {
        return isString(val_kind()) ? ((StringValue*) this) : nullptr;
    }

    inline Expression* as_expression() {
        return isExpression(val_kind()) ? ((Expression*) this) : nullptr;
    }

    inline ArrayValue* as_array_value() {
        return isArrayValue(val_kind()) ? ((ArrayValue*) this) : nullptr;
    }

    inline StructValue* as_struct_value() {
        return isStructValue(val_kind()) ? ((StructValue*) this) : nullptr;
    }

    inline LambdaFunction* as_lambda_func() {
        return isLambdaFunc(val_kind()) ? ((LambdaFunction*) this) : nullptr;
    }

    inline AccessChain* as_access_chain() {
        return isAccessChain(val_kind()) ? ((AccessChain*) this) : nullptr;
    }

    inline Value* as_chain_value() {
        return isChainValue(val_kind()) ? ((Value*) this) : nullptr;
    }

    inline IsValue* as_is_value() {
        return isIsValue(val_kind()) ? ((IsValue*) this) : nullptr;
    }

    inline NewTypedValue* as_new_value() {
        return isNewValue(val_kind()) ? ((NewTypedValue*) this) : nullptr;
    }

    inline DereferenceValue* as_deref_value() {
        return isDereferenceValue(val_kind()) ? ((DereferenceValue*) this) : nullptr;
    }

    inline RetStructParamValue* as_ret_struct_param_value() {
        return isRetStructParamValue(val_kind()) ? ((RetStructParamValue*) this) : nullptr;
    }

    inline CastedValue* as_casted_value() {
        return isCastedValue(val_kind()) ? ((CastedValue*) this) : nullptr;
    }

    inline VariableIdentifier* as_identifier() {
        return isIdentifier(val_kind()) ? ((VariableIdentifier*) this) : nullptr;
    }

    inline GenericInstIdentifier* as_generic_inst_identifier() {
        return isGenericInstIdentifier(val_kind()) ? ((GenericInstIdentifier*) this) : nullptr;
    }

    inline IndexOperator* as_index_op() {
        return isIndexOperator(val_kind()) ? ((IndexOperator*) this) : nullptr;
    }

    inline FunctionCall* as_func_call() {
        return isFunctionCall(val_kind()) ? ((FunctionCall*) this) : nullptr;
    }

    inline NegativeValue* as_negative_value() {
        return isNegativeValue(val_kind()) ? ((NegativeValue*) this) : nullptr;
    }

    inline NotValue* as_not_value() {
        return isNotValue(val_kind()) ? ((NotValue*) this) : nullptr;
    }

    inline BitwiseNot* as_bitwise_not() {
        return isBitwiseNot(val_kind()) ? ((BitwiseNot*) this) : nullptr;
    }

    inline NullValue* as_null_value() {
        return isNullValue(val_kind()) ? ((NullValue*) this) : nullptr;
    }

    inline SizeOfValue* as_sizeof_value() {
        return isSizeOfValue(val_kind()) ? ((SizeOfValue*) this) : nullptr;
    }

    inline OffsetOfValue* as_offset_of_value() {
        return isOffsetOfValue(val_kind()) ? ((OffsetOfValue*) this) : nullptr;
    }

    inline VariantCase* as_variant_case() {
        return isVariantCase(val_kind()) ? ((VariantCase*) this) : nullptr;
    }

    inline AddrOfValue* as_addr_of_value() {
        return isAddrOfValue(val_kind()) ? ((AddrOfValue*) this) : nullptr;
    }

    inline PatternMatchExpr* as_pattern_match_expr() {
        return val_kind() == ValueKind::PatternMatchExpr ? ((PatternMatchExpr*) this) : nullptr;
    }

    // --------------------------------------------
    // ------- as_value_unsafe methods begin here ---------
    // --------------------------------------------

    inline IntNumValue* as_int_num_value_unsafe() {
        CHECK_CAST(ValueKind::IntN);
        return ((IntNumValue*) this);
    }

    inline FloatValue* as_float_unsafe() {
        CHECK_CAST(ValueKind::Float);
        return ((FloatValue*) this);
    }

    inline DoubleValue* as_double_unsafe() {
        CHECK_CAST(ValueKind::Double);
        return ((DoubleValue*) this);
    }

    inline BoolValue* as_bool_unsafe() {
        CHECK_CAST(ValueKind::Bool);
        return ((BoolValue*) this);
    }

    inline StringValue* as_string_unsafe() {
        CHECK_CAST(ValueKind::String);
        return ((StringValue*) this);
    }

    inline Expression* as_expression_unsafe() {
        CHECK_CAST(ValueKind::Expression);
        return ((Expression*) this);
    }

    inline ArrayValue* as_array_value_unsafe() {
        CHECK_CAST(ValueKind::ArrayValue);
        return ((ArrayValue*) this);
    }

    inline StructValue* as_struct_value_unsafe() {
        CHECK_CAST(ValueKind::StructValue);
        return ((StructValue*) this);
    }

    inline LambdaFunction* as_lambda_func_unsafe() {
        CHECK_CAST(ValueKind::LambdaFunc);
        return ((LambdaFunction*) this);
    }

    inline IsValue* as_is_value_unsafe() {
        CHECK_CAST(ValueKind::IsValue);
        return ((IsValue*) this);
    }

    inline DereferenceValue* as_dereference_value_unsafe() {
        CHECK_CAST(ValueKind::DereferenceValue);
        return ((DereferenceValue*) this);
    }

    inline RetStructParamValue* as_ret_struct_param_value_unsafe() {
        CHECK_CAST(ValueKind::RetStructParamValue);
        return ((RetStructParamValue*) this);
    }

    inline CastedValue* as_casted_value_unsafe() {
        CHECK_CAST(ValueKind::CastedValue);
        return ((CastedValue*) this);
    }

    inline VariableIdentifier* as_identifier_unsafe() {
        CHECK_COND(isIdentifier(kind()));
        return ((VariableIdentifier*) this);
    }

    inline GenericInstIdentifier* as_generic_inst_identifier_unsafe() {
        CHECK_CAST(ValueKind::GenericInstIdentifier);
        return ((GenericInstIdentifier*) this);
    }

    inline AccessChain* as_access_chain_unsafe() {
        CHECK_CAST(ValueKind::AccessChain);
        return ((AccessChain*) this);
    }

    inline IndexOperator* as_index_op_unsafe() {
        CHECK_CAST(ValueKind::IndexOperator);
        return ((IndexOperator*) this);
    }

    inline FunctionCall* as_func_call_unsafe() {
        CHECK_CAST(ValueKind::FunctionCall);
        return ((FunctionCall*) this);
    }

    inline NegativeValue* as_negative_value_unsafe() {
        CHECK_CAST(ValueKind::NegativeValue);
        return ((NegativeValue*) this);
    }

    inline NotValue* as_not_value_unsafe() {
        CHECK_CAST(ValueKind::NotValue);
        return ((NotValue*) this);
    }

    inline BitwiseNot* as_bitwise_not_unsafe() {
        CHECK_CAST(ValueKind::BitwiseNot);
        return ((BitwiseNot*) this);
    }

    inline NullValue* as_null_value_unsafe() {
        CHECK_CAST(ValueKind::NullValue);
        return ((NullValue*) this);
    }

    inline SizeOfValue* as_sizeof_value_unsafe() {
        CHECK_CAST(ValueKind::SizeOfValue);
        return ((SizeOfValue*) this);
    }

    inline OffsetOfValue* as_offset_of_value_unsafe() {
        CHECK_CAST(ValueKind::OffsetOfValue);
        return ((OffsetOfValue*) this);
    }

    inline VariantCase* as_variant_case_unsafe() {
        CHECK_CAST(ValueKind::VariantCase);
        return ((VariantCase*) this);
    }

    inline AddrOfValue* as_addr_of_value_unsafe() {
        CHECK_CAST(ValueKind::AddrOfValue);
        return ((AddrOfValue*) this);
    }

    inline ReferenceOfValue* as_reference_of_value_unsafe() {
        CHECK_CAST(ValueKind::ReferenceOfValue);
        return ((ReferenceOfValue*) this);
    }

    inline PatternMatchExpr* as_pattern_match_expr_unsafe() {
        CHECK_CAST(ValueKind::PatternMatchExpr);
        return ((PatternMatchExpr*) this);
    }

    inline EmbeddedValue* as_embedded_value_unsafe() {
        CHECK_CAST(ValueKind::EmbeddedValue);
        return ((EmbeddedValue*) this);
    }

    inline IncDecValue* as_inc_dec_value_unsafe() {
        CHECK_CAST(ValueKind::IncDecValue);
        return ((IncDecValue*) this);
    }

    inline IfValue* as_if_value_unsafe() {
        CHECK_CAST(ValueKind::IfValue);
        return ((IfValue*) this);
    }

    inline SwitchValue* as_switch_value_unsafe() {
        CHECK_CAST(ValueKind::SwitchValue);
        return ((SwitchValue*) this);
    }

    inline LoopValue* as_loop_value_unsafe() {
        CHECK_CAST(ValueKind::LoopValue);
        return ((LoopValue*) this);
    }

    inline ExpressiveString* as_expressive_str_unsafe() {
        CHECK_CAST(ValueKind::ExpressiveString);
        return ((ExpressiveString*) this);
    }

    inline AwaitExpression* as_await_expression_unsafe() {
        CHECK_CAST(ValueKind::AwaitExpr);
        return ((AwaitExpression*) this);
    }

};