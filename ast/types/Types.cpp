// Copyright (c) Qinetik 2024.

#include "ast/base/Value.h"
#include "ArrayType.h"
#include "AnyType.h"
#include "BoolType.h"
#include "CharType.h"
#include "UCharType.h"
#include "DoubleType.h"
#include "FloatType.h"
#include "PointerType.h"
#include "LinkedType.h"
#include "LinkedValueType.h"
#include "StringType.h"
#include "LiteralType.h"
#include "VoidType.h"
#include "ExpressionType.h"
#include "WrapperType.h"
#include "ast/statements/Typealias.h"

const AnyType AnyType::instance(ZERO_LOC);
const BoolType BoolType::instance(ZERO_LOC);
const CharType CharType::instance(ZERO_LOC);
const DoubleType DoubleType::instance(ZERO_LOC);
const FloatType FloatType::instance(ZERO_LOC);
const StringType StringType::instance(ZERO_LOC);
const UCharType UCharType::instance(ZERO_LOC);
const VoidType VoidType::instance(ZERO_LOC);

bool ArrayType::satisfies(BaseType *pure_type) {
    const auto pure_type_kind = pure_type->kind();
    if(pure_type_kind == BaseTypeKind::String) {
        const auto pure = elem_type->pure_type();
        return pure->kind() == BaseTypeKind::IntN && pure->as_intn_type_unsafe()->num_bits() == 8;
    }
    if(pure_type_kind != BaseTypeKind::Array) return false;
    const auto arr_type = (ArrayType*) pure_type;
    if(has_array_size() && arr_type->has_no_array_size() && get_array_size() != arr_type->get_array_size()) return false;
    // can't get array element type, because array is empty probably and has no type declaration to lean on
    // sometimes it links with the expected type which is the type provided by this array type
    if(!arr_type->elem_type) return true;
    return elem_type->satisfies(arr_type->elem_type);
}

bool StringType::satisfies(BaseType *type) {
    return type->kind() == BaseTypeKind::String;
}

bool ExpressionType::satisfies(BaseType *type) {
    if(isLogicalAnd) {
        return firstType->satisfies(type) && secondType->satisfies(type);
    } else {
        return firstType->satisfies(type) || secondType->satisfies(type);
    }
}

bool LiteralType::satisfies(ASTAllocator& allocator, Value* value, bool assignment) {
    auto result = !value->reference() && underlying->satisfies(allocator, value, false);
    if(result) {
        return true;
    } else {
        auto type = value->create_type(allocator);
        if(type && type->kind() == BaseTypeKind::Literal) {
            return underlying->satisfies(((LiteralType*) type)->underlying);
        }
    }
    return false;
}

bool IntNType::satisfies(BaseType* type) {
    const auto type_kind = type->kind();
    if(type_kind == BaseTypeKind::IntN) {
        const auto intN = (IntNType*) type;
        const auto this_unsigned = is_unsigned();
        const auto other_unsigned = intN->is_unsigned();
        if((this_unsigned && other_unsigned) || (!this_unsigned && !other_unsigned)) {
            return true;
        } else {
            return false;
        }
    }
    return false;
}