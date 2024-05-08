// Copyright (c) Qinetik 2024.

#pragma once

#include "ValueType.h"
#include <string>
#include "compiler/Codegen.h"
#include "compiler/SymbolResolver.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmfwd.h"
#include "BaseTypeKind.h"

#endif

class PointerType;

class FunctionType;

class Value;

/**
 * BaseType is a base class for all the types there are
 */
class BaseType {
public:

    /**
     * constructor
     */
    BaseType() {

    }

    /**
     * this creates the child type
     */
    virtual std::unique_ptr<BaseType> create_child_type() const {
        return nullptr;
    }

    /**
     * copy the type, along with linked node
     */
    virtual BaseType *copy() const = 0;

    /**
     * a type, or a referenced type, can link itself with its definition
     */
    virtual void link(SymbolResolver &linker) {
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
    virtual bool is_same(BaseType* type) const = 0;

    /**
     * this basically tells whether the given value type would satisfy this type
     * @param type
     * @return
     */
    virtual bool satisfies(ValueType type) const = 0;

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
     * @return
     */
    virtual std::string representation() const = 0;

#ifdef COMPILER_BUILD

    /**
     * return the llvm type that corresponds to this base type
     * @param gen
     * @return
     */
    virtual llvm::Type *llvm_type(Codegen &gen) const = 0;

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

    /**
     * return this type, but for a struct member
     */
    virtual llvm::Type *llvm_struct_member_type(Codegen &gen) {
        return llvm_type(gen);
    }

    /**
     * type can intercept the return value and modify it
     * @param node is the calling node
     */
    virtual llvm::Value* llvm_return_intercept(Codegen &gen, llvm::Value* value, ASTNode* node) {
        return value;
    }

#endif

    virtual ~BaseType() = default;

};