// Copyright (c) Chemical Language Foundation 2025.

#include "StringValue.h"
#include "ast/types/PointerType.h"
#include "ast/types/CharType.h"
#include "ast/types/ArrayType.h"
#include "ast/statements/VarInit.h"
#include <iostream>

BaseType* StringValue::create_type(ASTAllocator& allocator) {
    return new (allocator.allocate<StringType>()) StringType(encoded_location());
}

bool StringValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *type) {
    if(type && type->kind() == BaseTypeKind::Array) {
        is_array = true;
        auto arrayType = (ArrayType*) (type);
        if(arrayType->get_array_size() > value.size()) {
            length = (unsigned int) arrayType->get_array_size();
        } else if(arrayType->has_no_array_size()) {
            length = value.size() + 1; // adding 1 for the last /0
        } else {
#ifdef DEBUG
        throw std::runtime_error("unknown");
#endif
        }
    }
    return true;
}

Value *StringValue::index(InterpretScope &scope, int i) {
#ifdef DEBUG
    if (i < 0 || i >= value.size()) {
        std::cerr << "[InterpretError] access index " << std::to_string(i) << " out of bounds for string " << value <<
        " of length " << std::to_string(value.size());
    }
#endif
    return new CharValue(value[i], encoded_location());
}
