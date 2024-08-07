// Copyright (c) Qinetik 2024.

#include "SizeOfValue.h"

#include <memory>
#include "ast/base/BaseType.h"
#include "compiler/SymbolResolver.h"
#include "ULongValue.h"

SizeOfValue::SizeOfValue(BaseType* for_type) : for_type(for_type), UBigIntValue(0) {

}

void SizeOfValue::calculate_size(bool is64Bit) {
    value = for_type->byte_size(is64Bit);
}

void SizeOfValue::link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr) {
    for_type->link(linker, for_type);
}