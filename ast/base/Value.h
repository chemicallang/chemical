// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ASTAny.h"
#include "ASTAllocator.h"
#include "ValueType.h"
#include "ast/utils/Operation.h"
#include <vector>
#include <memory>
#include "std/hybrid_ptr.h"

class SymbolResolver;

#ifdef COMPILER_BUILD
class Codegen;
#include "compiler/llvmfwd.h"

#endif

#include "Visitor.h"
#include "BaseTypeKind.h"
#include "ValueKind.h"

class FunctionDeclaration;

class InterfaceDefinition;

class StructValue;

class VarInitStatement;

class FunctionCall;

class BaseType;

class ASTDiagnoser;

/**
 * @brief Base class for all values in the AST.
 */
class Value : public ASTAny {
public:

    /**
     * default constructor
     */
    Value() = default;

    /**
     * deleted copy constructor
     */
    Value(const Value& other) = delete;

    /**
     * default move constructor
     */
    Value(Value&& other) = default;

    /**
     * move assignment operator
     */
    Value& operator =(Value &&other) = default;

    /**
     * any kind of 'value' is returned
     */
    ASTAnyKind any_kind() override {
        return ASTAnyKind::Value;
    }

    /**
     * get the value kind
     */
    virtual ValueKind val_kind() = 0;

    /**
     * extracts a value from a node
     */
    static Value* get_first_value_from_value_node(ASTNode* node);

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
    virtual bool link_assign(SymbolResolver& linker, Value*& value_ptr, BaseType* expected_type = nullptr) {
        return link(linker, value_ptr, expected_type);
    }

