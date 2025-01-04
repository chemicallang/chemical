// Copyright (c) Qinetik 2024.

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

const IntType IntType::instance(ZERO_LOC);
const BigIntType BigIntType::instance(ZERO_LOC);
const Int128Type Int128Type::instance(ZERO_LOC);
const ShortType ShortType::instance(ZERO_LOC);
const UBigIntType UBigIntType::instance(ZERO_LOC);
const UInt128Type UInt128Type::instance(ZERO_LOC);
const UIntType UIntType::instance(ZERO_LOC);
const UShortType UShortType::instance(ZERO_LOC);
const LongType LongType::instance64Bit(true, ZERO_LOC);
const LongType LongType::instance32Bit(false, ZERO_LOC);
const ULongType ULongType::instance64Bit(true, ZERO_LOC);
const ULongType ULongType::instance32Bit(false, ZERO_LOC);

Value *IntType::create(ASTAllocator& allocator, int64_t value) {
    return new (allocator.allocate<IntValue>()) IntValue(value, ZERO_LOC);
}

Value *CharType::create(ASTAllocator& allocator, int64_t value) {
    return new (allocator.allocate<CharValue>()) CharValue(value, location);
}

Value *UCharType::create(ASTAllocator& allocator, int64_t value) {
    return new (allocator.allocate<UCharValue>()) UCharValue(value, location);
}

Value *UIntType::create(ASTAllocator& allocator, int64_t value) {
    return new (allocator.allocate<UIntValue>()) UIntValue(value, location);
}

Value *ShortType::create(ASTAllocator& allocator, int64_t value) {
    return new (allocator.allocate<ShortValue>()) ShortValue(value, location);
}

Value *UShortType::create(ASTAllocator& allocator, int64_t value) {
    return new (allocator.allocate<UShortValue>()) UShortValue(value, location);
}

Value *LongType::create(ASTAllocator& allocator, int64_t value) {
    return new (allocator.allocate<LongValue>()) LongValue(value, num_bits() == 64, location);
}

Value *ULongType::create(ASTAllocator& allocator, int64_t value) {
    return new (allocator.allocate<ULongValue>()) ULongValue(value, num_bits() == 64, location);
}

Value *BigIntType::create(ASTAllocator& allocator, int64_t value) {
    return new BigIntValue(value, location);
}

Value *UBigIntType::create(ASTAllocator& allocator, int64_t value) {
    return new (allocator.allocate<UBigIntValue>()) UBigIntValue(value, location);
}

Value *Int128Type::create(ASTAllocator& allocator, int64_t value) {
    bool is_neg = value < 0;
    return new (allocator.allocate<Int128Value>()) Int128Value(is_neg ? -value : value, is_neg, location);
}

Value *UInt128Type::create(ASTAllocator& allocator, int64_t value) {
    return new (allocator.allocate<UInt128Value>()) UInt128Value(static_cast<uint64_t>(value), (value < 0) ? UINT64_MAX : 0, location);
}

constexpr inline bool isExe64Bit() {
    return sizeof(void*) == 8;
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

Value* pack_by_kind(InterpretScope& scope, ValueKind kind, int64_t value, SourceLocation location) {
    switch(kind) {
        case ValueKind::Char:
            return new (scope.allocate<CharValue>()) CharValue((char) value, location);
        case ValueKind::Short:
            return new (scope.allocate<ShortValue>()) ShortValue((short) value, location);
        case ValueKind::Int:
            return new (scope.allocate<IntValue>()) IntValue((int) value, location);
        case ValueKind::Long:
            return new (scope.allocate<LongValue>()) LongValue((long) value, isExe64Bit(), location);
        case ValueKind::BigInt:
            return new (scope.allocate<BigIntValue>()) BigIntValue((long long) value, location);
        case ValueKind::Int128:
            // TODO int128 is_negative is always false
            return new (scope.allocate<Int128Value>()) Int128Value((uint64_t) value, false, location);
        case ValueKind::UChar:
            return new (scope.allocate<UCharValue>()) UCharValue((char) value, location);
        case ValueKind::UShort:
            return new (scope.allocate<UShortValue>()) UShortValue((short) value, location);
        case ValueKind::UInt:
            return new (scope.allocate<UIntValue>()) UIntValue((int) value, location);
        case ValueKind::ULong:
            return new (scope.allocate<ULongValue>()) ULongValue((long) value, isExe64Bit(), location);
        case ValueKind::UBigInt:
            return new (scope.allocate<UBigIntValue>()) UBigIntValue((long long) value, location);
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