// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "BaseTypeKind.h"
#include <string>
#include <memory>
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
private:

    /**
     * the type kind
     */
    BaseTypeKind const _kind;

    /**
     * the location of this node
     */
    SourceLocation const _location;

public:

    /**
     * default constructor
     */
    inline explicit constexpr BaseType(BaseTypeKind k, SourceLocation loc) noexcept : _kind(k), _location(loc) {

    }

    /**
     * deleted copy constructor
     */
    BaseType(const BaseType& other) = delete;

    /**
     * default move constructor
     */
    BaseType(BaseType&& other) = default;

    /**
     * the type kind
     */
    inline BaseTypeKind kind() const noexcept {
        return _kind;
    }

    /**
     * get the encoded location
     */
    inline SourceLocation encoded_location() const noexcept {
        return _location;
    }

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
     * @deprecated
     */
    [[deprecated]]
    BaseType* pure_type();

    /**
     * important to note that this type doesn't canonicalize deeply
     * a pointer to a MyInt remains a pointer to MyInt however MyInt
     * becomes an int, it's only direct canonicalization
     */
    BaseType* canonical();

    /**
     * get pure type, this is the method to use
     */
    BaseType* deep_canonical(ASTAllocator& allocator);

    /**
     * pure type is an alias for deep canonical type
     */
    BaseType* pure_type(ASTAllocator& allocator) {
        return deep_canonical(allocator);
    }

    /**
     * copy the type, along with linked node
     * this copy function performs a deep copy, so it should be used with care
     */
    virtual BaseType *copy(ASTAllocator& allocator) const = 0;

    /**
     * is this type a reference to the given node
     */
    bool is_reference_to(ASTNode* node);

    /**
     * get loadable referred to type
     */
    BaseType* getLoadableReferredType();

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
     * check if this is a struct like type
     * any type stored internally as a struct in llvm / c backends, for example dynamic type is a struct like type
     * similarly generic and union are also struct like
     */
    bool isStructLikeType();

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
     * will get the generic iteration or -1 if this type can't provide one
     */
    int16_t get_generic_iteration();

    /**
     * whether the given value satisfies the current type
     */
    virtual bool satisfies(ASTAllocator& allocator, Value* value, bool assignment);

    /**
     * if this value can be auto dereferenced, the type of dereferenced value is returned
     * the expected type should be a reference for that to happen, otherwise a null pointer is
     * returned
     */
    BaseType* getAutoDerefType(BaseType* expected_type);

    /**
     * get de-referenced type, &int -> int
     */
    BaseType* removeReferenceFromType(ASTAllocator& allocator);

    /**
     * representation is representation of the type as a string
     * however this method maybe slow, use with care
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
    inline bool is_pointer() {
        return is_pointer(kind());
    }

    /**
     * check if this type is a pointer or ref
     */
    bool is_pointer_or_ref() {
        return is_pointer_or_ref(kind());
    }

    /**
     * check if this kind is a reference type
     */
    inline bool is_reference() {
        return kind() == BaseTypeKind::Reference;
    }

    /**
     * will make the type mutable, completely pointer type, linked type, generic type
     * everything inside will become mutable, as if you wrote mut keyword before the type
     */
    bool make_mutable();

    /**
     * check if this type is mutable
     */
    bool is_mutable();

    /**
     * get members container
     */
    MembersContainer* get_members_container();

    /**
     * get the default constructor for the type, if it exists
     */
    FunctionDeclaration* get_def_constructor();

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
    ASTNode* get_direct_linked_node();

    /**
     * get direct or referenced linked node, type should be Struct or Struct&
     */
    ASTNode* get_ref_or_linked_node();

    /**
     * if this is a directly referenced / generic type, get it's ref'ed node
     */
    StructDefinition* get_direct_linked_struct();

    /**
     * get direct linked interface from the following type
     */
    InterfaceDefinition* get_direct_linked_interface();

    /**
     * get direct or referenced struct, this means either the type should be Struct or Struct&
     */
    StructDefinition* get_ref_or_linked_struct();

    /**
     * if this is a directly referenced / generic variant, get it
     */
    VariantDefinition* get_direct_linked_variant();

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
    bool requires_moving();

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

    static inline constexpr bool isIntFloatOrBool(BaseTypeKind k) {
        return k == BaseTypeKind::IntN || k == BaseTypeKind::Bool || k == BaseTypeKind::Double || k == BaseTypeKind::Float;
    }

    static inline constexpr bool isPrimitiveType(BaseTypeKind k) {
        return isIntFloatOrBool(k) || k == BaseTypeKind::Pointer;
    }

    static inline constexpr bool isLoadableReferencee(BaseTypeKind k) {
        return isPrimitiveType(k) || k == BaseTypeKind::Function || k == BaseTypeKind::Literal;
    }

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

    static inline constexpr bool isExprType(BaseTypeKind k) {
        return k == BaseTypeKind::ExpressionType;
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

    inline ExpressionType* as_expr_type() {
        return isExprType(kind()) ? (ExpressionType*) this : nullptr;
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

    inline ExpressionType* as_expr_type_unsafe() {
        return (ExpressionType*) this;
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