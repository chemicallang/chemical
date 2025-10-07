// Copyright (c) Chemical Language Foundation 2025.

#include "ast/base/Value.h"
#include "ArrayType.h"
#include "AnyType.h"
#include "BoolType.h"
#include "DoubleType.h"
#include "FloatType.h"
#include "PointerType.h"
#include "LinkedType.h"
#include "IntNType.h"
#include "LinkedValueType.h"
#include "StringType.h"
#include "LiteralType.h"
#include "VoidType.h"
#include "ExpressionType.h"
#include "ast/statements/Typealias.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/AccessChain.h"
#include "ast/values/Negative.h"
#include "ast/values/NotValue.h"
#include "MaybeRuntimeType.h"
#include "RuntimeType.h"

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

bool LiteralType::satisfies(Value* value, bool assignment) {
    auto result = !value->reference() && underlying->satisfies(value, false);
    if(result) {
        return true;
    } else {
        auto type = value->getType();
        if(type && type->kind() == BaseTypeKind::Literal) {
            return underlying->satisfies(((LiteralType*) type)->underlying);
        }
    }
    return false;
}



bool MaybeRuntimeType::satisfies(Value* value, bool assignment) {
    return underlying->satisfies(value, assignment);
}

bool RuntimeType::satisfies(Value* value, bool assignment) {
    const auto value_type = value->getType();
    // check if value is a literal, comptime or runtime type
    switch(value_type->kind()) {
        case BaseTypeKind::Literal:
            return false;
        case BaseTypeKind::Runtime:
            return underlying->satisfies(value_type);
        default:
            break;
    }
    // will do
    return underlying->satisfies(value_type);
}