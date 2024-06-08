// Copyright (c) Qinetik 2024.

#include "StringType.h"
#include "CharType.h"

std::unique_ptr<BaseType> StringType::create_child_type() const {
    return std::unique_ptr<BaseType>(new CharType());
}