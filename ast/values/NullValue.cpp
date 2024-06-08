// Copyright (c) Qinetik 2024.

#include "NullValue.h"
#include "ast/types/PointerType.h"
#include "ast/types/VoidType.h"

std::unique_ptr<BaseType> NullValue::create_type() const {
    return std::make_unique<PointerType>(std::make_unique<VoidType>());
}