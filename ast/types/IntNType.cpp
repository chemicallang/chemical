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
#include "ast/values/BoolVAlue.h"

const IntType IntType::instance(nullptr);
const BigIntType BigIntType::instance(nullptr);
const Int128Type Int128Type::instance(nullptr);
const ShortType ShortType::instance(nullptr);
const UBigIntType UBigIntType::instance(nullptr);
const UInt128Type UInt128Type::instance(nullptr);
const UIntType UIntType::instance(nullptr);
const UShortType UShortType::instance(nullptr);
const LongType LongType::instance64Bit(true, nullptr);
const LongType LongType::instance32Bit(false, nullptr);
const ULongType ULongType::instance64Bit(true, nullptr);
const ULongType ULongType::instance32Bit(false, nullptr);

Value *IntType::create(int64_t value) {
    return new IntValue(value, nullptr);
}

Value *CharType::create(int64_t value) {
    return new CharValue(value, nullptr);
}

Value *UCharType::create(int64_t value) {
    return new UCharValue(value, nullptr);
}

Value *UIntType::create(int64_t value) {
    return new UIntValue(value, nullptr);
}

Value *ShortType::create(int64_t value) {
    return new ShortValue(value, nullptr);
}

Value *UShortType::create(int64_t value) {
    return new UShortValue(value, nullptr);
}

Value *LongType::create(int64_t value) {
    return new LongValue(value, num_bits() == 64, nullptr);
}

Value *ULongType::create(int64_t value) {
    return new ULongValue(value, num_bits() == 64, nullptr);
}

Value *BigIntType::create(int64_t value) {
    return new BigIntValue(value, nullptr);
}

Value *UBigIntType::create(int64_t value) {
    return new UBigIntValue(value, nullptr);
}

Value *Int128Type::create(int64_t value) {
    bool is_neg = value < 0;
    return new Int128Value(is_neg ? -value : value, is_neg, nullptr);
}

Value *UInt128Type::create(int64_t value) {
    return new UInt128Value(static_cast<uint64_t>(value), (value < 0) ? UINT64_MAX : 0, nullptr);
}

constexpr inline bool isExe64Bit() {
    return sizeof(void*) == 8;
}

Value* pack_by_kind(InterpretScope& scope, ValueKind kind, int64_t value) {
    switch(kind) {
        case ValueKind::Char:
            return new (scope.allocate<CharValue>()) CharValue((char) value, nullptr);
        case ValueKind::Short:
            return new (scope.allocate<ShortValue>()) ShortValue((short) value, nullptr);
        case ValueKind::Int:
            return new (scope.allocate<IntValue>()) IntValue((int) value, nullptr);
        case ValueKind::Long:
            return new (scope.allocate<LongValue>()) LongValue((long) value, isExe64Bit(), nullptr);
        case ValueKind::BigInt:
            return new (scope.allocate<BigIntValue>()) BigIntValue((long long) value, nullptr);
        case ValueKind::Int128:
            // TODO int128 is_negative is always false
            return new (scope.allocate<Int128Value>()) Int128Value((uint64_t) value, false, nullptr);
        case ValueKind::UChar:
            return new (scope.allocate<UCharValue>()) UCharValue((char) value, nullptr);
        case ValueKind::UShort:
            return new (scope.allocate<UShortValue>()) UShortValue((short) value, nullptr);
        case ValueKind::UInt:
            return new (scope.allocate<UIntValue>()) UIntValue((int) value, nullptr);
        case ValueKind::ULong:
            return new (scope.allocate<ULongValue>()) ULongValue((long) value, isExe64Bit(), nullptr);
        case ValueKind::UBigInt:
            return new (scope.allocate<UBigIntValue>()) UBigIntValue((long long) value, nullptr);
        case ValueKind::UInt128:
            return new (scope.allocate<UInt128Value>()) UInt128Value((uint64_t) value, false, nullptr);
        case ValueKind::Bool:
            return new (scope.allocate<BoolValue>()) BoolValue(value, nullptr);
        case ValueKind::NumberValue:
            return new (scope.allocate<NumberValue>()) NumberValue(value, nullptr);
        default:
            return nullptr;
    }
}