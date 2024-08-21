// Copyright (c) Qinetik 2024.

#include "IntNType.h"
#include "IntType.h"
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

const IntType IntType::instance;
const BigIntType BigIntType::instance;
const Int128Type Int128Type::instance;
const ShortType ShortType::instance;
const UBigIntType UBigIntType::instance;
const UInt128Type UInt128Type::instance;
const UIntType UIntType::instance;
const UShortType UShortType::instance;
const LongType LongType::instance64Bit(true);
const LongType LongType::instance32Bit(false);
const ULongType ULongType::instance64Bit(true);
const ULongType ULongType::instance32Bit(false);

Value *IntType::create(int64_t value) {
    return new IntValue(value);
}

Value *CharType::create(int64_t value) {
    return new CharValue(value);
}

Value *UCharType::create(int64_t value) {
    return new UCharValue(value);
}

Value *UIntType::create(int64_t value) {
    return new UIntValue(value);
}

Value *ShortType::create(int64_t value) {
    return new ShortValue(value);
}

Value *UShortType::create(int64_t value) {
    return new UShortValue(value);
}

Value *LongType::create(int64_t value) {
    return new LongValue(value, num_bits() == 64);
}

Value *ULongType::create(int64_t value) {
    return new ULongValue(value, num_bits() == 64);
}

Value *BigIntType::create(int64_t value) {
    return new BigIntValue(value);
}

Value *UBigIntType::create(int64_t value) {
    return new UBigIntValue(value);
}

Value *Int128Type::create(int64_t value) {
    bool is_neg = value < 0;
    return new Int128Value(is_neg ? -value : value, is_neg);
}

Value *UInt128Type::create(int64_t value) {
    return new UInt128Value(static_cast<uint64_t>(value), (value < 0) ? UINT64_MAX : 0);
}

bool BigIntType::satisfies(Value *value) {
    return value->value_type() == ValueType::BigInt;
}

bool IntType::satisfies(Value *value) {
    return value->value_type() == ValueType::Int;
}

bool LongType::satisfies(Value *value) {
    return value->value_type() == ValueType::Long;
}

bool ShortType::satisfies(Value *value) {
    return value->value_type() == ValueType::Short;
}

bool UBigIntType::satisfies(Value *value) {
    return value->value_type() == ValueType::UBigInt;
}

bool UInt128Type::satisfies(Value *value) {
    return value->value_type() == ValueType::UInt128;
}

bool ULongType::satisfies(Value *value) {
    return value->value_type() == ValueType::ULong;
}

bool UShortType::satisfies(Value *value) {
    return value->value_type() == ValueType::UShort;
}
