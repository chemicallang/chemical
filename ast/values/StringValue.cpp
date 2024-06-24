// Copyright (c) Qinetik 2024.

#include "StringValue.h"
#include "ast/types/PointerType.h"
#include "ast/types/CharType.h"

std::unique_ptr<BaseType> StringValue::create_type() const {
    return std::make_unique<PointerType>(std::make_unique<CharType>());
}