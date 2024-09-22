// Copyright (c) Qinetik 2024.

#include "ArrayType.h"

void ArrayType::link(SymbolResolver &linker) {
    elem_type->link(linker);
}