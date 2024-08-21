// Copyright (c) Qinetik 2024.

#include "StringType.h"
#include "CharType.h"

std::unique_ptr<BaseType> StringType::create_child_type() const {
    return std::unique_ptr<BaseType>(new CharType());
}

hybrid_ptr<BaseType> StringType::get_child_type() {
    return hybrid_ptr<BaseType> { (BaseType*) &CharType::instance, false };
}

BaseType* StringType::known_child_type() {
    return (BaseType*) &CharType::instance;
}