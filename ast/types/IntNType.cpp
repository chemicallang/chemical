// Copyright (c) Chemical Language Foundation 2025.

#include "IntNType.h"
#include "IntType.h"
#include "ast/types/BoolType.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/types/ReferenceType.h"
#include "ast/values/IntValue.h"
#include "ast/values/UIntValue.h"
#include "ast/values/ShortValue.h"
#include "ast/values/CharValue.h"
#include "ast/values/UCharValue.h"
#include "ast/values/UShortValue.h"
#include "ast/values/LongValue.h"
#include "ast/values/ULongValue.h"
#include "ast/values/BigIntValue.h"
#include "ast/values/UBigIntValue.h"
#include "ast/values/Int128Value.h"
#include "ast/values/UInt128Value.h"
#include "ast/values/NumberValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/BoolValue.h"
#include "NullPtrType.h"

bool BoolType::satisfies(BaseType *type) {
    switch(type->kind()) {
        case BaseTypeKind::Bool:
            return true;
        case BaseTypeKind::Reference:
            return type->as_reference_type_unsafe()->type->kind() == BaseTypeKind::Bool;
        default:
            return false;
    }
}

IntNType* IntNType::to_signed(ASTAllocator& allocator) {
    switch(IntNKind()) {
        case IntNTypeKind::UChar:
            return new (allocator.allocate<CharType>()) CharType();
        case IntNTypeKind::UShort:
            return new (allocator.allocate<ShortType>()) ShortType();
        case IntNTypeKind::UInt:
            return new (allocator.allocate<IntType>()) IntType();
        case IntNTypeKind::ULong:
            return new (allocator.allocate<LongType>()) LongType();
        case IntNTypeKind::UBigInt:
            return new (allocator.allocate<BigIntType>()) BigIntType();
        case IntNTypeKind::UInt128:
            return new (allocator.allocate<Int128Type>()) Int128Type();
        default:
            return this;
    }
}

bool IntNType::satisfies(BaseType *given) {
    const auto type = given->canonical();
    return type->kind() == BaseTypeKind::IntN && satisfies(type->as_intn_type_unsafe());
}

bool IntNType::satisfies(ASTAllocator &allocator, Value *value, bool assignment) {
    const auto literal = value->isValueIntegerLiteral();
    auto otherType = value->getType();
    if(!otherType) return false;
    if(otherType->kind() == BaseTypeKind::Reference) {
        otherType = otherType->as_reference_type_unsafe()->type->canonical();
    }
    if(literal && otherType->kind() == BaseTypeKind::IntN) {
        return true;
    } else {
        return satisfies(otherType);
    }
}

Value *IntType::create(ASTAllocator& allocator, TypeBuilder& typeBuilder, uint64_t value, SourceLocation loc) {
    return new (allocator.allocate<IntValue>()) IntValue(value, typeBuilder.getIntType(), loc);
}

Value *CharType::create(ASTAllocator& allocator, TypeBuilder& typeBuilder, uint64_t value, SourceLocation loc) {
    return new (allocator.allocate<CharValue>()) CharValue(value, typeBuilder.getCharType(), loc);
}

Value *UCharType::create(ASTAllocator& allocator, TypeBuilder& typeBuilder, uint64_t value, SourceLocation loc) {
    return new (allocator.allocate<UCharValue>()) UCharValue(value, typeBuilder.getUCharType(), loc);
}

Value *UIntType::create(ASTAllocator& allocator, TypeBuilder& typeBuilder, uint64_t value, SourceLocation loc) {
    return new (allocator.allocate<UIntValue>()) UIntValue(value, typeBuilder.getUIntType(), loc);
}

Value *ShortType::create(ASTAllocator& allocator, TypeBuilder& typeBuilder, uint64_t value, SourceLocation loc) {
    return new (allocator.allocate<ShortValue>()) ShortValue(value, typeBuilder.getShortType(), loc);
}

Value *UShortType::create(ASTAllocator& allocator, TypeBuilder& typeBuilder, uint64_t value, SourceLocation loc) {
    return new (allocator.allocate<UShortValue>()) UShortValue(value, typeBuilder.getUShortType(), loc);
}

Value *LongType::create(ASTAllocator& allocator, TypeBuilder& typeBuilder, uint64_t value, SourceLocation loc) {
    return new (allocator.allocate<LongValue>()) LongValue(value, typeBuilder.getLongType(), loc);
}

Value *ULongType::create(ASTAllocator& allocator, TypeBuilder& typeBuilder, uint64_t value, SourceLocation loc) {
    return new (allocator.allocate<ULongValue>()) ULongValue(value, typeBuilder.getULongType(), loc);
}

