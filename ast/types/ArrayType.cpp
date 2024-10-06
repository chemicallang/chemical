// Copyright (c) Qinetik 2024.

#include "ArrayType.h"

bool ArrayType::link(SymbolResolver &linker) {
    return elem_type->link(linker);
}