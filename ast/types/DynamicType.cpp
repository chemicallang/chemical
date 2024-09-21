// Copyright (c) Qinetik 2024.

#include "DynamicType.h"

DynamicType::DynamicType(BaseType* referenced, CSTToken* token) : referenced(referenced), TokenizedBaseType(token) {

}

void DynamicType::link(SymbolResolver &linker, BaseType*& current) {
    referenced->link(linker, referenced);
}