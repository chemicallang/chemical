// Copyright (c) Chemical Language Foundation 2025.

#include "StringValue.h"
#include "ast/types/PointerType.h"
#include "ast/types/CharType.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/types/ArrayType.h"
#include "ast/statements/VarInit.h"
#include <iostream>

BaseType* StringValue::create_type(ASTAllocator& allocator) {
    return new (allocator.allocate<StringType>()) StringType();
}

Value *StringValue::index(InterpretScope &scope, int i) {
#ifdef DEBUG
    if (i < 0 || i >= value.size()) {
        std::cerr << "[InterpretError] access index " << std::to_string(i) << " out of bounds for string " << value <<
        " of length " << std::to_string(value.size());
    }
#endif
    return new CharValue(value[i], scope.global->typeBuilder.getCharType(), encoded_location());
}
