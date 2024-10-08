// Copyright (c) Qinetik 2024.

#include "SizeOfValue.h"

#include <memory>
#include "ast/base/BaseType.h"
#include "compiler/SymbolResolver.h"
#include "ULongValue.h"

SizeOfValue::SizeOfValue(BaseType* for_type, CSTToken* token) : for_type(for_type), UBigIntValue(0, token) {

}

void SizeOfValue::calculate_size(bool is64Bit) {
    value = for_type->byte_size(is64Bit);
}

bool SizeOfValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    for_type->link(linker);
    return true;
}