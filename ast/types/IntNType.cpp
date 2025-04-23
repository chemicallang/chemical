// Copyright (c) Chemical Language Foundation 2025.

#include "IntNType.h"
#include "IntType.h"
#include "ast/base/InterpretScope.h"
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

const IntType IntType::instance;
const BigIntType BigIntType::instance;
const Int128Type Int128Type::instance;
const ShortType ShortType::instance;
const UBigIntType UBigIntType::instance;
const UInt128Type UInt128Type::instance;
const UIntType UIntType::instance;
const UShortType UShortType::instance;
const LongType LongType::instance;
const ULongType ULongType::instance;
const NullPtrType NullPtrType::instance;

Value *IntType::create(ASTAllocator& allocator, uint64_t value, SourceLocation loc) {
    return new (allocator.allocate<IntValue>()) IntValue(value, loc);
}

Value *CharType::create(ASTAllocator& allocator, uint64_t value, SourceLocation loc) {
    return new (allocator.allocate<CharValue>()) CharValue(value, loc);
}

Value *UCharType::create(ASTAllocator& allocator, uint64_t value, SourceLocation loc) {
    return new (allocator.allocate<UCharValue>()) UCharValue(value, loc);
}

Value *UIntType::create(ASTAllocator& allocator, uint64_t value, SourceLocation loc) {
    return new (allocator.allocate<UIntValue>()) UIntValue(value, loc);
}

Value *ShortType::create(ASTAllocator& allocator, uint64_t value, SourceLocation loc) {
    return new (allocator.allocate<ShortValue>()) ShortValue(value, loc);
}

Value *UShortType::create(ASTAllocator& allocator, uint64_t value, SourceLocation loc) {
    return new (allocator.allocate<UShortValue>()) UShortValue(value, loc);
}

Value *LongType::create(ASTAllocator& allocator, uint64_t value, SourceLocation loc) {
    return new (allocator.allocate<LongValue>()) LongValue(value, loc);
}

Value *ULongType::create(ASTAllocator& allocator, uint64_t value, SourceLocation loc) {
    return new (allocator.allocate<ULongValue>()) ULongValue(value, loc);
}

Value *BigIntType::create(ASTAllocator& allocator, uint64_t value, SourceLocation loc) {
    return new BigIntValue(value, loc);
}

Value *UBigIntType::create(ASTAllocator& allocator, uint64_t value, SourceLocation loc) {
    return new (allocator.allocate<UBigIntValue>()) UBigIntValue(value, loc);
}

Value *Int128Type::create(ASTAllocator& allocator, uint64_t value, SourceLocation loc) {
    // TODO is_neg
    bool is_neg = value < 0;
    return new (allocator.allocate<Int128Value>()) Int128Value(is_neg ? -value : value, is_neg, loc);
}

Value *UInt128Type::create(ASTAllocator& allocator, uint64_t value, SourceLocation loc) {
    // TODO is_neg
    return new (allocator.allocate<UInt128Value>()) UInt128Value(static_cast<uint64_t>(value), (value < 0) ? UINT64_MAX : 0, loc);
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
    switch(kind) {
        case ValueKind::Double:
            return new (scope.allocate<DoubleValue>()) DoubleValue((double) value, location);
        case ValueKind::Float:
            return new (scope.allocate<FloatValue>()) FloatValue((float) value, location);
        case ValueKind::Bool:
            return new (scope.allocate<BoolValue>()) BoolValue((int) value, location);
        default:
            return nullptr;
    }
}

Value* pack_by_kind(InterpretScope& scope, ValueKind kind, uint64_t value, SourceLocation location) {
    switch(kind) {
        case ValueKind::Char:
            return new (scope.allocate<CharValue>()) CharValue((char) value, location);
        case ValueKind::Short:
            return new (scope.allocate<ShortValue>()) ShortValue((short) value, location);
        case ValueKind::Int:
            return new (scope.allocate<IntValue>()) IntValue((int) value, location);
        case ValueKind::Long:
            return new (scope.allocate<LongValue>()) LongValue((long) value, location);
        case ValueKind::BigInt:
            return new (scope.allocate<BigIntValue>()) BigIntValue((long long) value, location);
        case ValueKind::Int128:
            // TODO int128 is_negative is always false
            return new (scope.allocate<Int128Value>()) Int128Value((uint64_t) value, false, location);
        case ValueKind::UChar:
            return new (scope.allocate<UCharValue>()) UCharValue((unsigned char) value, location);
        case ValueKind::UShort:
            return new (scope.allocate<UShortValue>()) UShortValue((unsigned short) value, location);
        case ValueKind::UInt:
            return new (scope.allocate<UIntValue>()) UIntValue((unsigned int) value, location);
        case ValueKind::ULong:
            return new (scope.allocate<ULongValue>()) ULongValue((unsigned long) value, location);
        case ValueKind::UBigInt:
            return new (scope.allocate<UBigIntValue>()) UBigIntValue((unsigned long long) value, location);
        case ValueKind::UInt128:
            return new (scope.allocate<UInt128Value>()) UInt128Value((uint64_t) value, false, location);
        case ValueKind::Bool:
            return new (scope.allocate<BoolValue>()) BoolValue(value, location);
        case ValueKind::NumberValue:
            return new (scope.allocate<NumberValue>()) NumberValue(value, location);
        default:
            return nullptr;
    }
}