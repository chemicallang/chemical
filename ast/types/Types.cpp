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
#include "ast/statements/Typealias.h"

const AnyType AnyType::instance(nullptr);
const BoolType BoolType::instance(nullptr);
const CharType CharType::instance(nullptr);
const DoubleType DoubleType::instance(nullptr);
const FloatType FloatType::instance(nullptr);
const StringType StringType::instance(nullptr);
const UCharType UCharType::instance(nullptr);
const VoidType VoidType::instance(nullptr);

bool ArrayType::satisfies(BaseType *pure_type) {
    const auto pure_type_kind = pure_type->kind();
    if(pure_type_kind == BaseTypeKind::String) {
        const auto pure = elem_type->pure_type();
        return pure->kind() == BaseTypeKind::Char;
    }
    if(pure_type_kind != BaseTypeKind::Array) return false;
    const auto arr_type = (ArrayType*) pure_type;
    if(array_size != -1 && arr_type->array_size != -1 && array_size != arr_type->array_size) return false;
    // can't get array element type, because array is empty probably and has no type declaration to lean on
    // sometimes it links with the expected type which is the type provided by this array type
    if(!arr_type->elem_type) return true;
    return elem_type->satisfies(arr_type->elem_type);
}

bool StringType::satisfies(BaseType *type) {
    return type->kind() == BaseTypeKind::String;
}

bool LiteralType::satisfies(ASTAllocator& allocator, Value* value) {
    return !value->reference() && underlying->satisfies(allocator, value);
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