    /**
     * relink value after generic types are known in the function call
     */
    virtual void relink_after_generic(SymbolResolver& linker, BaseType* expected_type) {
        // does nothing
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
     * get byte size of this value
     */
    virtual uint64_t byte_size(bool is64Bit);

    /**
     * if this value has a child by this name, it should return a pointer to it
     */
    virtual Value* child(InterpretScope& scope, const std::string& name);

    /**
     * this function returns a function declaration for a member function
     */
    virtual Value* call_member(InterpretScope& scope, const std::string& name, std::vector<Value*>& values);

    /**
     * index operator [] calls this on a value
     */
    virtual Value* index(InterpretScope& scope, int i);

    /**
     * set the child value, with given name, performing operation op
     */
    virtual void set_child_value(const std::string& name, Value* value, Operation op);

    /**
     * this function expects this identifier value to find itself in the parent value given to it
     * this is only expected to be overridden by identifiers
     * @param parent
     * @return
     */
    virtual Value* find_in(InterpretScope& scope, Value* parent);

    /**
     * called on a identifier to set it's value in the given parent
     */
    virtual void set_value_in(InterpretScope& scope, Value* parent, Value* value, Operation op);

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
     * check if this value is a stored pointer
     */
    bool is_stored_pointer();

    /**
     * check if this value is a reference
     */
    bool is_ref();

    /**
     * is this value a function call
     */
    bool is_func_call();

    /**
     * check if this value is a reference and moved
     */
    bool is_ref_moved();

    /**
     * a value that references a struct that is mem copied into arguments
     * however the value is not moved, because it's not movable
     */
    bool requires_memcpy_ref_struct(BaseType* known_type);

#ifdef COMPILER_BUILD

    /**
     * value is supposed to destruct itself, call the destructor, because scope has ended
     */
    virtual void llvm_destruct(Codegen& gen, llvm::Value* allocaInst) {
        // does nothing by default
    }

    /**
     * provide a function type, if this value represents a function
     * like a function pointer or a lambda
     */
    virtual llvm::FunctionType* llvm_func_type(Codegen& gen) {
        return (llvm::FunctionType*) llvm_type(gen);
    }

    /**
     * allocates the given type and stores the given value in it
     * @return returns pointer to alloca instruction
     */
    llvm::AllocaInst* llvm_allocate_with(Codegen& gen, llvm::Value* value, llvm::Type* type);

    /**
     * load the given value, taking in to account structs
     */
    static llvm::Value* load_value(Codegen& gen, BaseType* known_t, llvm::Type* type, llvm::Value* ptr);

    /**
     * load the given value, taking in to account structs
     */
    static llvm::Value* load_value(Codegen& gen, Value* value, llvm::Value* ptr) {
        return load_value(gen, value->known_type(), value->llvm_type(gen), ptr);
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
     * provides llvm_elem_type, which is the child type for example elem type of an array value
     * @param gen
     * @return
     */
    virtual llvm::Type* llvm_elem_type(Codegen& gen);

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
    virtual llvm::Value* llvm_arg_value(Codegen& gen, FunctionCall* call, unsigned int index);

    /**
     * this method is called by return statement to get the return value for this Value
     * if this class defines specific behavior for return, it should override this method
     */
    virtual llvm::Value* llvm_ret_value(Codegen& gen, ReturnStatement* returnStmt);

    /**
     * called by assignment, to assign the current value to left hand side
     */
    virtual llvm::Value* llvm_assign_value(Codegen& gen, llvm::Value* lhsPtr, Value* lhs);

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
    virtual bool add_child_index(Codegen& gen, std::vector<llvm::Value*>& indexes, const std::string& name);

#endif

    /**
     * Called by assignment statement, to set the value of this value
     * since InterpretValue can also represent an identifier or access chain
     * This method is overridden by for ex : an identifier, which can find itself in the scope above and set this value
     * The given op must be performed on the already stored value
     * @param vars
     * @param value
     * @param op
     */
    virtual void set_identifier_value(InterpretScope& scope, Value* rawValue, Operation op);

    /**
     * check if this value is an int n
     */
    virtual bool is_int_n() {
        return false;
    }

    /**
     * is an int num value, this includes referenced values
     */
    bool is_value_int_n() {
        auto type = value_type();
        return type >= ValueType::IntNStart && type <= ValueType::IntNEnd;
    }

    /**
     * return a var init statement that this value corresponds to
     */
    virtual VarInitStatement* declaration() {
        return nullptr;
    }

    /**
     * return true if this value is a reference (VariableIdentifier)
     */
    virtual bool reference() {
        return false;
    }

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
     * This method can only be called on primitive values as they are the only ones that support copy operation
     * @return
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
    virtual Value* scope_value(InterpretScope& scope);

    /**
     * does this value compute the value, in other words (is it an expression -> e.g a + b)
     * @return
     */
    virtual bool computed() {
        return false;
    }

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
     * evaluate the children of this value, if this is called on a function call
     * the arguments of the function call will be
     */
    virtual void evaluate_children(InterpretScope& scope) {

    }

    /**
     * called by access chain, to evaluate this value, in the parent
     */
    virtual Value* evaluated_chain_value(InterpretScope& scope, Value* parent);

    /**
     * just a helper method, to evaluate a value as a boolean
     */
    inline bool evaluated_bool(InterpretScope& scope) {
        return evaluated_value(scope)->as_bool();
    }

    /**
     * return if this is a chain value
     */
    virtual ChainValue* as_chain_value() {
        return nullptr;
    }

    /**
     * a function to be overridden by char values to return actual values
     */
    char as_char();

    /**
     * a function to be overridden by bool values to return actual values
     */
    bool as_bool();

    /**
     * will get the string from the value
     * this usually works on string values
     */
    std::string as_string();

    /**
     * a function to be overridden by values that can return int
     */
    int as_int();

    /**
     * will return a unsigned int representation
     */
    unsigned as_uint();

    /**
     * a function to be overridden by values that can return float
     */
    float as_float();

    /**
     * a function to be overridden by values that can return double
     */
    double as_double();

    /**
     * as string value
     */
    StringValue* as_string_value();

    /**
     * a function to be overridden by number value to return itself
     */
    virtual NumberValue* as_number_val() {
        return nullptr;
    }

    /**
     * check if identifier
     */
    static constexpr inline bool is_identifier(ValueKind k) {
        return k == ValueKind::Identifier;
    }

    /**
     * a function to be overridden by identifier
     */
    VariableIdentifier* as_identifier() {
        return is_identifier(val_kind()) ? (VariableIdentifier*) this : nullptr;
    }

    /**
     * get dereference value
     */
    DereferenceValue* as_deref_value() {
        return val_kind() == ValueKind::DereferenceValue ? (DereferenceValue*) this : nullptr;
    }

    /**
     * a function to be overridden by values that can return struct
     */
    StructValue* as_struct() {
        return val_kind() == ValueKind::StructValue ? (StructValue*) this : nullptr;
    }

    /**
     * a function to be overridden by values that can return array value
     */
    ArrayValue* as_array_value() {
        return val_kind() == ValueKind::ArrayValue ? (ArrayValue*) this : nullptr;
    }

    /**
     * check is function call
     */
    static constexpr inline bool isFunctionCall(ValueKind k) {
        return k == ValueKind::FunctionCall;
    }

    /**
     * return if value is a function call
     */
    FunctionCall* as_func_call() {
        return isFunctionCall(val_kind()) ? (FunctionCall*) this : nullptr;
    }

    /**
     * check if is index operator
     */
    static constexpr inline bool isIndexOperator(ValueKind k) {
        return k == ValueKind::IndexOperator;
    }

    /**
     * return if value is an index operator
     */
    IndexOperator* as_index_op() {
        return isIndexOperator(val_kind()) ? (IndexOperator*) this : nullptr;
    }

    /**
     * return if value is a access chain
     */
    virtual AccessChain* as_access_chain() {
        return nullptr;
    }

    /**
     * return if this is a variant call
     */
    virtual VariantCall* as_variant_call() {
        return nullptr;
    }

    /**
     * returns the type of value
     * @return
     */
    virtual ValueType value_type() const {
        return ValueType::Unknown;
    };

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

};

static_assert(sizeof(Value) <= 8, "Value must always be equal or less than 8 bytes");