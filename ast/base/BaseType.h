// Copyright (c) Qinetik 2024.

#pragma once

#include "BaseTypeKind.h"
#include "ValueType.h"
#include <string>
#include <memory>
#include "Visitor.h"
#include "ASTAny.h"
#include "ASTAllocator.h"
#include "std/hybrid_ptr.h"
#include "std/chem_string_view.h"
#include <iostream>

class Codegen;

class SymbolResolver;

class ASTNode;

class PointerType;

class FunctionType;

class Value;

class MembersContainer;

class IntNType;

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
    ASTAnyKind any_kind() final {
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
    virtual BaseType* create_child_type(ASTAllocator& allocator) const {
        return nullptr;
    }

    /**
     * get the type alignment
     */
    unsigned type_alignment(bool is64Bit);

//    /**
//     * get a pointer to it's child type
//     */
//    virtual hybrid_ptr<BaseType> get_child_type() {
//        throw std::runtime_error("BaseType::get_child_type called on base type");
//    }

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
    virtual BaseType *copy(ASTAllocator& allocator) const = 0;

    /**
     * is this type a reference to the given node
     */
    bool is_reference_to(ASTNode* node, BaseTypeKind k);

    /**
     * a type, or a referenced type, can link itself with its definition
     */
    virtual bool link(SymbolResolver &linker) {
        return true;
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
    virtual Value* promote(ASTAllocator& allocator, Value* value) {
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
    virtual bool satisfies(ASTAllocator& allocator, Value* value, bool assignment);

    /**
     * pointer type returns pointer type
     */
    PointerType *pointer_type(BaseTypeKind k);

    /**
     * a helper inline function
     */
    inline PointerType* pointer_type() {
        return pointer_type(kind());
    }

    /**
     * function type returns function type
     */
    FunctionType *function_type(BaseTypeKind k);

    /**
     * a helper inline function
     */
    FunctionType *function_type() {
        return function_type(kind());
    }

    /**
     * representation is representation of the type
     */
    std::string representation();

    /**
     * reference name
     */
    chem::string_view& linked_name();

    /**
     * check if this kind is a pointer
     */
    static inline bool is_pointer(BaseTypeKind k) {
        return k == BaseTypeKind::Pointer || k == BaseTypeKind::String;
    }

    /**
     * check if this kind is a pointer
     */
    static inline bool is_pointer_or_ref(BaseTypeKind k) {
        return k == BaseTypeKind::Reference || is_pointer(k);
    }

    /**
     * check if this type is a pointer
     */
    bool is_pointer() {
        return is_pointer(kind());
    }

    /**
     * check if this type is a pointer or ref
     */
    bool is_pointer_or_ref() {
        return is_pointer_or_ref(kind());
    }

    /**
     * will make the type mutable, completely pointer type, linked type, generic type
     * everything inside will become mutable, as if you wrote mut keyword before the type
     */
    bool make_mutable(BaseTypeKind k);

    /**
     * check if this type is mutable
     */
    bool is_mutable(BaseTypeKind k);

    /**
     * check if this represents a reference
     */
    bool is_reference(BaseTypeKind k) {
        return k == BaseTypeKind::Reference;
    }

    /**
     * helper function
     */
    inline bool is_reference() {
        return is_reference(kind());
    }

    /**
     * get members container
     */
    MembersContainer* get_members_container();

    /**
     * does this type has a destructor
     */
    FunctionDeclaration* get_destructor();

    /**
     * does this type has a pre move function
     */
    FunctionDeclaration* get_pre_move_fn();

    /**
     * does this type has a move function
     */
    FunctionDeclaration* get_move_fn();

    /**
     * does this type has a clear function
     */
    FunctionDeclaration* get_clear_fn();

    /**
     * does thi type has a copy function
     */
    FunctionDeclaration* get_copy_fn();

    /**
     * is destructor required
     */
    bool requires_destructor();

    /**
     * is move required
     */
    bool requires_move_fn();

    /**
     * is a clear function required
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
     * get direct linked interface from the following type
     */
    InterfaceDefinition* get_direct_linked_interface(BaseTypeKind k);

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
     * get linked struct that is non movable (meaning has no destructor or move fn or implicit copy)
     */
    StructDefinition* get_direct_non_movable_struct();

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
    FunctionDeclaration* implicit_constructor_for(ASTAllocator& allocator, Value* value);

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
     * returns this type, but for a llvm parameter
     */
    virtual llvm::Type *llvm_param_type(Codegen &gen) {
        return llvm_type(gen);
    }

#endif

    virtual ~BaseType();

    //---------------------------------------------
    // Helper is methods
    //---------------------------------------------

    static inline constexpr bool isAnyType(BaseTypeKind k) {
        return k == BaseTypeKind::Any;
    }

    static inline constexpr bool isArrayType(BaseTypeKind k) {
        return k == BaseTypeKind::Array;
    }

    static inline constexpr bool isStructType(BaseTypeKind k) {
        return k == BaseTypeKind::Struct;
    }

    static inline constexpr bool isUnionType(BaseTypeKind k) {
        return k == BaseTypeKind::Union;
    }

    static inline constexpr bool isBoolType(BaseTypeKind k) {
        return k == BaseTypeKind::Bool;
    }

    static inline constexpr bool isDoubleType(BaseTypeKind k) {
        return k == BaseTypeKind::Double;
    }

    static inline constexpr bool isFloatType(BaseTypeKind k) {
        return k == BaseTypeKind::Float;
    }

    static inline constexpr bool isLongDoubleType(BaseTypeKind k) {
        return k == BaseTypeKind::LongDouble;
    }

    static inline constexpr bool isComplexType(BaseTypeKind k) {
        return k == BaseTypeKind::Complex;
    }

    static inline constexpr bool isFloat128Type(BaseTypeKind k) {
        return k == BaseTypeKind::Float128;
    }

    static inline constexpr bool isFunctionType(BaseTypeKind k) {
        return k == BaseTypeKind::Function;
    }

    static inline constexpr bool isGenericType(BaseTypeKind k) {
        return k == BaseTypeKind::Generic;
    }

    static inline constexpr bool isIntNType(BaseTypeKind k) {
        return k == BaseTypeKind::IntN;
    }

    static inline constexpr bool isPointerType(BaseTypeKind k) {
        return k == BaseTypeKind::Pointer;
    }

    static inline constexpr bool isReferenceType(BaseTypeKind k) {
        return k == BaseTypeKind::Reference;
    }

    static inline constexpr bool isLinkedType(BaseTypeKind k) {
        return k == BaseTypeKind::Linked;
    }

    static inline constexpr bool isStringType(BaseTypeKind k) {
        return k == BaseTypeKind::String;
    }

    static inline constexpr bool isLiteralType(BaseTypeKind k) {
        return k == BaseTypeKind::Literal;
    }

    static inline constexpr bool isDynamicType(BaseTypeKind k) {
        return k == BaseTypeKind::Dynamic;
    }

    static inline constexpr bool isVoidType(BaseTypeKind k) {
        return k == BaseTypeKind::Void;
    }

    //---------------------------------------------
    // Helper as methods
    //---------------------------------------------

    inline AnyType* as_any_type() {
        return isAnyType(kind()) ? (AnyType*) this : nullptr;
    }

    inline ArrayType* as_array_type() {
        return isArrayType(kind()) ? (ArrayType*) this : nullptr;
    }

    inline StructType* as_struct_type() {
        return isStructType(kind()) ? (StructType*) this : nullptr;
    }

    inline UnionType* as_union_type() {
        return isUnionType(kind()) ? (UnionType*) this : nullptr;
    }

    inline BoolType* as_bool_type() {
        return isBoolType(kind()) ? (BoolType*) this : nullptr;
    }

    inline DoubleType* as_double_type() {
        return isDoubleType(kind()) ? (DoubleType*) this : nullptr;
    }

    inline FloatType* as_float_type() {
        return isFloatType(kind()) ? (FloatType*) this : nullptr;
    }

    inline LongDoubleType* as_longdouble_type() {
        return isLongDoubleType(kind()) ? (LongDoubleType*) this : nullptr;
    }

    inline ComplexType* as_complex_type() {
        return isComplexType(kind()) ? (ComplexType*) this : nullptr;
    }

    inline Float128Type* as_float128_type() {
        return isFloat128Type(kind()) ? (Float128Type*) this : nullptr;
    }

    inline FunctionType* as_function_type() {
        return isFunctionType(kind()) ? (FunctionType*) this : nullptr;
    }

    inline GenericType* as_generic_type() {
        return isGenericType(kind()) ? (GenericType*) this : nullptr;
    }

    inline IntNType* as_intn_type() {
        return isIntNType(kind()) ? (IntNType*) this : nullptr;
    }

    inline PointerType* as_pointer_type() {
        return isPointerType(kind()) ? (PointerType*) this : nullptr;
    }

    inline ReferenceType* as_reference_type() {
        return isReferenceType(kind()) ? (ReferenceType*) this : nullptr;
    }

    inline LinkedType* as_linked_type() {
        return isLinkedType(kind()) ? (LinkedType*) this : nullptr;
    }

    inline StringType* as_string_type() {
        return isStringType(kind()) ? (StringType*) this : nullptr;
    }

    inline LiteralType* as_literal_type() {
        return isLiteralType(kind()) ? (LiteralType*) this : nullptr;
    }

    inline DynamicType* as_dynamic_type() {
        return isDynamicType(kind()) ? (DynamicType*) this : nullptr;
    }

    inline VoidType* as_void_type() {
        return isVoidType(kind()) ? (VoidType*) this : nullptr;
    }

    //---------------------------------------------
    // Helper as (unsafe) methods
    //---------------------------------------------

    inline AnyType* as_any_type_unsafe() {
        return (AnyType*) this;
    }

    inline ArrayType* as_array_type_unsafe() {
        return (ArrayType*) this;
    }

    inline StructType* as_struct_type_unsafe() {
        return (StructType*) this;
    }

    inline UnionType* as_union_type_unsafe() {
        return (UnionType*) this;
    }

    inline BoolType* as_bool_type_unsafe() {
        return (BoolType*) this;
    }

    inline CharType* as_char_type_unsafe() {
        return (CharType*) this;
    }

    inline UCharType* as_uchar_type_unsafe() {
        return (UCharType*) this;
    }

    inline DoubleType* as_double_type_unsafe() {
        return (DoubleType*) this;
    }

    inline FloatType* as_float_type_unsafe() {
        return (FloatType*) this;
    }

    inline LongDoubleType* as_longdouble_type_unsafe() {
        return (LongDoubleType*) this;
    }

    inline ComplexType* as_complex_type_unsafe() {
        return (ComplexType*) this;
    }

    inline Float128Type* as_float128_type_unsafe() {
        return (Float128Type*) this;
    }

    inline FunctionType* as_function_type_unsafe() {
        return (FunctionType*) this;
    }

    inline GenericType* as_generic_type_unsafe() {
        return (GenericType*) this;
    }

    inline IntNType* as_intn_type_unsafe() {
        return (IntNType*) this;
    }

    inline PointerType* as_pointer_type_unsafe() {
        return (PointerType*) this;
    }

    inline ReferenceType* as_reference_type_unsafe() {
        return (ReferenceType*) this;
    }

    inline LinkedType* as_linked_type_unsafe() {
        return (LinkedType*) this;
    }

    inline StringType* as_string_type_unsafe() {
        return (StringType*) this;
    }

    inline LiteralType* as_literal_type_unsafe() {
        return (LiteralType*) this;
    }

    inline DynamicType* as_dynamic_type_unsafe() {
        return (DynamicType*) this;
    }

    inline VoidType* as_void_type_unsafe() {
        return (VoidType*) this;
    }


};

// just a helper class that takes care about cst token
class TokenizedBaseType : public BaseType {
public:
    SourceLocation location;
    constexpr explicit TokenizedBaseType(SourceLocation location) : location(location) {}
    SourceLocation encoded_location() override {
        return location;
    }
};

static_assert(sizeof(BaseType) <= 8, "BaseType must always be equal or less than 8 bytes");
static_assert(sizeof(TokenizedBaseType) <= 16, "TokenizedBaseType must always be equal or less than 8 bytes");