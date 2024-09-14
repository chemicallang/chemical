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
     * any kind of 'type' is returned
     */
    ASTAnyKind any_kind() override {
        return ASTAnyKind::Type;
    }

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
        throw std::runtime_error("BaseType::get_child_type called on base type");
    }

    /**
     * get known child type
     */
    virtual BaseType* known_child_type() {
        return nullptr;
    }

    /**
     * get pure type from this type, if referenced, it will resolve it
     */
    virtual BaseType* pure_type() {
        return this;
    }

    /**
     * copy the type, along with linked node
     */
    virtual BaseType *copy() const = 0;

    /**
     * copies unique
     */
    std::unique_ptr<BaseType> copy_unique() {
        return std::unique_ptr<BaseType>(copy());
    }

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
     * a helper function
     */
    std::unique_ptr<Value> promote_unique(Value* value);

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
     * reference name
     */
    std::string& linked_name();

    /**
     * check if this type is a pointer
     */
    bool is_pointer() {
        return kind() == BaseTypeKind::Pointer || kind() == BaseTypeKind::String;
    }

    /**
     * check if this represents a reference
     */
    bool is_reference(BaseTypeKind k);

    /**
     * helper function
     */
    inline bool is_reference() {
        return is_reference(kind());
    }

    /**
     * is destructor required
     */
    bool requires_destructor();

    /**
     * is a move function required
     */
    bool requires_clear_fn();

    /**
     * is a copy function required
     */
    bool requires_copy_fn();

    /**
     * if this is a directly referenced / generic type, get it's ref'ed node
     */
    ASTNode* get_direct_linked_node(BaseTypeKind kind);

    /**
     * get direct or referenced linked node, type should be Struct or Struct&
     */
    ASTNode* get_ref_or_linked_node(BaseTypeKind kind);

    /**
     * a helper function
     */
    inline ASTNode* get_direct_linked_node() {
        return get_direct_linked_node(kind());
    }

    /**
     * if this is a directly referenced / generic type, get it's ref'ed node
     */
    StructDefinition* get_direct_linked_struct(BaseTypeKind k);

    /**
     * get direct or referenced struct, this means either the type should be Struct or Struct&
     */
    StructDefinition* get_ref_or_linked_struct(BaseTypeKind k);

    /**
     * a helper function
     */
    inline StructDefinition* get_direct_linked_struct() {
        return get_direct_linked_struct(kind());
    }

    /**
     * if this is a directly referenced / generic variant, get it
     */
    VariantDefinition* get_direct_linked_variant(BaseTypeKind k);

    /**
     * a helper function
     */
    inline VariantDefinition* get_direct_linked_variant() {
        return get_direct_linked_variant(kind());
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
     * get linked interface definition
     */
    InterfaceDefinition* linked_interface_def();

    /**
     * get linked generic interface definition
     */
    InterfaceDefinition* get_generic_interface();

    /**
     * get linked interface definition if it's dynamic
     */
    InterfaceDefinition* linked_dyn_interface();

    /**
     * a movable directly referenced struct is returned otherwise nullptr
     * here movable means that the struct has a destructor or a move function or both
     * which means, it just can't be mem copied, after copied, move functions should be called
     * moves should be tracked and all that
     */
    StructDefinition* get_direct_linked_movable_struct();

    /**
     * this type references a struct
     */
    inline bool is_linked_struct() {
        return get_direct_linked_struct() != nullptr;
    }

    /**
     * a helper function
     */
    inline bool is_movable_linked_struct() {
        return get_direct_linked_movable_struct() != nullptr;
    }

    /**
     * check if linked is a movable type
     */
    bool requires_moving(BaseTypeKind k);

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
     * return clang type for the current type
     */
    virtual clang::QualType clang_type(clang::ASTContext &context);

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

// just a helper class that takes care about cst token
class TokenizedBaseType : public BaseType {
public:
    CSTToken* token;
    explicit TokenizedBaseType(CSTToken* token) : token(token) {}
    CSTToken* cst_token() override {
        return token;
    }
};