Value *BigIntType::create(ASTAllocator& allocator, TypeBuilder& typeBuilder, uint64_t value, SourceLocation loc) {
    return new (allocator.allocate<BigIntValue>()) BigIntValue(value, typeBuilder.getBigIntType(), loc);
}

Value *UBigIntType::create(ASTAllocator& allocator, TypeBuilder& typeBuilder, uint64_t value, SourceLocation loc) {
    return new (allocator.allocate<UBigIntValue>()) UBigIntValue(value, typeBuilder.getUBigIntType(), loc);
}

Value *Int128Type::create(ASTAllocator& allocator, TypeBuilder& typeBuilder, uint64_t value, SourceLocation loc) {
    // TODO is_neg
    bool is_neg = value < 0;
    return new (allocator.allocate<Int128Value>()) Int128Value(is_neg ? -value : value, is_neg, typeBuilder.getInt128Type(), loc);
}

Value *UInt128Type::create(ASTAllocator& allocator, TypeBuilder& typeBuilder, uint64_t value, SourceLocation loc) {
    // TODO is_neg
    return new (allocator.allocate<UInt128Value>()) UInt128Value(static_cast<uint64_t>(value), (value < 0) ? UINT64_MAX : 0, typeBuilder.getUInt128Type(), loc);
}

double get_double_value(Value* value, ValueKind k) {
    if(k == ValueKind::Double) {
        return (double) ((DoubleValue*) value)->value;
    } else if(k == ValueKind::Float) {
        return (double) ((FloatValue*) value)->value;
    } else if(k >= ValueKind::IntNStart && k <= ValueKind::IntNEnd) {
        return (double) ((IntNumValue*) value)->get_num_value();
    } else {
        return 0;
    }
}

Value* pack_by_kind(InterpretScope& scope, ValueKind kind, double value, SourceLocation location) {
    auto& typeBuilder = scope.global->typeBuilder;
    switch(kind) {
        case ValueKind::Double:
            return new (scope.allocate<DoubleValue>()) DoubleValue((double) value, typeBuilder.getDoubleType(), location);
        case ValueKind::Float:
            return new (scope.allocate<FloatValue>()) FloatValue((float) value, typeBuilder.getFloatType(), location);
        case ValueKind::Bool:
            return new (scope.allocate<BoolValue>()) BoolValue((int) value, typeBuilder.getBoolType(), location);
        default:
            return nullptr;
    }
}

Value* pack_by_kind(InterpretScope& scope, ValueKind kind, uint64_t value, SourceLocation location) {
    auto& typeBuilder = scope.global->typeBuilder;
    switch(kind) {
        case ValueKind::Char:
            return new (scope.allocate<CharValue>()) CharValue((char) value, typeBuilder.getCharType(), location);
        case ValueKind::Short:
            return new (scope.allocate<ShortValue>()) ShortValue((short) value, typeBuilder.getShortType(), location);
        case ValueKind::Int:
            return new (scope.allocate<IntValue>()) IntValue((int) value, typeBuilder.getIntType(), location);
        case ValueKind::Long:
            return new (scope.allocate<LongValue>()) LongValue((long) value, typeBuilder.getLongType(), location);
        case ValueKind::BigInt:
            return new (scope.allocate<BigIntValue>()) BigIntValue((long long) value, typeBuilder.getBigIntType(), location);
        case ValueKind::Int128:
            // TODO int128 is_negative is always false
            return new (scope.allocate<Int128Value>()) Int128Value((uint64_t) value, false, typeBuilder.getInt128Type(), location);
        case ValueKind::UChar:
            return new (scope.allocate<UCharValue>()) UCharValue((unsigned char) value, typeBuilder.getUCharType(), location);
        case ValueKind::UShort:
            return new (scope.allocate<UShortValue>()) UShortValue((unsigned short) value, typeBuilder.getUShortType(), location);
        case ValueKind::UInt:
            return new (scope.allocate<UIntValue>()) UIntValue((unsigned int) value, typeBuilder.getUIntType(), location);
        case ValueKind::ULong:
            return new (scope.allocate<ULongValue>()) ULongValue((unsigned long) value, typeBuilder.getULongType(), location);
        case ValueKind::UBigInt:
            return new (scope.allocate<UBigIntValue>()) UBigIntValue((unsigned long long) value, typeBuilder.getUBigIntType(), location);
        case ValueKind::UInt128:
            return new (scope.allocate<UInt128Value>()) UInt128Value((uint64_t) value, false, typeBuilder.getUInt128Type(), location);
        case ValueKind::Bool:
            return new (scope.allocate<BoolValue>()) BoolValue(value, typeBuilder.getBoolType(), location);
        case ValueKind::NumberValue:
            return new (scope.allocate<NumberValue>()) NumberValue(value, location);
        default:
            return nullptr;
    }
}