// Copyright (c) Qinetik 2024.

#include "ArrayType.h"

void ArrayType::link(SymbolResolver &linker, std::unique_ptr<BaseType> &current) {
    elem_type->link(linker, elem_type);
}