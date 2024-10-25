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

Value *IntType::create(int64_t value) {
    return new IntValue(value, ZERO_LOC);
}

Value *CharType::create(int64_t value) {
    return new CharValue(value, location);
}

Value *UCharType::create(int64_t value) {
    return new UCharValue(value, location);
}

Value *UIntType::create(int64_t value) {
    return new UIntValue(value, location);
}

Value *ShortType::create(int64_t value) {
    return new ShortValue(value, location);
}

Value *UShortType::create(int64_t value) {
    return new UShortValue(value, location);
}

Value *LongType::create(int64_t value) {
    return new LongValue(value, num_bits() == 64, location);
}

Value *ULongType::create(int64_t value) {
    return new ULongValue(value, num_bits() == 64, location);
}

Value *BigIntType::create(int64_t value) {
    return new BigIntValue(value, location);
}

Value *UBigIntType::create(int64_t value) {
    return new UBigIntValue(value, location);
}

Value *Int128Type::create(int64_t value) {
    bool is_neg = value < 0;
    return new Int128Value(is_neg ? -value : value, is_neg, location);
}

Value *UInt128Type::create(int64_t value) {
    return new UInt128Value(static_cast<uint64_t>(value), (value < 0) ? UINT64_MAX : 0, location);
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

Value* pack_by_kind(InterpretScope& scope, ValueKind kind, double value) {
    switch(kind) {
        case ValueKind::Double:
            return new (scope.allocate<DoubleValue>()) DoubleValue((double) value, ZERO_LOC);
        case ValueKind::Float:
            return new (scope.allocate<FloatValue>()) FloatValue((float) value, ZERO_LOC);
        case ValueKind::Bool:
            return new (scope.allocate<BoolValue>()) BoolValue((int) value, ZERO_LOC);
        default:
            return nullptr;
    }
}

Value* pack_by_kind(InterpretScope& scope, ValueKind kind, int64_t value) {
    switch(kind) {
        case ValueKind::Char:
            return new (scope.allocate<CharValue>()) CharValue((char) value, ZERO_LOC);
        case ValueKind::Short:
            return new (scope.allocate<ShortValue>()) ShortValue((short) value, ZERO_LOC);
        case ValueKind::Int:
            return new (scope.allocate<IntValue>()) IntValue((int) value, ZERO_LOC);
        case ValueKind::Long:
            return new (scope.allocate<LongValue>()) LongValue((long) value, isExe64Bit(), ZERO_LOC);
        case ValueKind::BigInt:
            return new (scope.allocate<BigIntValue>()) BigIntValue((long long) value, ZERO_LOC);
        case ValueKind::Int128:
            // TODO int128 is_negative is always false
            return new (scope.allocate<Int128Value>()) Int128Value((uint64_t) value, false, ZERO_LOC);
        case ValueKind::UChar:
            return new (scope.allocate<UCharValue>()) UCharValue((char) value, ZERO_LOC);
        case ValueKind::UShort:
            return new (scope.allocate<UShortValue>()) UShortValue((short) value, ZERO_LOC);
        case ValueKind::UInt:
            return new (scope.allocate<UIntValue>()) UIntValue((int) value, ZERO_LOC);
        case ValueKind::ULong:
            return new (scope.allocate<ULongValue>()) ULongValue((long) value, isExe64Bit(), ZERO_LOC);
        case ValueKind::UBigInt:
            return new (scope.allocate<UBigIntValue>()) UBigIntValue((long long) value, ZERO_LOC);
        case ValueKind::UInt128:
            return new (scope.allocate<UInt128Value>()) UInt128Value((uint64_t) value, false, ZERO_LOC);
        case ValueKind::Bool:
            return new (scope.allocate<BoolValue>()) BoolValue(value, ZERO_LOC);
        case ValueKind::NumberValue:
            return new (scope.allocate<NumberValue>()) NumberValue(value, ZERO_LOC);
        default:
            return nullptr;
    }
}