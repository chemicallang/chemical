// Copyright (c) Qinetik 2024.

#include "DynamicType.h"

DynamicType::DynamicType(std::unique_ptr<BaseType> referenced) : referenced(std::move(referenced)) {

}

void DynamicType::link(SymbolResolver &linker, std::unique_ptr<BaseType> &current) {
    referenced->link(linker, referenced);
}