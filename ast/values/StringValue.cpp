// Copyright (c) Chemical Language Foundation 2025.

#include "StringValue.h"
#include "ast/types/PointerType.h"
#include "ast/values/IntNumValue.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/types/ArrayType.h"
#include "ast/statements/VarInit.h"
#include "compiler/lab/TargetData.h"
#include <iostream>

uint64_t StringValue::byte_size(const TargetData& target) {
    return target.is64Bit ? 8 : 4;
}