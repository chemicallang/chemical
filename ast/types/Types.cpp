// Copyright (c) Chemical Language Foundation 2025.

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
#include "ast/statements/Typealias.h"

const AnyType AnyType::instance;
const BoolType BoolType::instance;
const CharType CharType::instance;
const DoubleType DoubleType::instance;
const FloatType FloatType::instance;
const StringType StringType::instance;
const UCharType UCharType::instance;
const VoidType VoidType::instance;

bool ArrayType::satisfies(BaseType *pure_type) {
    const auto pure_type_kind = pure_type->kind();
    if(pure_type_kind == BaseTypeKind::String) {
        const auto pure = elem_type->canonical();
        return pure->kind() == BaseTypeKind::IntN && pure->as_intn_type_unsafe()->is_char_or_uchar_type();
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
    switch(type->kind()) {
        case BaseTypeKind::String:
            return true;
        case BaseTypeKind::Pointer: {
            const auto child_type = type->as_pointer_type_unsafe()->type;
            switch (child_type->kind()) {
                case BaseTypeKind::IntN:
                    return child_type->as_intn_type_unsafe()->is_char_or_uchar_type();
                default:
                    return false;
            }
        }
        default:
            return false;
    }
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