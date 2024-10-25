// Copyright (c) Qinetik 2024.

#include "StringValue.h"
#include "ast/types/PointerType.h"
#include "ast/types/CharType.h"
#include "ast/types/ArrayType.h"
#include "ast/statements/VarInit.h"

BaseType* StringValue::create_type(ASTAllocator& allocator) {
    return new (allocator.allocate<StringType>()) StringType(location);
}

bool StringValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *type) {
    if(type && type->kind() == BaseTypeKind::Array) {
        is_array = true;
        auto arrayType = (ArrayType*) (type);
        if(arrayType->array_size > (int) value.size()) {
            length = (unsigned int) arrayType->array_size;
        } else if(arrayType->array_size == -1) {
            length = value.size() + 1; // adding 1 for the last /0
        } else {
#ifdef DEBUG
        throw std::runtime_error("unknown");
#endif
        }
    }
    return true;
}