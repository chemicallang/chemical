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

bool BigIntType::satisfies(ASTAllocator& allocator, Value* value) {
    return value->value_type() == ValueType::BigInt;
}

bool IntType::satisfies(ASTAllocator& allocator, Value* value) {
    return value->value_type() == ValueType::Int;
}

bool LongType::satisfies(ASTAllocator& allocator, Value* value) {
    return value->value_type() == ValueType::Long;
}

bool ShortType::satisfies(ASTAllocator& allocator, Value* value) {
    return value->value_type() == ValueType::Short;
}

bool UBigIntType::satisfies(ASTAllocator& allocator, Value* value) {
    return value->value_type() == ValueType::UBigInt;
}

bool UInt128Type::satisfies(ASTAllocator& allocator, Value* value) {
    return value->value_type() == ValueType::UInt128;
}

bool ULongType::satisfies(ASTAllocator& allocator, Value* value) {
    return value->value_type() == ValueType::ULong;
}

bool UShortType::satisfies(ASTAllocator& allocator, Value* value) {
    return value->value_type() == ValueType::UShort;
}
