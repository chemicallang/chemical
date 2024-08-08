// Copyright (c) Qinetik 2024.

#pragma once

#include "BaseTypeKind.h"
#include "ValueType.h"
#include <string>
#include <memory>
#include "Visitor.h"
#include "ASTAny.h"
#include "std/hybrid_ptr.h"
#include <iostream>

class Codegen;

class SymbolResolver;

class ASTNode;

class PointerType;

class FunctionType;

class Value;

/**
 * BaseType is a base class for all the types there are
 */
class BaseType : public ASTAny {
public:

    /**
     * default constructor
     */
    BaseType() = default;

    /**
     * deleted copy constructor
     */
    BaseType(const BaseType& other) = delete;

    /**
     * default move constructor
     */
    BaseType(BaseType&& other) = default;

    /**
     * move assignment operator
     */
    BaseType& operator =(BaseType &&other) = default;

    /**
     * get the byte size, of this type
     */
    virtual uint64_t byte_size(bool is64Bit) {
        throw std::runtime_error("byte_size called on base type");
    }

    /**
     * this creates the child type
     */
    virtual std::unique_ptr<BaseType> create_child_type() const {
        return nullptr;
    }

    /**
     * get a pointer to it's child type
     */
    virtual hybrid_ptr<BaseType> get_child_type() {
        throw std::runtime_error("byte_size called on base type");
    }

    /**
     * pure type means, no referenced type, only pure types
     * referenced tyeps created by typealiases return the actual types
     */
    virtual hybrid_ptr<BaseType> get_pure_type() {
        return hybrid_ptr<BaseType> { this, false };
    }

    /**
     * accept the visitor
     */
    virtual void accept(Visitor *visitor) = 0;

    /**
     * copy the type, along with linked node
     */
    virtual BaseType *copy() const = 0;

    /**
     * a type, or a referenced type, can link itself with its definition
     */
    virtual void link(SymbolResolver &linker, std::unique_ptr<BaseType>& current) {
        // does nothing by default
    }

    /**
     * just return the linked node
     */
    virtual ASTNode *linked_node() {
        return nullptr;
    }

    /**
     * return kind of base type
     */
    virtual BaseTypeKind kind() const {
        return BaseTypeKind::Unknown;
    }

    /**
     * return type of value
     */
    virtual ValueType value_type() const {
        return ValueType::Unknown;
    }

    /**
     * if the other value can be promoted, this should return true
     * promotion means changing type to a higher type for expression
     * in an expression firstValue(10) operator(==) secondValue(10)
     *
     * the type of both values should be same, when two values of different type
     * for example a float and an integer are involved, we promote the integer type to a float
     *
     * if one of the values is a referenced value, meaning a variable, a cast is performed
     * a (int) == b (short), we will promote b in this example to an int
     *
     * in the case of 'a' (float / double) == b (int32), we would promote b to a float / double
     *
     * in the case of 'a' (long / ulong / bigint / ubigint) == b (int32), we must cast the a variable to a int32
     * to perform the comparison operation, this will be faster
     *
     * in the case of 'a' (short) == b (int32), as 'a' has less bits than int32
     * if b can fit in the range of a (short), we will demote int32 to a short
     * otherwise we will
     */
    virtual bool can_promote(Value* value) {
        return false;
    }

    /**
     * a promotion always results in a new value creation or none at all
     * promote will only be called if can_promote returns true
     *
     * Refer to can_promote for documentation
     * \see can_promote
     */
    virtual Value* promote(Value* value) {
        return nullptr;
    }

    /**
     * check if given base type is of same type
     */
    virtual bool is_same(BaseType* type) = 0;

    /**
     * check if given type satisfies the this type
     */
    virtual bool satisfies(BaseType* type) {
        return is_same(type);
    }

    /**
     * this basically tells whether the given value type would satisfy this type
     * @deprecated
     */
    virtual bool satisfies(ValueType type) = 0;

    /**
     * will get the generic iteration or -1 if this type can't provide one
     */
    virtual int16_t get_generic_iteration() {
        return -1;
    }

    /**
     * whether the given value satisfies the current type
     */
    virtual bool satisfies(Value* value) {
        throw std::runtime_error("satisfies Value* called on base type");
    }

    /**
     * pointer type returns pointer type
     */
    virtual PointerType *pointer_type() {
        return nullptr;
    }

    /**
     * function type returns function type
     */
    virtual FunctionType *function_type() {
        return nullptr;
    }

    /**
     * representation is representation of the type
     */
    std::string representation();

    /**
     * check if this type is a pointer
     */
    bool is_pointer() {
        return kind() == BaseTypeKind::Pointer || kind() == BaseTypeKind::String;
    }

    /**
     * if this type is linked with a struct definition, it can be retrieved using this function
     */
    StructDefinition* linked_struct_def();

    /**
     * get linked generic struct from this type, if there's any
     */
    StructDefinition* get_generic_struct();

    /**
     * this type references a struct
     */
    bool is_ref_struct();

    /**
     * searches a implicit constructor for given value, using the linked struct with this type
     * otherwise nullptr
     */
    FunctionDeclaration* implicit_constructor_for(Value* value);

    /**
     * if this type supports get_generic_struct
     * this method get's the generic struct and set's the given iteration to it
     * @return -2 if unsuccessful, otherwise previous iteration of the struct
     */
    int16_t set_generic_iteration(int16_t iteration);

#ifdef COMPILER_BUILD

    /**
     * return a func type, if this type can represent one
     */
    virtual llvm::FunctionType* llvm_func_type(Codegen &gen) {
        return (llvm::FunctionType*) llvm_type(gen);
    }

    /**
     * returns this type, but for a llvm parameter
     */
    virtual llvm::Type *llvm_param_type(Codegen &gen) {
        return llvm_type(gen);
    }

#endif

    virtual ~BaseType();

};