// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ASTAny.h"
#include "ASTAllocator.h"
#include "ast/utils/Operation.h"
#include <vector>
#include <memory>
#include "std/hybrid_ptr.h"

class SymbolResolver;

#ifdef COMPILER_BUILD
class Codegen;
#include "compiler/llvmfwd.h"

#endif

#include "BaseTypeKind.h"
#include "ValueKind.h"
#include "std/chem_string_view.h"

class FunctionDeclaration;

class InterfaceDefinition;

class VarInitStatement;

class BaseType;

class ASTDiagnoser;

class WrapValue;

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
     * encoded source location
     */
    SourceLocation _location;

public:

    /**
     * check if given kind is a r value
     */
    static bool isValueKindRValue(ValueKind kind);

    /**
     * default constructor
     */
    inline explicit constexpr Value(ValueKind k, SourceLocation loc) noexcept : _kind(k), _location(loc) {

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

    inline void set_encoded_location(SourceLocation loc) noexcept {
        _location = loc;
    }

    /**
     * extracts a value from a node
     */
    static Value* get_first_value_from_value_node(ASTNode* node);

    /**
     * check if given kind is a r value
     */
    bool isValueRValue(ASTAllocator& allocator);

    /**
     * this function is called to allow variable identifiers to link with a node on the map
     * that will help it provide information, to allow it to generate code, or interpret
     * The reason it takes a reference to unique_ptr<Value> is because the current value
     * can basically replace itself in the pointer, some compile time values like sizeof
     * replace themselves at resolution phase
     */
    virtual bool link(SymbolResolver& linker, Value*& value_ptr, BaseType* expected_type = nullptr) {
        return true;
    }

    /**
     * every access chain or identifier x, x.y is used to access elements, however
     * when used in lhs of a assignment statement, it is being used to assign, this distinction is required
     * in some cases, for example when accessing something, we may be accessing a moved variable,
     * however when assigning something, it may have been moved but we allow it because assignment will make it
     * unmoved
     * for example assume 'x' is moved now if we use it in rhs
     * var y = x <-- error 'x' is moved already, but if we use it in lhs
     * x = something_else <-- no error because x is being unmoved !
     * this is called by lhs value of assignment statement to link itself, no other place !
     */
    bool link_assign(SymbolResolver& linker, Value*& value_ptr, BaseType* expected_type = nullptr);

    /**
     * it must return the node that will be used to find the next node in the access chain
     * this node is extracted from the node that was linked to in link method
     * @return return node that signifies the fragment in the access chain
     */
    virtual ASTNode* linked_node() {
        return nullptr;
    }

    /**
     * get byte size of this value
     */
    virtual uint64_t byte_size(bool is64Bit);

    /**
     * if this value has a child by this name, it should return a pointer to it
     */
    virtual Value* child(InterpretScope& scope, const chem::string_view& name);

    /**
     * this function returns a function declaration for a member function
     */
    virtual Value* call_member(InterpretScope& scope, const chem::string_view& name, std::vector<Value*>& values);

    /**
     * index operator [] calls this on a value
     */
    virtual Value* index(InterpretScope& scope, int i);

    /**
     * set the child value, with given name, performing operation op
     */
    void set_child_value(InterpretScope& scope, const chem::string_view& name, Value* value, Operation op);

    /**
     * this function expects this identifier value to find itself in the parent value given to it
     * this is only expected to be overridden by identifiers
     * @param parent
     * @return
     */
    virtual Value* find_in(InterpretScope& scope, Value* parent);

    /**
     * set the value of this value, this is lhs in assignment
     */
    void set_value(InterpretScope& scope, Value* value, Operation op, SourceLocation location);

    /**
     * called on a identifier to set it's value in the given parent
     */
    void set_value_in(InterpretScope& scope, Value* parent, Value* value, Operation op, SourceLocation location);

    /**
     * give representation of the value as it appears in source
     * this doesn't need to be 100% accurate
     * @return
     */
    std::string representation();

    /**
     * will get linked struct, if this is linked with a func param
     * and the param's type is linked type with struct
     */
    StructDefinition* get_param_linked_struct();

    /**
     * get the known type from linked node of this chain value
     */
    virtual BaseType* known_type() {
        return nullptr;
    }

    /**
     * get the pure type from known type directly
     */
    BaseType* pure_type_ptr();

    /**
     * this method should be preferred over get_base_type()->get_child_type()
     * because parent type returned type will die when get_base_type goes out of scope making
     * child_type pointer invalid
     */
    hybrid_ptr<BaseType> get_child_type();

    /**
     * should build chain type returns true if a single identifier is linked with a union
     * meaning if you try to access a child of a union, this method would return true
     * because unions mean different types
     */
    static bool should_build_chain_type(std::vector<Value*>& chain, unsigned index);

    /**
     * get pure type from the base type
     */
    BaseType* get_pure_type(ASTAllocator& allocator);

    /**
     * create a base type that represents the type of this value
     */
    virtual BaseType* create_type(ASTAllocator& allocator);

    /**
     * is this value has a pointer type (includes strings)
     */
    [[nodiscard]]
    bool is_pointer() const {
        const auto k = type_kind();
        return k == BaseTypeKind::Pointer || k == BaseTypeKind::String;
    }

    /**
     * is this value has a pointer type (includes strings)
     */
    [[nodiscard]]
    bool is_pointer_or_ref() const {
        const auto k = type_kind();
        return k == BaseTypeKind::Pointer || k == BaseTypeKind::String || k == BaseTypeKind::Reference;
    }

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
     * check if this value references other values  (access chain or identifier)
     */
    bool is_ref_value();

    /**
     * this checks if this value is mutable
     * basically something that you can assign to
     */
    bool check_is_mutable(ASTAllocator& allocator, bool assigning);

    /**
     * is this value an l value, in C++ l values are those that have backing storage
     */
    bool is_ref_l_value();

    /**
     * is this value a function call
     */
    bool is_chain_func_call();

    /**
     * check if this is a function call
     */
    bool is_func_call();

    /**
     * check if this value is a reference and moved
     */
    bool is_ref_moved();

    /**
     * will get this value as identifier, if it's wrapped in access chain or not
     */
    VariableIdentifier* get_chain_id();

    /**
     * a value that references a struct that is mem copied into arguments
     * however the value is not moved, because it's not movable
     */
    bool requires_memcpy_ref_struct(BaseType* known_type);

#ifdef COMPILER_BUILD

    /**
     * set the drop flag, for this value
     * @return true on success / no need, false on failure
     */
    bool set_drop_flag_for_ref(Codegen& gen, bool flag);

    /**
     * when a value has been moved, and it has a associated runtime drop flag
     * we need to make sure that flag is set to false (don't drop)
     * @return true on success / no need, false on failure
     */
    inline bool set_drop_flag_for_moved_ref(Codegen& gen) {
        // set drop flag to false, so that value is not dropped
        return set_drop_flag_for_ref(gen, false);
    }

    /**
     * when a value has been unmoved, by re assignment or another methods, and it has a associated runtime drop flag
     * we need to make sure that flag is set to true (drop it)
     * @return true on success / no need, false on failure
     */
    inline bool set_drop_flag_for_unmoved_ref(Codegen& gen) {
        // set drop flag to true, so that now value is dropped
        return set_drop_flag_for_ref(gen, true);
    }

    /**
     * value is supposed to destruct itself, call the destructor, because scope has ended
     */
    void llvm_destruct(Codegen& gen, llvm::Value* allocaInst);

    /**
     * load the given value, taking in to account structs
     */
    static llvm::Value* load_value(Codegen& gen, BaseType* known_t, llvm::Type* type, llvm::Value* ptr, SourceLocation location);

    /**
     * load the given value, taking in to account structs
     */
    static llvm::Value* load_value(Codegen& gen, Value* value, llvm::Value* ptr) {
        return load_value(gen, value->known_type(), value->llvm_type(gen), ptr, value->encoded_location());
    }

    /**
     * allocates this value with this identifier, and also creates a store instruction
     */
    virtual llvm::AllocaInst* llvm_allocate(
            Codegen& gen,
            const std::string& identifier,
            BaseType* expected_type
    );

    /**
     * store this value in the allocated struct value
     *
     * it takes an index to store value at, it returns an index to store next value at
     * this is because some values store multiple values making next value to be stored at index + 2
     *
     * @param index is the index at which the value should be stored
     * @return an index at which next value should be stores is returned
     */
    virtual unsigned int store_in_struct(
            Codegen& gen,
            Value* parent,
            llvm::Value* allocated,
            llvm::Type* allocated_type,
            std::vector<llvm::Value *> idxList,
            unsigned int index,
            BaseType* expected_type
    );

    /**
     * store this value in the allocated array
     * this method is same as above store_in_struct
     *
     * only this method stores this value in an array
     * by default it gets the value, stores it in array at the given index, return index + 1
     *
     * this method could be overridden by array values to provide different behavior for nested arrays
     */
    virtual unsigned int store_in_array(
            Codegen& gen,
            Value* parent,
            llvm::Value* allocated,
            llvm::Type* allocated_type,
            std::vector<llvm::Value *> idxList,
            unsigned int index,
            BaseType* expected_type
    );

    /**
     * It get's the element pointer in the allocated parent value
     * the parent can be of type struct or array, so yeah, It allows us to
     * get an index for element of an array or struct, so we can load it, or store in it
     */
    static llvm::Value* get_element_pointer(
            Codegen& gen,
            llvm::Type* in_type,
            llvm::Value* ptr,
            std::vector<llvm::Value *>& idxList,
            unsigned int index
    );

    /**
     * returns a llvm pointer to the value, if has any
     * @param gen
     * @return
     */
    virtual llvm::Value* llvm_pointer(Codegen& gen);

    /**
     * creates and returns the llvm value
     * type which is optional can be provided to support features that depend on knowledge
     * of type, for example dynamic objects, when it's var x : dyn X = X() <-- x is created and put in a dynamic object
     * the dynamic object is just a struct with two pointers, to actual object, and to implementation like Rust
     */
    virtual llvm::Value* llvm_value(Codegen& gen, BaseType* type = nullptr);

    /**
     * if statements call this, to evaluate conditional values
     * if statements need to run a block, on condition, that block is given to this function as end_block
     * this way expression values can optimize expressions and then progress into this block without calculating multiple things
     */
    virtual void llvm_conditional_branch(Codegen& gen, llvm::BasicBlock* then_block, llvm::BasicBlock* otherwise_block);

    /**
     * this method is called by function call to get the parameter value for this Value
     * if this class defines specific behavior for function call, it should override this method
     */
    virtual llvm::Value* llvm_arg_value(Codegen& gen, BaseType* expected_type);

    /**
     * this method is called by return statement to get the return value for this Value
     * if this class defines specific behavior for return, it should override this method
     */
    virtual llvm::Value* llvm_ret_value(Codegen& gen, ReturnStatement* returnStmt);

    /**
     * called by assignment, to assign the current value to left hand side
     */
    virtual void llvm_assign_value(Codegen& gen, llvm::Value* lhsPtr, Value* lhs);

    /**
     * destruct the given destructibles
     */
    static void destruct(Codegen& gen, std::vector<std::pair<Value*, llvm::Value*>>& destructibles);

    /**
     * add member index for the given identifier
     * WARNING : parent can be null ptr when this is the first element in access chain
     * @return whether it was successful in access index(s)
     */
    virtual bool add_member_index(Codegen& gen, Value* parent, std::vector<llvm::Value*>& indexes);

    /**
     * add child index in llvm indexes vector
     */
    virtual bool add_child_index(Codegen& gen, std::vector<llvm::Value*>& indexes, const chem::string_view& name);

#endif

    /**
     * is an int num value, this includes referenced values
     */
    bool is_value_int_n() {
        auto k = val_kind();
        return k >= ValueKind::IntNStart && k <= ValueKind::IntNEnd;
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
     * just a helper method, to evaluate a value as a boolean
     */
    inline bool evaluated_bool(InterpretScope& scope) {
        return evaluated_value(scope)->get_the_bool();
    }

    /**
     * a function to be overridden by char values to return actual values
     */
    char get_the_char();

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
     * get any number from the type
     */
    std::optional<uint64_t> get_the_number();

    /**
     * a function to be overridden by values that can return int
     */
    int get_the_int();

    /**
     * get number value if this value is any interger
     * otherwise nullopt
     */
    std::optional<uint64_t> get_number();

    /**
     * will return a unsigned int representation
     */
    unsigned get_the_uint();

    /**
     * a function to be overridden by values that can return float
     */
    float get_the_float();

    /**
     * a function to be overridden by values that can return double
     */
    double get_the_double();

    /**
     * get the type kind for this value
     */
    virtual BaseTypeKind type_kind() const {
        return BaseTypeKind::Unknown;
    }

    /**
     * virtual default destructor
     */
    virtual ~Value();

    // --------------------------------------------
    // ------- isValue methods begin here ---------
    // --------------------------------------------

    static constexpr inline bool isInt(ValueKind k) {
        return k == ValueKind::Int;
    }

    static constexpr inline bool isUInt(ValueKind k) {
        return k == ValueKind::UInt;
    }

    static constexpr inline bool isChar(ValueKind k) {
        return k == ValueKind::Char;
    }

    static constexpr inline bool isUChar(ValueKind k) {
        return k == ValueKind::UChar;
    }

    static constexpr inline bool isShort(ValueKind k) {
        return k == ValueKind::Short;
    }

    static constexpr inline bool isUShort(ValueKind k) {
        return k == ValueKind::UShort;
    }

    static constexpr inline bool isLong(ValueKind k) {
        return k == ValueKind::Long;
    }

    static constexpr inline bool isULong(ValueKind k) {
        return k == ValueKind::ULong;
    }

    static constexpr inline bool isBigInt(ValueKind k) {
        return k == ValueKind::BigInt;
    }

    static constexpr inline bool isUBigInt(ValueKind k) {
        return k == ValueKind::UBigInt;
    }

    static constexpr inline bool isInt128(ValueKind k) {
        return k == ValueKind::Int128;
    }

    static constexpr inline bool isUInt128(ValueKind k) {
        return k == ValueKind::UInt128;
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

    static constexpr inline bool isNumberValue(ValueKind k) {
        return k == ValueKind::NumberValue;
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
        return k == ValueKind::Identifier;
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

    static constexpr inline bool isNullValue(ValueKind k) {
        return k == ValueKind::NullValue;
    }

    static constexpr inline bool isSizeOfValue(ValueKind k) {
        return k == ValueKind::SizeOfValue;
    }

    static constexpr inline bool isVariantCase(ValueKind k) {
        return k == ValueKind::VariantCase;
    }

    static constexpr inline bool isAddrOfValue(ValueKind k) {
        return k == ValueKind::AddrOfValue;
    }

    static constexpr inline bool isWrapValue(ValueKind k) {
        return k == ValueKind::WrapValue;
    }

    // --------------------------------------------
    // ------- as_value methods begin here ---------
    // --------------------------------------------

    inline IntValue* as_int_value() {
        return isInt(val_kind()) ? ((IntValue*) this) : nullptr;
    }

    inline UIntValue* as_uint_value() {
        return isUInt(val_kind()) ? ((UIntValue*) this) : nullptr;
    }

    inline CharValue* as_char_value() {
        return isChar(val_kind()) ? ((CharValue*) this) : nullptr;
    }

    inline UCharValue* as_uchar() {
        return isUChar(val_kind()) ? ((UCharValue*) this) : nullptr;
    }

    inline ShortValue* as_short() {
        return isShort(val_kind()) ? ((ShortValue*) this) : nullptr;
    }

    inline UShortValue* as_ushort() {
        return isUShort(val_kind()) ? ((UShortValue*) this) : nullptr;
    }

    inline LongValue* as_long() {
        return isLong(val_kind()) ? ((LongValue*) this) : nullptr;
    }

    inline ULongValue* as_ulong() {
        return isULong(val_kind()) ? ((ULongValue*) this) : nullptr;
    }

    inline BigIntValue* as_bigint() {
        return isBigInt(val_kind()) ? ((BigIntValue*) this) : nullptr;
    }

    inline UBigIntValue* as_ubigint() {
        return isUBigInt(val_kind()) ? ((UBigIntValue*) this) : nullptr;
    }

    inline Int128Value* as_int128() {
        return isInt128(val_kind()) ? ((Int128Value*) this) : nullptr;
    }

    inline UInt128Value* as_uint128() {
        return isUInt128(val_kind()) ? ((UInt128Value*) this) : nullptr;
    }

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

    inline NumberValue* as_number_value() {
        return isNumberValue(val_kind()) ? ((NumberValue*) this) : nullptr;
    }

    inline AccessChain* as_access_chain() {
        return isAccessChain(val_kind()) ? ((AccessChain*) this) : nullptr;
    }

    inline ChainValue* as_chain_value() {
        return isChainValue(val_kind()) ? ((ChainValue*) this) : nullptr;
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

    inline NullValue* as_null_value() {
        return isNullValue(val_kind()) ? ((NullValue*) this) : nullptr;
    }

    inline SizeOfValue* as_sizeof_value() {
        return isSizeOfValue(val_kind()) ? ((SizeOfValue*) this) : nullptr;
    }

    inline VariantCase* as_variant_case() {
        return isVariantCase(val_kind()) ? ((VariantCase*) this) : nullptr;
    }

    inline AddrOfValue* as_addr_of_value() {
        return isAddrOfValue(val_kind()) ? ((AddrOfValue*) this) : nullptr;
    }

    inline WrapValue* as_wrap_value() {
        return isWrapValue(val_kind()) ? ((WrapValue*) this) : nullptr;
    }

    // --------------------------------------------
    // ------- as_value_unsafe methods begin here ---------
    // --------------------------------------------

    inline IntValue* as_int_unsafe() {
        return ((IntValue*) this);
    }

    inline UIntValue* as_uint_unsafe() {
        return ((UIntValue*) this);
    }

    inline CharValue* as_char_unsafe() {
        return ((CharValue*) this);
    }

    inline UCharValue* as_uchar_unsafe() {
        return ((UCharValue*) this);
    }

    inline ShortValue* as_short_unsafe() {
        return ((ShortValue*) this);
    }

    inline UShortValue* as_ushort_unsafe() {
        return ((UShortValue*) this);
    }

    inline LongValue* as_long_unsafe() {
        return ((LongValue*) this);
    }

    inline ULongValue* as_ulong_unsafe() {
        return ((ULongValue*) this);
    }

    inline BigIntValue* as_bigint_unsafe() {
        return ((BigIntValue*) this);
    }

    inline UBigIntValue* as_ubigint_unsafe() {
        return ((UBigIntValue*) this);
    }

    inline IntNumValue* as_int_num_value_unsafe() {
        return ((IntNumValue*) this);
    }

    inline Int128Value* as_int128_unsafe() {
        return ((Int128Value*) this);
    }

    inline UInt128Value* as_uint128_unsafe() {
        return ((UInt128Value*) this);
    }

    inline FloatValue* as_float_unsafe() {
        return ((FloatValue*) this);
    }

    inline DoubleValue* as_double_unsafe() {
        return ((DoubleValue*) this);
    }

    inline BoolValue* as_bool_unsafe() {
        return ((BoolValue*) this);
    }

    inline StringValue* as_string_unsafe() {
        return ((StringValue*) this);
    }

    inline Expression* as_expression_unsafe() {
        return ((Expression*) this);
    }

    inline ArrayValue* as_array_value_unsafe() {
        return ((ArrayValue*) this);
    }

    inline StructValue* as_struct_value_unsafe() {
        return ((StructValue*) this);
    }

    inline LambdaFunction* as_lambda_func_unsafe() {
        return ((LambdaFunction*) this);
    }

    inline NumberValue* as_number_value_unsafe() {
        return ((NumberValue*) this);
    }

    inline IsValue* as_is_value_unsafe() {
        return ((IsValue*) this);
    }

    inline DereferenceValue* as_dereference_value_unsafe() {
        return ((DereferenceValue*) this);
    }

    inline RetStructParamValue* as_ret_struct_param_value_unsafe() {
        return ((RetStructParamValue*) this);
    }

    inline CastedValue* as_casted_value_unsafe() {
        return ((CastedValue*) this);
    }

    inline VariableIdentifier* as_identifier_unsafe() {
        return ((VariableIdentifier*) this);
    }

    inline AccessChain* as_access_chain_unsafe() {
        return ((AccessChain*) this);
    }

    inline IndexOperator* as_index_op_unsafe() {
        return ((IndexOperator*) this);
    }

    inline FunctionCall* as_func_call_unsafe() {
        return ((FunctionCall*) this);
    }

    inline NegativeValue* as_negative_value_unsafe() {
        return ((NegativeValue*) this);
    }

    inline NotValue* as_not_value_unsafe() {
        return ((NotValue*) this);
    }

    inline NullValue* as_null_value_unsafe() {
        return ((NullValue*) this);
    }

    inline SizeOfValue* as_sizeof_value_unsafe() {
        return ((SizeOfValue*) this);
    }

    inline VariantCase* as_variant_case_unsafe() {
        return ((VariantCase*) this);
    }

    inline AddrOfValue* as_addr_of_value_unsafe() {
        return ((AddrOfValue*) this);
    }

    inline WrapValue* as_wrap_value_unsafe() {
        return ((WrapValue*) this);
    }

};