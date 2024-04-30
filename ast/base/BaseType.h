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
     * check if given base type is of same type
     */
    virtual bool is_same(BaseType* type) const = 0;

    /**
     * this is the precedence in an expression
     * every type is by default 0, however float is 1 and double is 2
     */
    virtual unsigned int precedence() {
        return 0;
    }

    /**
     * given the value, the type will promote the value
     * for example float or doubles will promote integers
     * it's to note that by default no promotion takes place
     */
    virtual std::unique_ptr<Value> promote(Value* value);

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
    virtual llvm::Type *llvm_type(Codegen &gen) const = 0;\

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

#endif

    virtual ~BaseType() = default;

};