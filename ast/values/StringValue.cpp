// Copyright (c) Qinetik 2024.

#include "StringValue.h"
#include "ast/types/PointerType.h"
#include "ast/types/CharType.h"
#include "ast/types/ArrayType.h"
#include "ast/statements/VarInit.h"

std::unique_ptr<BaseType> StringValue::create_type() {
    return std::make_unique<StringType>(nullptr);
}

void StringValue::link(SymbolResolver &linker, VarInitStatement *stmnt) {
    if(stmnt->type.has_value() && stmnt->type.value()->kind() == BaseTypeKind::Array) {
        is_array = true;
        auto arrayType = (ArrayType*) (stmnt->type.value().get());
        if(arrayType->array_size > (int) value.size()) {
            length = arrayType->array_size;
        } else if(arrayType->array_size == -1) {
            length = value.size() + 1; // adding 1 for the last /0
        }
    }
}