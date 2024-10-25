// Copyright (c) Qinetik 2024.

#include "StringType.h"
#include "CharType.h"

BaseType* StringType::create_child_type(ASTAllocator& allocator) const {
    return new (allocator.allocate<CharType>()) CharType(location);
}

//hybrid_ptr<BaseType> StringType::get_child_type() {
//    return hybrid_ptr<BaseType> { (BaseType*) &CharType::instance, false };
//}

BaseType* StringType::known_child_type() {
    return (BaseType*) &CharType::instance;
}