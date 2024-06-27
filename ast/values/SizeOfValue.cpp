// Copyright (c) Qinetik 2024.

#include "SizeOfValue.h"

#include <memory>
#include "ast/base/BaseType.h"
#include "compiler/SymbolResolver.h"
#include "ULongValue.h"

SizeOfValue::SizeOfValue(BaseType* for_type) : for_type(for_type) {

}

void SizeOfValue::link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr) {
    for_type->link(linker, for_type);
    value_ptr = std::make_unique<ULongValue>(for_type->byte_size(linker.is64Bit), linker.is64Bit);
}