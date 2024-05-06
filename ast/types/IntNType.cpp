// Copyright (c) Qinetik 2024.

#include "IntNType.h"
#include "IntType.h"
#include "ast/values/IntValue.h"
#include "ast/values/UIntValue.h"
#include "ast/values/ShortValue.h"
#include "ast/values/UShortValue.h"
#include "ast/values/LongValue.h"
#include "ast/values/ULongValue.h"
#include "ast/values/BigIntValue.h"
#include "ast/values/UBigIntValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "UIntType.h"

llvm::Type *IntNType::llvm_type(Codegen &gen) const {
    auto ty = gen.builder->getIntNTy(number);
    if(!ty) {
        gen.error("Couldn't get intN type for int:" + std::to_string(number));
    }
    return ty;
}

#endif

Value *IntType::create(int64_t value) {
    return new IntValue(value);
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
    return new LongValue(value, number == 64);
}

Value *ULongType::create(int64_t value) {
    return new ULongValue(value, number == 64);
}

Value *BigIntType::create(int64_t value) {
    return new BigIntValue(value);
}

Value *UBigIntType::create(int64_t value) {
    return new UBigIntValue(value);
}